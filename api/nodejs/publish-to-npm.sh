#!/bin/bash

echo "================================================================================"
echo "Engage NPM Publisher"
echo "Copyright (c) 2019 Rally Tactical Systems, Inc."
echo "================================================================================"

THIS_ROOT=`pwd`
BUILD_ROOT=${THIS_ROOT}/.build
API_ROOT=${THIS_ROOT}/../
BIN_ROOT=${THIS_ROOT}/../../.build/debug
REDIST_ROOT=${THIS_ROOT}/../../.cache/redist
WINDOWS_REDIST_ROOT=${THIS_ROOT}/../../.cache/redist/windows
WINDOWS_REDIST_URL=https://github.com/rallytac/etc/raw/master/windows/redist
SRC_BIN_VERSION="${1}"
VERSION_EXTENSION="${2}"

if [ "${SRC_BIN_VERSION}" == "" -o "${VERSION_EXTENSION}" == "" ]; then
    echo "usage ${0} src_version pub_extension"
    exit 1
fi

#function determine_src_bin_version()
#{
#    FILES=(`ls -r ${BIN_ROOT}`)
#    SRC_BIN_VERSION=${FILES[0]}
#}

function get_microsoft_redistributables()
{
    function check_for_ms_file()
    {
        PLAT=${1}
        FN=${2}

        if [ ! -f "${WINDOWS_REDIST_ROOT}/${PLAT}/${FN}" ]; then
            wget "${WINDOWS_REDIST_URL}/${PLAT}/${FN}" -O "${WINDOWS_REDIST_ROOT}/${PLAT}/${FN}" > /dev/null
            if [[ $? != "0" ]]; then
                echo "ERROR: Failed to download Windows redistributable ${PLAT}/${FN}"
                exit 1
            fi
        fi
    }
    
    mkdir -p "${WINDOWS_REDIST_ROOT}/x64"
    mkdir -p "${WINDOWS_REDIST_ROOT}/ia32"

    FILES=("concrt140.dll" "msvcp140.dll" "msvcp140_1.dll" "msvcp140_2.dll" "vccorlib140.dll" "vcruntime140.dll")

    for f in ${FILES[@]}; do
        check_for_ms_file "x64" "${f}"
        check_for_ms_file "ia32" "${f}"
    done
}

function publish_it()
{
    get_microsoft_redistributables

    CURRDIR=`pwd`

    rm -rf ${BUILD_ROOT}
    mkdir ${BUILD_ROOT}
    cd ${BUILD_ROOT}

    cp ../binding.gyp .
    cp ../engage.cpp .
    cp ../index.js .
    cp ../package.json .
    cp ../README.md .    

    mkdir include
    cp ${API_ROOT}/c/include/* include

    mkdir -p lib/darwin.x64
    cp ${BIN_ROOT}/${SRC_BIN_VERSION}/darwin_x64/libengage-shared.dylib lib/darwin.x64

    mkdir -p lib/linux.x64
    cp ${BIN_ROOT}/${SRC_BIN_VERSION}/linux_x64/libengage-shared.so lib/linux.x64

    mkdir -p lib/win32.x64
    cp ${BIN_ROOT}/${SRC_BIN_VERSION}/win_x64/engage-shared.dll lib/win32.x64
    cp ${BIN_ROOT}/${SRC_BIN_VERSION}/win_x64/engage-shared.lib lib/win32.x64
    cp -r "${WINDOWS_REDIST_ROOT}/x64/"* lib/win32.x64

    mkdir -p lib/win32.ia32
    cp ${BIN_ROOT}/${SRC_BIN_VERSION}/win_ia32/engage-shared.dll lib/win32.ia32
    cp ${BIN_ROOT}/${SRC_BIN_VERSION}/win_ia32/engage-shared.lib lib/win32.ia32
    cp -r "${WINDOWS_REDIST_ROOT}/ia32/"* lib/win32.ia32

    npm version ${SRC_BIN_VERSION}${VERSION_EXTENSION}
    #npm publish

    cd ${CURRDIR}
    #rm -rf ${BUILD_ROOT}
}

#determine_SRC_bin_version
echo "Publishing for version ${SRC_BIN_VERSION}${VERSION_EXTENSION} ..."

publish_it
