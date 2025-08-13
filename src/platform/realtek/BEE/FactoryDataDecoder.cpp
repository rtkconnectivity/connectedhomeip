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

#include "FactoryDataDecoder.h"
#include "matter_utils.h"
#include <platform/internal/CHIPDeviceLayerInternal.h>

namespace chip {
namespace DeviceLayer {

CHIP_ERROR FactoryDataDecoder::ReadFactoryData(uint8_t * buffer, uint32_t buffer_len, uint16_t * pfactorydata_len)
{
    uint32_t ret = 0;
    ret          = ReadFactory(buffer, buffer_len, pfactorydata_len);
    if (ret != 0)
        return CHIP_ERROR_INTERNAL;

    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataDecoder::DecodeFactoryData(uint8_t * buffer, FactoryData * fdata, uint16_t factorydata_len)
{
    uint32_t ret = 0;
    ret          = DecodeFactory(buffer, fdata, factorydata_len);
    if (ret != 0)
        return CHIP_ERROR_INTERNAL;

    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataDecoder::FactoryDataGetValue(uint8_t tag,
                                                   uint8_t *input,
                                                   uint32_t input_len,
                                                   uint8_t *buf,
                                                   size_t buf_size,
                                                   uint16_t *data_len)
{
    if(0 != matter_factory_data_value_get(tag, input, input_len, buf, buf_size, data_len))
    {
        return CHIP_ERROR_INTERNAL;
    }

    return CHIP_NO_ERROR;
}

} // namespace DeviceLayer
} // namespace chip
