#
# Engage Provisioning Tool
# Copyright (c) 2022 Rally Tactical Systems, Inc.
#

from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes, padding
import base64
import hashlib
import os
import sys
import json
import argparse

EP_HEADER = 'Engage Provisioning|'
KEY_SALT = b'04DBAA5900D5421D9CCB25A54ED4FA56'
BYTE_ORDER = 'big'
STR_ENCODING = 'utf-8'
ADS_BYTES = 4
IV_BYTES=16


# --------------------------------------------------------------------------
def makeKey(pwd):
    return hashlib.pbkdf2_hmac('sha256', str.encode(pwd), KEY_SALT, 10000, 32)


# --------------------------------------------------------------------------
def aesEncrypt(key, clearBytes):
    lData = len(clearBytes).to_bytes(ADS_BYTES, BYTE_ORDER)
    data = lData + clearBytes
    iv = os.urandom(IV_BYTES)

    padder = padding.PKCS7(algorithms.AES.block_size).padder()
    paddedData = padder.update(data) + padder.finalize()
    backend = default_backend()
    cipher = Cipher(algorithms.AES(key), modes.CBC(iv), backend)
    rc = iv
    rc = rc + cipher.encryptor().update(paddedData)
    rc = rc + cipher.encryptor().finalize()
    return rc


# --------------------------------------------------------------------------
def aesDecrypt(key, encryptedBytes):
    iv = encryptedBytes[:IV_BYTES]
    backend = default_backend()
    cipher = Cipher(algorithms.AES(key), modes.CBC(iv), backend)
    tmp = cipher.decryptor().update(encryptedBytes[IV_BYTES:])
    tmp = tmp + cipher.decryptor().finalize()
    rc = tmp[ADS_BYTES:int.from_bytes(tmp[:ADS_BYTES], 'big') + ADS_BYTES]
    return rc


# --------------------------------------------------------------------------
def encryptToBase64String(pwd, input):
    return base64.b64encode(aesEncrypt(makeKey(pwd), input)).decode(STR_ENCODING)


# --------------------------------------------------------------------------
def decryptFromBase64String(pwd, input):
    return aesDecrypt(makeKey(pwd), base64.b64decode(input)).decode()


# --------------------------------------------------------------------------
if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    try:
        parser.add_argument('--pack', action='store_true')
        parser.add_argument('--no-pack', dest='pack', action='store_false')
        parser.set_defaults(pack=True)

        parser.add_argument('-pwd', required=True, metavar='password', help='password for provisioning file')
        parser.add_argument('-i', required=True, metavar='input_file', help='input file')
        parser.add_argument('-o', required=True, metavar='output_file', help='output file')

        args = parser.parse_args()

    except:
        parser.print_help()
        exit()

    if args.pack:
        with open(args.i, 'rb') as f:
            inputJson = json.load(f)

        if not 'engageProvisioning' in inputJson:
            print(args.i + ' is not a valid inout file')
            exit()

        if 'policies' in inputJson['engageProvisioning']:
            for x in range(0, len(inputJson['engageProvisioning']['policies'])):
                s = inputJson['engageProvisioning']['policies'][x]
                if s.startswith('@file://'):
                    with open(s[8:], 'rt') as f:
                        inputJson['engageProvisioning']['policies'][x] = {'file': os.path.basename(f.name), 'content': json.load(f)}

        if 'identities' in inputJson['engageProvisioning']:
            for x in range(0, len(inputJson['engageProvisioning']['identities'])):
                s = inputJson['engageProvisioning']['identities'][x]
                if s.startswith('@file://'):
                    with open(s[8:], 'rt') as f:
                        inputJson['engageProvisioning']['identities'][x] = {'file': os.path.basename(f.name), 'content': json.load(f)}

        if 'missions' in inputJson['engageProvisioning']:
            for x in range(0, len(inputJson['engageProvisioning']['missions'])):
                s = inputJson['engageProvisioning']['missions'][x]
                if s.startswith('@file://'):
                    with open(s[8:], 'rt') as f:
                        inputJson['engageProvisioning']['missions'][x] = {'file': os.path.basename(f.name), 'content': json.load(f)}

        if 'certStores' in inputJson['engageProvisioning']:
            for x in range(0, len(inputJson['engageProvisioning']['certStores'])):
                s = inputJson['engageProvisioning']['certStores'][x]
                if s.startswith('@file://'):
                    with open(s[8:], 'rb') as f:
                        csContent = f.read(-1)
                        csContent = base64.b64encode(csContent).decode(STR_ENCODING)
                        inputJson['engageProvisioning']['certStores'][x] = {'file': os.path.basename(f.name), 'content': csContent}
        
        inputData = json.dumps(inputJson, separators=(',', ':')).encode(STR_ENCODING)
        outputData = EP_HEADER + encryptToBase64String(args.pwd, inputData)       

    else:
        with open(args.i, 'rb') as f:
            inputData = f.read(-1)

        testString = inputData.decode(STR_ENCODING)
        if not testString.startswith(EP_HEADER):
            print(args.i + ' is not an Engage Provisioning Profile')
            exit()

        inputData = inputData[len(EP_HEADER):]
        outputData = decryptFromBase64String(args.pwd, inputData)

    with open(args.o, 'wb') as f:
        f.write(outputData.encode(STR_ENCODING))