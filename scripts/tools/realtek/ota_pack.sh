#!/bin/bash

#
# Copyright (c) 2022 Project CHIP Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

set -e

# check environment variable
for var in MATTER_EXAMPLE_PATH OT_SRCDIR REALTEK_SDK_PATH OUT_FOLDER OTA_FOLDER; do
    if [ -z "${!var}" ]; then
        echo "Environment variable $var not set!"
        exit 1
    fi
done

BOARD_TARGET="$1"
RT_PLATFORM="$2"

PROJECT_CONFIG=$MATTER_EXAMPLE_PATH/main/include/CHIPProjectConfig.h
OTA_IMG_TOOL="$OT_SRCDIR/../matter/connectedhomeip/src/app/ota_image_tool.py"

# select tools for different platforms
if [ "$(uname -s)" = "Darwin" ]; then
    FLASHMAP_GEN_CLI="$REALTEK_SDK_PATH/tools/FlashMapGenerateCli/FlashMapGenerateCli.macOS"
    PACKCLI="$REALTEK_SDK_PATH/tools/PackCli/PackCli.macOS"
elif [ "$(uname -s)" = "Linux" ]; then
    if [ "${RT_PLATFORM}" = "bee4" ]; then
        FLASHMAP_GEN_CLI="${REALTEK_SDK_PATH}/tools/FlashMapGenerateCli/FlashMapGenerateCli"
        PACKCLI="${REALTEK_SDK_PATH}/tools/PackCli/PackCli"
    elif [ "${RT_PLATFORM}" = "rtl8752h" ]; then
        FLASHMAP_GEN_CLI="${OT_SRCDIR}/tools/FlashMapGenerateCli"
        PACKCLI="${OT_SRCDIR}/tools/PackCli"
    fi
fi

# read OTA metadata
VENDOR_ID=$(awk '/#define CHIP_DEVICE_CONFIG_DEVICE_VENDOR_ID/{print $3}' "$PROJECT_CONFIG")
PRODUCT_ID=$(awk '/#define CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID/{print $3}' "$PROJECT_CONFIG")
VERSION=$(awk '/#define CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION/{print $3; exit}' "$PROJECT_CONFIG")
VERSION_STR=$(awk '/#define CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING/{print $3}' "$PROJECT_CONFIG")

echo "VENDOR_ID=$VENDOR_ID"
echo "PRODUCT_ID=$PRODUCT_ID"
echo "VERSION=$VERSION"
echo "VERSION_STR=$VERSION_STR"

chmod +x "$FLASHMAP_GEN_CLI"
chmod +x "$PACKCLI"

# use different copy and image generation methods for dual/single bank modes
if [[ $BOARD_TARGET == *"dual"* ]]; then
    cp -f "$OT_SRCDIR/vendor/$RT_PLATFORM/$BOARD_TARGET"/*.ini "$OTA_FOLDER"
    cp -f "$OT_SRCDIR/vendor/$RT_PLATFORM/${BOARD_TARGET%/secure}"/firmware/bank0/* "$OTA_FOLDER"
    cp -f "$OT_SRCDIR/vendor/$RT_PLATFORM/${BOARD_TARGET%/secure}"/firmware/bank1/* "$OTA_FOLDER"
    cp -f "$OUT_FOLDER"/bin/*MP_*.bin "$OTA_FOLDER"

    OTA_VERSION="$3"
    "$FLASHMAP_GEN_CLI" "$OTA_FOLDER" "$OTA_VERSION" "$OTA_FOLDER"
else
    cp -f "$OT_SRCDIR/vendor/$RT_PLATFORM/$BOARD_TARGET"/*.ini "$OTA_FOLDER"
    cp -f "$OUT_FOLDER"/bin/*MP_*.bin "$OTA_FOLDER"
fi

# generate pack bin
rm -rf "$OTA_FOLDER"/*.ota
if [ "${RT_PLATFORM}" = "bee4" ]; then
    "$PACKCLI" 8772gwp ota "$OTA_FOLDER" "$OTA_FOLDER"/..
elif [ "${RT_PLATFORM}" = "rtl8752h" ]; then
    "$PACKCLI" 8752h ota "$OTA_FOLDER" "$OTA_FOLDER"/..
fi

# generate matter ota bin
"$OTA_IMG_TOOL" create \
    -v "$VENDOR_ID" \
    -p "$PRODUCT_ID" \
    -vn "$VERSION" \
    -vs "$VERSION_STR" \
    -da sha256 \
    "$OTA_FOLDER/ImgPacketFile"*.bin \
    "$OTA_FOLDER/matter.ota"

# remove unnecessary files
rm -rf "$OTA_FOLDER"/*.bin
rm -rf "$OTA_FOLDER"/*.ini
