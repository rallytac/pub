from cryptography.fernet import Fernet
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes, padding
from cryptography.hazmat.backends import _get_backend
import base64
import hashlib
import os
import binascii

def printBytes(msg, bytes):
    s = binascii.hexlify(bytearray(bytes))
    print(msg + ':' + str(s))

def makeKey(pwd):
    return hashlib.pbkdf2_hmac('sha256', str.encode(pwd), b'04DBAA5900D5421D9CCB25A54ED4FA56', 10000, 32)

def fernetEncrypt(key, clearBytes):
    cryptoInstance = Fernet(base64.urlsafe_b64encode(key))
    rc = cryptoInstance.encrypt(clearBytes)
    return rc

def fernetDecrypt(key, encryptedBytes):
    cryptoInstance = Fernet(base64.urlsafe_b64encode(key))
    rc = cryptoInstance.decrypt(encryptedBytes)
    return rc

def aesEncrypt(key, clearBytes):
    lData = len(clearBytes).to_bytes(2, 'big')
    data = lData + clearBytes
    iv = os.urandom(16)

    padder = padding.PKCS7(algorithms.AES.block_size).padder()
    paddedData = padder.update(data) + padder.finalize()
    backend = default_backend()
    cipher = Cipher(algorithms.AES(key), modes.CBC(iv), backend)
    rc = iv
    rc = rc + cipher.encryptor().update(paddedData)
    rc = rc + cipher.encryptor().finalize()
    return rc

def aesDecrypt(key, encryptedBytes):
    iv = encryptedBytes[:16]
    backend = default_backend()
    cipher = Cipher(algorithms.AES(key), modes.CBC(iv), backend)
    tmp = cipher.decryptor().update(encryptedBytes[16:])
    tmp = tmp + cipher.decryptor().finalize()
    rc = tmp[2:int.from_bytes(tmp[:2], 'big') + 2]

    return rc

orig = 'Shaun Botha'
pwd = 'this is just a password'

fE = fernetEncrypt(makeKey(pwd), orig.encode())
fD = fernetDecrypt(makeKey(pwd), fE).decode()

sE = base64.urlsafe_b64encode(aesEncrypt(makeKey(pwd), orig.encode())).decode('utf-8')
sD = aesDecrypt(makeKey(pwd), base64.urlsafe_b64decode(sE)).decode()
print(sD)

finalEncryptedData = base64.urlsafe_b64encode(sE).decode('utf-8')
print(finalEncryptedData)



#if(sD == orig):
#    print('>>>>>>>>>>> shaun crypto worked!!!')
#else:
#    print('>>>>>>>>>>> shaun crypto failed')

#print('fernet=' + str(len(fE)))
#print('shaun=' + str(len(sE)))

