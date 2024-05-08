/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <stdlib.h>
#include "BindingHandler.h"
#include "CHIPDeviceManager.h"
#include "DeviceCallbacks.h"
#include "LEDWidget.h"
#include "LightSwitch.h"
#include <DeviceInfoProviderImpl.h>
//#include <lwip_netconf.h>
#include <app/clusters/bindings/bindings.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/clusters/network-commissioning/network-commissioning.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <app/util/af.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <lib/core/ErrorStr.h>
#include <platform/Bee/BeeConfig.h>
#include <platform/Bee/FactoryDataProvider.h>
//#include <platform/Bee/NetworkCommissioningDriver.h>
#include <platform/CHIPDeviceLayer.h>
#include <setup_payload/ManualSetupPayloadGenerator.h>
#include <setup_payload/QRCodeSetupPayloadGenerator.h>
#include <platform/OpenThread/GenericNetworkCommissioningThreadDriver.h>
#include <platform/OpenThread/GenericThreadStackManagerImpl_OpenThread.h>

#include <support/CHIPMem.h>

#if CONFIG_ENABLE_PW_RPC
#include <Rpc.h>
#endif

#if CONFIG_ENABLE_CHIP_SHELL
#include <lib/shell/Engine.h>
#endif

#if CHIP_ENABLE_OPENTHREAD
#include <mbedtls/platform.h>
#include <openthread/cli.h>
#include <openthread/dataset.h>
#include <openthread/error.h>
#include <openthread/heap.h>
#include <openthread/icmp6.h>
#include <openthread/instance.h>
#include <openthread/link.h>
#include <platforms/openthread-system.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>
#include <openthread/dataset_ftd.h>
#endif // CHIP_ENABLE_OPENTHREAD

#include "chip_porting.h"
#include <os_mem.h>

using namespace ::chip;
using namespace ::app;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceManager;
using namespace ::chip::DeviceLayer;
using namespace ::chip::System;

//1_to_8  connectedhomeip/examples/light-switch-app/light-switch-common/light-switch-app-1_to_8.zap
#define MAX_SUPPORTED_LIGHT_NUM 8
constexpr EndpointId kLightSwitchForGroupEndpointId = 1;
constexpr EndpointId kLightGenericSwitchEndpointId = 10;
constexpr EndpointId kLightSwitchEndpointId[MAX_SUPPORTED_LIGHT_NUM] =
{
    2, 3, 4, 5, 6, 7, 8, 9
};

#if 0
//1_to_11   You must modify src/app/util/config.h EMBER_BINDING_TABLE_SIZE > 10 
#define MAX_SUPPORTED_LIGHT_NUM 11
constexpr EndpointId kLightSwitchForGroupEndpointId = 1;
constexpr EndpointId kLightGenericSwitchEndpointId = 10;
constexpr EndpointId kLightSwitchEndpointId[MAX_SUPPORTED_LIGHT_NUM] =
{
    2, 3, 4, 5, 6, 7, 8, 9,
    11, 12, 13
};

//1_to_2
#define MAX_SUPPORTED_LIGHT_NUM 2
constexpr EndpointId kLightSwitchForGroupEndpointId = 1;
constexpr EndpointId kLightGenericSwitchEndpointId = 4;
constexpr EndpointId kLightSwitchEndpointId[MAX_SUPPORTED_LIGHT_NUM] =
{
    2, 3
};
#endif

LEDWidget statusLED1;

Identify gIdentify0 = {
    chip::EndpointId{ 0 },
    [](Identify *) { ChipLogProgress(Zcl, "onIdentifyStart"); },
    [](Identify *) { ChipLogProgress(Zcl, "onIdentifyStop"); },
    Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator,
};

Identify gIdentify1 = {
    chip::EndpointId{ 1 },
    [](Identify *) { ChipLogProgress(Zcl, "onIdentifyStart"); },
    [](Identify *) { ChipLogProgress(Zcl, "onIdentifyStop"); },
    Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator,
};

static DeviceCallbacks EchoCallbacks;
chip::DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;

static void InitServer(intptr_t context)
{
    // Init ZCL Data Model and CHIP App Server
    static chip::CommonCaseDeviceServerInitParams initParams;
    initParams.InitializeStaticResourcesBeforeServerInit();
    chip::Server::GetInstance().Init(initParams);
    gExampleDeviceInfoProvider.SetStorageDelegate(&Server::GetInstance().GetPersistentStorage());
    chip::DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);

    // QR code will be used with CHIP Tool
    PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));

    InitBindingHandler();
}

extern "C" void InitiateSingleSwitch(uint8_t index, uint8_t action)
{
    if(index >= MAX_SUPPORTED_LIGHT_NUM)
    {
        ChipLogError(DeviceLayer, "Endpoint index exceed!");
        return;
    }

    LightSwitch::GetInstance().InitiateActionSwitch(kLightSwitchEndpointId[index], action, false);
}

extern "C" void InitiateGroupSwitch(uint8_t action)
{
    LightSwitch::GetInstance().InitiateActionSwitch(kLightSwitchForGroupEndpointId, action, true);
}

extern "C" void ChipTestShutdown(void)
{
    ChipLogProgress(DeviceLayer, "Lighting APP Demo! Shutdown Now!");
    CHIPDeviceManager::GetInstance().Shutdown();
}

void ButtonPressCallback(uint8_t index, uint8_t state)
{
    ChipLogProgress(DeviceLayer, "Key %d, State %d", index, state);

    switch (index)
    {
    case 0:
        if(state == MATTER_GPIO_KEY_STATE_RELEASE)
        {
            ChipTestShutdown();
            WDT_SystemReset(RESET_ALL, SW_RESET_APP_START);
        }
        break;

    case 1:
        if(state == MATTER_GPIO_KEY_STATE_RELEASE)
        {
            InitiateGroupSwitch(SWITCH_ACTION_TOGGLE);
        }
        break;

    case 2:
        if(state == MATTER_GPIO_KEY_STATE_RELEASE)
        {
            LightSwitch::GetInstance().GenericSwitchReleasePress();
        }
        else
        {
            LightSwitch::GetInstance().GenericSwitchInitialPress();
        }
        break;

    case 3:
    case 4:
        break;

    default:
        break;
    }
}

extern "C" void InitGPIO(void)
{
    LEDWidget::InitGpio();

    statusLED1.Init(0);
    statusLED1.Set(true);

    matter_gpio_init(ButtonPressCallback);
}

extern "C" void ChipTest(void)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    ChipLogProgress(DeviceLayer, "Light Switch App Demo!");

#if CONFIG_ENABLE_PW_RPC
    chip::rpc::Init();
#endif

    LightSwitch::GetInstance().Init(kLightGenericSwitchEndpointId);

    err = CHIPDeviceManager::GetInstance().Init(&EchoCallbacks);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "DeviceManagerInit() - ERROR!\r\n");
    }
    else
    {
        ChipLogProgress(DeviceLayer, "DeviceManagerInit() - OK\r\n");
    }

    chip::DeviceLayer::PlatformMgr().ScheduleWork(InitServer, 0);

#if CONFIG_ENABLE_CHIP_SHELL
    chip::Shell::Engine::Root().Init();
    chip::Shell::Engine::Root().RunMainLoop();
#endif

	ChipLogProgress(DeviceLayer, "os_mem_peek(RAM_TYPE_DATA_ON) : (%u)\r\n", os_mem_peek(RAM_TYPE_DATA_ON));
}

bool lowPowerClusterSleep()
{
    return true;
}
