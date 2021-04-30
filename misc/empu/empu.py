#
# Copyright (c) 2020 Rally Tactical Systems, Inc.
# All rights reserved
#

import sys
import struct
import zlib

QR_CODE_HEADER = "&*3$e1@E"
QR_VERSION = "001"
QR_DEFLECTION_URL_SEP = "/??"
QR_CHARSET = "utf-8"

base91_alphabet = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '!', '#', '$',
	'%', '&', '(', ')', '*', '+', ',', '.', '/', ':', ';', '<', '=',
	'>', '?', '@', '[', ']', '^', '_', '`', '{', '|', '}', '~', '"']

decode_table = dict((v,k) for k,v in enumerate(base91_alphabet))

def decodeFromBase91(encoded_str):
    v = -1
    b = 0
    n = 0
    out = bytearray()
    for strletter in encoded_str:
        if not strletter in decode_table:
            continue
        c = decode_table[strletter]
        if(v < 0):
            v = c
        else:
            v += c*91
            b |= v << n
            n += 13 if (v & 8191)>88 else 14
            while True:
                out += struct.pack('B', b&255)
                b >>= 8
                n -= 8
                if not n>7:
                    break
            v = -1
    if v+1:
        out += struct.pack('B', (b | v << n) & 255 )
    return out


def encodeToBase91(bindata):
    b = 0
    n = 0
    out = ''
    for count in range(len(bindata)):
        byte = bindata[count:count+1]
        b |= struct.unpack('B', byte)[0] << n
        n += 8
        if n>13:
            v = b & 8191
            if v > 88:
                b >>= 13
                n -= 13
            else:
                v = b & 16383
                b >>= 14
                n -= 14
            out += base91_alphabet[v % 91] + base91_alphabet[v // 91]
    if n:
        out += base91_alphabet[b % 91]
        if n>7 or b>90:
            out += base91_alphabet[b // 91]
    return out


def pack(jsonText):
    qrText = QR_CODE_HEADER + QR_VERSION + jsonText
    compressor = zlib.compressobj(9, zlib.DEFLATED, zlib.MAX_WBITS | 16)
    compressedBytes = compressor.compress(qrText.encode(QR_CHARSET)) + compressor.flush()
    base91Text = encodeToBase91(compressedBytes)
    
    return base91Text


def unpack(packedText):
    position = packedText.find("/??")
    if position >= 0:
        base91Text = packedText[position + 3:]
    else:
        base91Text = packedText

    compressedBytes = decodeFromBase91(base91Text)
    decompressedBytes = zlib.decompress(compressedBytes, 16 + zlib.MAX_WBITS)
    baselineString = decompressedBytes.decode(QR_CHARSET)

    headerPosition = baselineString.find(QR_CODE_HEADER)
    if headerPosition < 0:
        print("ERROR: Invalid header signature")
        exit()

    version = baselineString[headerPosition + 8 : headerPosition + 8 + 3]    
    if version != QR_VERSION:
        print("ERROR: Unsupported version")
        exit()

    jsonText = baselineString[headerPosition + 8 + 3:]

    return jsonText


def packFile(fn):
    jsonText = unpack(open(fn, "r").read())
    print(jsonText)


def unpackFile(fn):
    base91Text = pack(open(fn, "r").read())
    print(base91Text)    


def showHelp():
    msg = (
        "--------------------------------------------------------------------------------\n"
        "Engage Mission Packer/Unpacker version 0.2\n"
        "Copyright (c) 2020 Rally Tactical Systems, Inc.\n"
        "\n"
        "This utility packs or unpacks Engage mission files, outputting the result to the\n"
        "standard output device (like a console or terminal window).\n"
        "\n"
        "For example: to pack an input JSON file 'mymission.json' and place the output in\n"
        "'mymission.mission, do the following:\n"
        "\n"
        "    $ python empu pack mymision.json > mymission.mission\n"
        "\n"
        "To go the other way - i.e. unpack the file back to the original JSON text and\n"
        "display the ouput\n"
        "\n"
        "    $ python empu unpack mymision.mission\n"
        "\n"
        "--------------------------------------------------------------------------------\n"
        "\n"
    )
    print(msg)

# ------------------------ MAIN -------------------------------
if len(sys.argv) == 3:
    if sys.argv[1] == "pack":
        unpackFile(sys.argv[2])
    elif sys.argv[1] == "unpack":
        packFile(sys.argv[2])
    else:
        showHelp()
else:
    showHelp()
