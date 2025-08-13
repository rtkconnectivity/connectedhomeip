/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
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

#include <platform/internal/CHIPDeviceLayerInternal.h>

namespace chip {
namespace DeviceLayer {

class FactoryDataDecoder
{
public:
    CHIP_ERROR ReadFactoryData(uint8_t * buffer, uint32_t buffer_len, uint16_t * pfactorydata_len);
    CHIP_ERROR DecodeFactoryData(uint8_t * buffer, FactoryData * fdata, uint16_t factorydata_len);

    CHIP_ERROR FactoryDataGetValue(uint8_t tag,
                                   uint8_t *input,
                                   uint32_t input_len,
                                   uint8_t *buf,
                                   size_t buf_size,
                                   uint16_t *data_len);

    static FactoryDataDecoder & GetInstance()
    {
        static FactoryDataDecoder instance;
        return instance;
    }
};

} // namespace DeviceLayer
} // namespace chip
