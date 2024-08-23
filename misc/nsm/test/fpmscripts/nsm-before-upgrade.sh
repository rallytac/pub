#!/bin/bash

systemctl stop nsm 2> /dev/null
systemctl disable nsm 2> /dev/null

NSM_DIR="/etc/nsm"
BACKUP_DIR="${NSM_DIR}/backup"
CONF_FILE="nsm_conf.json"

mkdir -p "${BACKUP_DIR}"

cp "${NSM_DIR}/${CONF_FILE}" "${BACKUP_DIR}"

# Make sure we don't exit with an error
exit 0
