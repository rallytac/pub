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
elif [ "${TAG}" == "dot.r1" ]; then
        TAG="dot"
        CMDEXTRA="-filets"
        FN=./sampledata/r1_links.dot
        MIMETYPE="text/vnd.graphviz"
elif [ "${TAG}" == "dot.r2" ]; then
        TAG="dot"
        CMDEXTRA="-filets"
        FN=./sampledata/r2_links.dot
        MIMETYPE="text/vnd.graphviz"
elif [ "${TAG}" == "dot.r3" ]; then
        TAG="dot"
        CMDEXTRA="-filets"
        FN=./sampledata/r3_links.dot
        MIMETYPE="text/vnd.graphviz"
elif [ "${TAG}" == "dot.r4" ]; then
        TAG="dot"
        CMDEXTRA="-filets"
        FN=./sampledata/r4_links.dot
        MIMETYPE="text/vnd.graphviz"
elif [ "${TAG}" == "dot.r5" ]; then
        TAG="dot"
        CMDEXTRA="-filets"
        FN=./sampledata/r5_links.dot
        MIMETYPE="text/vnd.graphviz"
elif [ "${TAG}" == "dog" ]; then
        CMDEXTRA="-filets"
        FN=./sampledata/black_dog.png
        MIMETYPE="image/png"
else
        echo "usage: ${0} <ebs.status | ebs.config | rp.config>"
        exit
fi

python edrs_agent.py \
        -f:${FN} \
        -cert:./EdrsClient.cert \
        -key:./EdrsClient.key \
        -url:https://127.0.0.1:6767/v1/archive/doc \
        -api:d0497292d40f413a970fe08d8ef30985ea9fe3aa40b2449a819c807ae7d979ff \
        -int:1 \
        -verbose \
        -insecure \
        -tag:${TAG} \
        -mt:${MIMETYPE} \
        -instance:ebs01 \
        -node:abcdef ${CMDEXTRA}
