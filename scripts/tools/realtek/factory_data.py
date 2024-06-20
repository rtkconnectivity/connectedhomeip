#!/usr/bin/env python3
#
#    Copyright (c) 2022 Project CHIP Authors
#    All rights reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

import os
import sys
import logging
import argparse
import subprocess
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.serialization import load_der_private_key
import factory_data_pb2 as factory_data
import qrcode

SDK_ROOT = os.path.dirname(os.path.realpath(__file__))
SDK_ROOT = os.path.dirname(SDK_ROOT)
SDK_ROOT = os.path.dirname(SDK_ROOT)
SDK_ROOT = os.path.dirname(SDK_ROOT)

sys.path.append(os.path.join(SDK_ROOT, "src/setup_payload/python"))
from generate_setup_payload import CommissioningFlow, SetupPayload



INVALID_PASSCODES = [00000000, 11111111, 22222222, 33333333, 44444444, 55555555,
                     66666666, 77777777, 88888888, 99999999, 12345678, 87654321]

FACTORY_DATA = factory_data.FactoryDataProvider()

args = None


def open_wrap(file_name: str, mode: str):
    if args is not None:
        real_file_name = os.path.join(args.out, file_name)
        return open(real_file_name, mode)
    else:
        raise Exception


def align_16(length):
    return (length + 15) & ~15

# split the private key der file to public and private keys, return private key
def get_raw_private_key_der(der_file: str, password: str):
    """ Split given der file to get separated key pair consisting of public and private keys.
    Args:
        der_file (str): Path to .der file containing public and private keys
        password (str): Password to decrypt Keys. It can be None, and then KEY is not encrypted.
    Returns:
        hex string: return a hex string containing extracted and decrypted private KEY from given .der file.
    """
    try:
        with open(der_file, 'rb') as file:
            key_data = file.read()
            if password is None:
                logging.warning("KEY password has not been provided. It means that DAC key is not encrypted.")
            keys = load_der_private_key(key_data, password, backend=default_backend())
            private_key = keys.private_numbers().private_value.to_bytes(32, byteorder='big')
            print(private_key)

            return private_key

    except IOError or ValueError:
        return None


# check_str_range: for validate_args
def check_str_range(s, min_len, max_len, name):
    if s and ((len(s) < min_len) or (len(s) > max_len)):
        logging.error('%s must be between %d and %d characters', name, min_len, max_len)
        sys.exit(1)


# check_int_range: for validate_args
def check_int_range(value, min_value, max_value, name):
    if value and ((value < min_value) or (value > max_value)):
        logging.error('%s is out of range, should be in range [%d, %d]', name, min_value, max_value)
        sys.exit(1)


# validate_args: check if all the arguments are valid
def validate_args(args):
    # Validate the passcode
    if args.passcode is not None:
        if ((args.passcode < 0x0000001 and args.passcode > 0x5F5E0FE) or (args.passcode in INVALID_PASSCODES)):
            logging.error('Invalid passcode:' + str(args.passcode))
            sys.exit(1)

    check_int_range(args.discriminator, 0x0000, 0x0FFF, 'Discriminator')
    check_int_range(args.product_id, 0x0000, 0xFFFF, 'Product id')
    check_int_range(args.vendor_id, 0x0000, 0xFFFF, 'Vendor id')
    check_int_range(args.hw_ver, 0x0000, 0xFFFF, 'Hardware version')

    check_str_range(args.serial_num, 1, 32, 'Serial number')
    check_str_range(args.vendor_name, 1, 32, 'Vendor name')
    check_str_range(args.product_name, 1, 32, 'Product name')
    check_str_range(args.hw_ver_str, 1, 64, 'Hardware version string')
    check_str_range(args.mfg_date, 8, 16, 'Manufacturing date')
    check_str_range(args.rd_id_uid, 32, 32, 'Rotating device Unique id')

    logging.info('Discriminator:{} Passcode:{}'.format(args.discriminator, args.passcode))


