/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
 *    All rights reserved.
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

#include <app/server/Server.h>
#include <app/util/binding-table.h>
#include <controller/InvokeInteraction.h>
#include <app/clusters/switch-server/switch-server.h>
#include <app-common/zap-generated/attributes/Accessors.h>

#include "LightSwitch.h"
#include "BindingHandler.h"

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;

void LightSwitch::Init(chip::EndpointId aLightGenericSwitchEndpointId)
{
    mLightGenericSwitchEndpointId = aLightGenericSwitchEndpointId;
}

void LightSwitch::InitiateActionSwitch(chip::EndpointId endpointId, uint8_t action, bool isGroup)
{
    BindingTable & bindingTable = BindingTable::GetInstance();
    BindingCommandData * data = Platform::New<BindingCommandData>();

    if (!bindingTable.Size())
    {
        ChipLogError(DeviceLayer, "bindingTable empty");
        return;
    }

    if (data)
    {
        data->localEndpointId = endpointId;
        data->clusterId  = Clusters::OnOff::Id;
        data->isGroup = isGroup;

        switch (action)
        {
        case SWITCH_ACTION_TOGGLE:
            data->commandId = Clusters::OnOff::Commands::Toggle::Id;
            break;

        case SWITCH_ACTION_ON:
            data->commandId = Clusters::OnOff::Commands::On::Id;
            break;

        case SWITCH_ACTION_OFF:
            data->commandId = Clusters::OnOff::Commands::Off::Id;
            break;

        default:
            Platform::Delete(data);
            return;
        }

        DeviceLayer::PlatformMgr().ScheduleWork(SwitchWorkerFunction, reinterpret_cast<intptr_t>(data));
    }
}

void LightSwitch::GenericSwitchInitialPress()
{
    DeviceLayer::SystemLayer().ScheduleLambda([this] {
        // Press moves Position from 0 (idle) to 1 (press)
        uint8_t newPosition = 1;

        Clusters::Switch::Attributes::CurrentPosition::Set(mLightGenericSwitchEndpointId, newPosition);
        // InitialPress event takes newPosition as event data
        Clusters::SwitchServer::Instance().OnInitialPress(mLightGenericSwitchEndpointId, newPosition);
    });
}

void LightSwitch::GenericSwitchReleasePress()
{
    DeviceLayer::SystemLayer().ScheduleLambda([this] {
        // Release moves Position from 1 (press) to 0 (idle)
        uint8_t previousPosition = 1;
        uint8_t newPosition      = 0;

        Clusters::Switch::Attributes::CurrentPosition::Set(mLightGenericSwitchEndpointId, newPosition);
        // ShortRelease event takes previousPosition as event data
        Clusters::SwitchServer::Instance().OnShortRelease(mLightGenericSwitchEndpointId, previousPosition);
    });
}
