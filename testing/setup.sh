#!/bin/bash

function show_usage()
{
    echo "usage: setup <platform> <binary_version>"    
}

PLATFORM=${1}
VERSION=${2}
GITHUB_BASE=https://github.com/rallytac/pub/raw/master

if [[ "${PLATFORM}" == "" ]]; then
    show_usage
    exit 1
fi

if [[ "${VERSION}" == "" ]]; then
    show_usage
    exit 1
fi

# Certificates
wget -O rtsCA.pem ${GITHUB_BASE}/certificates/rtsCA.pem
wget -O rtsFactoryDefaultEngage.pem ${GITHUB_BASE}/certificates/rtsFactoryDefaultEngage.pem
wget -O rtsFactoryDefaultEngage.key ${GITHUB_BASE}/certificates/rtsFactoryDefaultEngage.key

# Configurations
wget -O sample_engine_policy.json ${GITHUB_BASE}/configurations/sample_engine_policy.json
sed -i 's/@..\/certificates\//@.\//g' sample_engine_policy.json
wget -O sample_mission_template.json ${GITHUB_BASE}/configurations/sample_mission_template.json
sed -i 's/@..\/certificates\//@.\//g' sample_mission_template.json

# Binaries
wget -O engage-cmd ${GITHUB_BASE}/bin/${VERSION}/${PLATFORM}/engage-cmd
chmod +x engage-cmd
wget -O libengage-shared.so ${GITHUB_BASE}/bin/${VERSION}/${PLATFORM}/libengage-shared.so
chmod +x libengage-shared.so

echo "Dont forget to run 'export LD_LIBRARY_PATH=./' in your terminal!"