# gen_spake2p_params: generate spake2p parameters
def gen_spake2p_params(args):
    # TODO: pass in as arguments
    passcode = args.passcode
    spake2p_bin = args.spake2p_path
    iter_count_max = 1000
    salt_len_max = 24

    cmd = [
        spake2p_bin, 'gen-verifier',
        '--iteration-count', str(iter_count_max),
        '--salt-len', str(salt_len_max),
        '--pin-code', str(passcode),
        '--out', '-',
    ]

    output = subprocess.check_output(cmd)
    output = output.decode('UTF-8').splitlines()
    return dict(zip(output[0].split(','), output[1].split(',')))


# populate_factory_data: populate data into protobuf format to be serialized
def populate_factory_data(args, spake2p_params):
    # from test import CD, DAC, DACPubKey, PAI, DACPriKey
    FACTORY_DATA.cdata.passcode = args.passcode
    FACTORY_DATA.cdata.discriminator = args.discriminator
    FACTORY_DATA.cdata.spake2_it = int(spake2p_params['Iteration Count'])
    FACTORY_DATA.cdata.spake2_salt.value = spake2p_params['Salt'].encode('UTF-8')
    FACTORY_DATA.cdata.spake2_salt.length = len(spake2p_params['Salt'])
    FACTORY_DATA.cdata.spake2_verifier.value = spake2p_params['Verifier'].encode('UTF-8')
    FACTORY_DATA.cdata.spake2_verifier.length = len(spake2p_params['Verifier'])
    f = open(os.path.abspath(args.dac_cert), 'rb')
    temp_data = f.read()
    FACTORY_DATA.dac.dac_cert.value = temp_data
    FACTORY_DATA.dac.dac_cert.length = len(temp_data)
    # FACTORY_DATA.dac.dac_cert.value = bytes(DAC)
    # FACTORY_DATA.dac.dac_cert.length = len(DAC)
    f.close()

    f = open(os.path.abspath(args.pai_cert), 'rb')
    temp_data = f.read()
    FACTORY_DATA.dac.pai_cert.value = temp_data
    FACTORY_DATA.dac.pai_cert.length = len(temp_data)
    # FACTORY_DATA.dac.pai_cert.value = bytes(PAI)
    # FACTORY_DATA.dac.pai_cert.length = len(PAI)
    f.close()

    f = open(os.path.abspath(args.cd), 'rb')
    temp_data = f.read()
    FACTORY_DATA.dac.cd.value = temp_data
    FACTORY_DATA.dac.cd.length = len(temp_data)
    # FACTORY_DATA.dac.cd.value = bytes(CD)
    # FACTORY_DATA.dac.cd.length = len(CD)
    f.close()

    # split up dac-key files into pub and priv keys, extract only the priv keys
    dac_priv_key = get_raw_private_key_der(os.path.abspath(args.dac_key), None)  # password set as None first
    if dac_priv_key is None:
        logging.error("Cannot read DAC keys from : {}".format(args.dac_key))
        sys.exit(-1)
    FACTORY_DATA.dac.dac_key.value = dac_priv_key
    FACTORY_DATA.dac.dac_key.length = len(dac_priv_key)
    # FACTORY_DATA.dac.dac_key.value = bytes(DACPriKey)
    # FACTORY_DATA.dac.dac_key.length = len(DACPriKey)

    if args.vendor_id is not None:
        FACTORY_DATA.dii.vendor_id = args.vendor_id
    if args.vendor_name is not None:
        FACTORY_DATA.dii.vendor_name.value = args.vendor_name.encode('UTF-8')
        FACTORY_DATA.dii.vendor_name.length = len(args.vendor_name)
    if args.product_id is not None:
        FACTORY_DATA.dii.product_id = args.product_id
    if args.product_name is not None:
        FACTORY_DATA.dii.product_name.value = args.product_name.encode('UTF-8')
        FACTORY_DATA.dii.product_name.length = len(args.product_name)
    if args.hw_ver is not None:
        FACTORY_DATA.dii.hw_ver = args.hw_ver
    if (args.hw_ver_str is not None):
        FACTORY_DATA.dii.hw_ver_string.value = args.hw_ver_str.encode('UTF-+8')
        FACTORY_DATA.dii.hw_ver_string.length = len(args.hw_ver_str)
    if args.mfg_date is not None:
        FACTORY_DATA.dii.mfg_date.value = args.mfg_date.encode('UTF-8')
        FACTORY_DATA.dii.mfg_date.length = len(args.mfg_date)
    if args.serial_num is not None:
        FACTORY_DATA.dii.serial_num.value = args.serial_num.encode('UTF-8')
        FACTORY_DATA.dii.serial_num.length = len(args.serial_num)
    if args.rd_id_uid is not None:
        from math import ceil
        FACTORY_DATA.dii.rd_id_uid.length = ceil(int(args.rd_id_uid, 16).bit_length() / 8) + 1
        FACTORY_DATA.dii.rd_id_uid.value = int(args.rd_id_uid, 16).to_bytes(FACTORY_DATA.dii.rd_id_uid.length, "little")


