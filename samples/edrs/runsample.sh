#!/bin/bash

TAG=${1}
MIMETYPE=
CMDEXTRA=

if [ "${TAG}" == "ebs.status" ]; then
        FN=./sampledata/sampleebs01_status.json
        MIMETYPE="application/json"
elif [ "${TAG}" == "ebs.config" ]; then
        CMDEXTRA="-filets"
        FN=./sampledata/engagebridged_conf.json
        MIMETYPE="application/json"
elif [ "${TAG}" == "rp.config" ]; then
        CMDEXTRA="-filets"
        FN=./sampledata/rallypointd_conf.json
        MIMETYPE="application/json"
else
        echo "usage: ${0} <ebs.status | ebs.config | rp.config>"
        exit
fi

python edrs_agent.py \
        -f:${FN} \
        -cert:./EdrsClient.cert \
        -key:./EdrsClient.key \
        -url:https://127.0.0.1:6767/v1/archive/json \
        -api:d0497292d40f413a970fe08d8ef30985ea9fe3aa40b2449a819c807ae7d979ff \
        -int:1 \
        -verbose \
        -insecure \
        -tag:${TAG} \
        -mt:${MIMETYPE} \
        -instance:ebs01 \
        -node:abcdef ${CMDEXTRA}
