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

#include "AppTask.h"
#include "CHIPDeviceManager.h"
#include <stdlib.h>
#include "matter_ble.h"
#include "os_mem.h"

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceManager;
using namespace ::chip::DeviceLayer;
using namespace ::chip::System;

#if !CHIP_DEVICE_CONFIG_SUPPORTS_CONCURRENT_CONNECTION
extern "C" otInstance *otInstanceInitSingle(void)
{
    return NULL;
}
#endif

extern "C" void ChipTestShutdown(void)
{
    ChipLogProgress(DeviceLayer, "Light switch APP Demo! Shutdown Now!");
    CHIPDeviceManager::GetInstance().Shutdown();
}

extern "C" void InitGPIO(void)
{
    GetAppTask().InitGpio();
}

extern "C" void ChipTest(void)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    DBG_DIRECT("[ChipTest] remain data_ram_size = %d, buffer_ram_size = %d",
               os_mem_peek(RAM_TYPE_DATA_ON), os_mem_peek(RAM_TYPE_BUFFER_ON));

    matter_ble_init(1);

    err = GetAppTask().StartAppTask();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "GetAppTask().StartAppTask() failed");
        return;
    }
}

extern "C" void InitiateSingleSwitch(uint8_t index, uint8_t action)
{
    if (index >= MAX_SUPPORTED_LIGHT_NUM)
    {
        ChipLogError(DeviceLayer, "Endpoint index exceed!");
        return;
    }

    LightSwitch::GetInstance().InitiateActionSwitch(kLightSwitchEndpointId[index], action);
}

extern "C" void InitiateGroupSwitch(uint8_t action)
{
    LightSwitch::GetInstance().InitiateActionSwitch(kLightSwitchForGroupEndpointId, action);
}

extern "C" void InitiateGroupSwitch2(uint8_t index, uint8_t action)
{
    LightSwitch::GetInstance().InitiateActionSwitch(kLightSwitchEndpointId[index], action);
}

#if CONFIG_ENABLE_ATTRIBUTE_SUBSCRIBE

extern "C" void ShutdownSubscribeRequestForOneNode(uint8_t index)
{
    if (index >= MAX_SUPPORTED_LIGHT_NUM)
    {
        ChipLogError(DeviceLayer, "Endpoint index exceed!");
        return;
    }

    LightSwitch::GetInstance().ShutdownSubscribeRequestForOneNode(kLightSwitchEndpointId[index]);
}

extern "C" void SubscribeRequestForOneNode(uint8_t index)
{
    if (index >= MAX_SUPPORTED_LIGHT_NUM)
    {
        ChipLogError(DeviceLayer, "Endpoint index exceed!");
        return;
    }
    LightSwitch::GetInstance().SubscribeRequestForOneNode(kLightSwitchEndpointId[index]);
}

typedef void (*P_ProcessCommandCallback)(uint8_t status, uint8_t index);
static P_ProcessCommandCallback g_NotifyUpperStatusChange = NULL;

void UpdateLightingStatetoGUI(EndpointId endpointId, uint8_t status)
{
    uint8_t index;

    for (index = 0; index < MAX_SUPPORTED_LIGHT_NUM; index++)
    {
        if (endpointId == kLightSwitchEndpointId[index])
        {
            break;
        }
    }

    if (g_NotifyUpperStatusChange)
    {
        g_NotifyUpperStatusChange(status, index);
    }
}

extern "C" void RegisterSwitchCommandCallback(P_ProcessCommandCallback cback)
{
    g_NotifyUpperStatusChange = cback;
}
#endif

extern "C" unsigned int __atomic_fetch_add_4(volatile void * ptr, unsigned int val, int memorder)
{
    unsigned int old;

    taskENTER_CRITICAL();
    old = *(unsigned int *)ptr;
    *(unsigned int *)ptr = old + val;
    taskEXIT_CRITICAL();

    return old;
}

extern "C" bool __atomic_compare_exchange_4(volatile void * pulDestination, void * ulComparand, unsigned int desired, bool weak,
                                            int success_memorder, int failure_memorder)
{
    bool ret = false;
    unsigned int old;

    taskENTER_CRITICAL();

    old = *(volatile unsigned int *)pulDestination;
    if (old == *(unsigned int *)ulComparand)
    {
        *(volatile unsigned int *)pulDestination = desired;
        ret = true;
    }
    else
    {
        *(unsigned int *)ulComparand = old;
        ret = false;
    }

    taskEXIT_CRITICAL();
    return ret;
}

extern "C" unsigned int __atomic_fetch_sub_4(volatile void * ptr, unsigned int val, int memorder)
{
    unsigned int old;

    taskENTER_CRITICAL();
    old = *(unsigned int *)ptr;
    *(unsigned int *)ptr = old - val;
    taskEXIT_CRITICAL();

    return old;
}

extern "C" bool __atomic_compare_exchange_1(volatile void * pulDestination, void * ulComparand, unsigned char desired, bool weak,
                                            int success_memorder, int failure_memorder)
{
    bool ret = false;
    unsigned char old;

    taskENTER_CRITICAL();

    old = *(volatile unsigned char *)pulDestination;
    if (old == *(unsigned char *)ulComparand)
    {
        *(volatile unsigned char *)pulDestination = desired;
        ret = true;
    }
    else
    {
        *(unsigned char *)ulComparand = old;
        ret = false;
    }

    taskEXIT_CRITICAL();
    return ret;
}

extern "C" unsigned int __atomic_fetch_and_4(volatile void * pulDestination, unsigned int ulValue, int memorder)
{
    unsigned int old;

    taskENTER_CRITICAL();
    old = *(unsigned int *)pulDestination;
    *(unsigned int *)pulDestination = old & ulValue;
    taskEXIT_CRITICAL();

    return old;
}