# main: main program
def main():
    def any_base_int(s):
        return int(s, 0)

    parser = argparse.ArgumentParser(description='Ameba Factory NVS Data generator tool')

    parser.add_argument('--spake2p_path', type=str, required=True,
                        help='The path to the spake2p binary')
    # These will be used by CommissionableDataProvider
    parser.add_argument('-p', '--passcode', type=any_base_int, required=True,
                        help='The setup passcode for pairing, range: 0x01-0x5F5E0FE')
    parser.add_argument('-d', '--discriminator', type=any_base_int, required=True,
                        help='The discriminator for pairing, range: 0x00-0x0FFF')
    # parser.add_argument("--spake2_it", type=allow_any_int, required=True,
    #                     help="[int | hex int] Provide Spake2+ iteration count.")
    # parser.add_argument("--spake2_salt", type=base64_str, required=True,
    #                     help="[base64 string] Provide Spake2+ salt.")
    # parser.add_argument("--spake2_verifier", type=base64_str,
    #                     help="[base64 string] Provide Spake2+ verifier without generating it.")

    # These will be used by DeviceAttestationCredentialsProvider
    parser.add_argument('--dac_cert', type=str, required=True,
                        help='The path to the DAC certificate in der format')
    parser.add_argument('--dac_key', type=str, required=True,
                        help='The path to the DAC private key in der format')
    parser.add_argument('--pai_cert', type=str, required=True,
                        help='The path to the PAI certificate in der format')
    parser.add_argument('--cd', type=str, required=True,
                        help='The path to the certificate declaration der format')

    # These will be used by DeviceInstanceInfoProvider
    parser.add_argument('--out', type=str, required=False, default="factory_data.bin", help='Vendor id')
    parser.add_argument('--vendor-id', type=any_base_int, required=False, help='Vendor id')
    parser.add_argument('--vendor-name', type=str, required=False, help='Vendor name')
    parser.add_argument('--product-id', type=any_base_int, required=False, help='Product id')
    parser.add_argument('--product-name', type=str, required=False, help='Product name')
    parser.add_argument('--hw-ver', type=any_base_int, required=False, help='Hardware version')
    parser.add_argument('--hw-ver-str', type=str, required=False, help='Hardware version string')
    parser.add_argument('--mfg-date', type=str, required=False, help='Manufacturing date in format YYYY-MM-DD')
    parser.add_argument('--serial-num', type=str, required=False, help='Serial number')
    parser.add_argument('--rd-id-uid', type=str, required=False,
                        help='128-bit unique identifier for generating rotating device identifier, provide 32-byte hex string, e.g. "1234567890abcdef1234567890abcdef"')

    # These will be used by DeviceInfoProvider
    # parser.add_argument('--calendar-types', type=str, nargs='+', required=False,
    #                     help='List of supported calendar types.\nSupported Calendar Types: Buddhist, Chinese, Coptic, Ethiopian, Gregorian, Hebrew, Indian, Islamic, Japanese, Korean, Persian, Taiwanese')
    # parser.add_argument('--locales', type=str, nargs='+', required=False,
    #                     help='List of supported locales, Language Tag as defined by BCP47, eg. en-US en-GB')
    # parser.add_argument('--fixed-labels', type=str, nargs='+', required=False,
    #                     help='List of fixed labels, eg: "0/orientation/up" "1/orientation/down" "2/orientation/down"')
    #
    # parser.add_argument('-s', '--size', type=any_base_int, required=False, default=0x6000,
    #                     help='The size of the partition.bin, default: 0x6000')
    # parser.add_argument('-e', '--encrypt', action='store_true', required=False,
    #                     help='Encrypt the factory parititon NVS binary')

    # additional arguments for factorydata encryption
    parser.add_argument('--factorydata-key', type=str, required=False, help='32-bytes key to encrypt factorydata')
    parser.add_argument('--factorydata-iv', type=str, required=False, help='16-bytes iv to encrypt factorydata')

    parser.add_argument('--gen_qrcode', default='True', help='Generate QR code for onboarding payload')
    parser.add_argument('--gen_manual_pairing_code', default='True', help='Generate manual pairing code for onboarding payload')

    global args
    args = parser.parse_args()
    validate_args(args)
    spake2p_params = gen_spake2p_params(args)
    setup_payload = SetupPayload(discriminator=args.discriminator,
                                pincode=args.passcode,
                                rendezvous=2,  # fixed pairing BLE
                                flow=CommissioningFlow.Standard,
                                vid=args.vendor_id,
                                pid=args.product_id)
    
    
    with open_wrap("manual_pairing_code.txt", "w") as manual_code_file:
        manual_code_file.write("Manualcode: " + setup_payload.generate_manualcode() + "\n")

    with open_wrap("qrcode.txt", "w") as qrcode_file:
        qrcode_file.write("QRCode: " + setup_payload.generate_qrcode())
    qr = qrcode.make(setup_payload.generate_qrcode())
    qr.save(os.path.join(args.out, "qrcode.png"))

    populate_factory_data(args, spake2p_params)


    # write the factorydata pbuf (with or without encryption) from temp file to factory_data.bin file
    # we want to prepend the data length (2bytes) to the factory_data.bin file in big endian format, the data length is not encrypted
    data_to_write = FACTORY_DATA.SerializeToString()
    data_len = len(data_to_write)
      
    total_data_len = data_len + 2
    img_len = align_16(total_data_len)
    extra_len = img_len - total_data_len
    
    f2 = open_wrap("factory_data.bin", "wb")
    f2.write(data_len.to_bytes(2, 'little') + data_to_write)
    if extra_len > 0:
        f2.write(b"\xf5" * extra_len)

    f2.close()

    print(FACTORY_DATA.ListFields())
    print("Total factorydata size: {}".format(len(FACTORY_DATA.SerializeToString())))


