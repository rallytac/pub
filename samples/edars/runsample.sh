#!/bin/bash

TYPE=${1}
CMDEXTRA=

if [ "${TYPE}" == "ebs.status" ]; then
        FN=./sampledata/sampleebs01_status.json
elif [ "${TYPE}" == "ebs.config" ]; then
        CMDEXTRA="-filets"
        FN=./sampledata/engagebridged_conf.json
elif [ "${TYPE}" == "rp.config" ]; then
        CMDEXTRA="-filets"
        FN=./sampledata/rallypointd_conf.json
else
        echo "usage: ${0} <ebs.status | ebs.config | rp.config>"
        exit
fi

python ejars_agent.py \
        -jf:${FN} \
        -cert:./EjarsRestClient.cert \
        -key:./EjarsRestClient.key \
        -url:https://127.0.0.1:6767/v1/archive/json \
        -api:d0497292d40f413a970fe08d8ef30985ea9fe3aa40b2449a819c807ae7d979ff \
        -int:1 \
        -verbose \
        -insecure \
        -type:${TYPE} \
        -instance:ebs01 \
        -node:abcdef ${CMDEXTRA}
