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

#pragma once

#include <app/util/basic-types.h>
#include <lib/core/CHIPError.h>

#define SWITCH_ACTION_TOGGLE   0
#define SWITCH_ACTION_ON       1
#define SWITCH_ACTION_OFF      2

class LightSwitch
{
public:
    void Init(chip::EndpointId aLightGenericSwitchEndpointId);
    void InitiateActionSwitch(chip::EndpointId endpointId, uint8_t action, bool isGroup);
    void GenericSwitchInitialPress();
    void GenericSwitchReleasePress();

    static LightSwitch &GetInstance()
    {
        static LightSwitch sLightSwitch;
        return sLightSwitch;
    }

private:
    chip::EndpointId mLightGenericSwitchEndpointId;
};