if __name__ == "__main__":
    main()
# python factory_data.py -p 20202021 -d 3840 --spake2p_path=./spake2p --vendor-id 0xFFF1 --vendor-name "TEST_VENDOR" --product-id 0x8001 --product-name "TEST_PRODUCT" --serial-num "TEST_SN" --hw-ver 1 --hw-ver-str "1.0" --rd-id-uid 00112233445566778899aabbccddeeff
#--factorydata-key ff0102030405060708090a0b0c0d0e0fff0102030405060708090a0b0c0d0e0f --factorydata-iv ff0102030405060708090a0b0c0d0e0f


# python3 factory_data.py -p 20202021 -d 3840 --spake2p_path=../test/spake2p --vendor-id 0xFFF1 --vendor-name "TEST_VENDOR" --product-id 0x0123 --product-name "TEST_PRODUCT" --serial-num "TEST_SN" --hw-ver 1 --hw-ver-str "1.0" --rd-id-uid 00112233445566778899aabbccddeeff --dac_cert ../test/Chip-DAC-Cert.der --dac_key ../test/Chip-DAC-Key.der --pai_cert ../test/Chip-PAI-Cert.der --cd ../test/CD.der


# --factorydata-key ff0102030405060708090a0b0c0d0e0fff0102030405060708090a0b0c0d0e0f --factorydata-iv ff0102030405060708090a0b0c0d0e0f

