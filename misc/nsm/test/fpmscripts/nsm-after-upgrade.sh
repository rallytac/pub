#!/bin/bash

NSM_DIR="/etc/nsm"
BACKUP_DIR="${NSM_DIR}/backup"
CONF_FILE="nsm_conf.json"

if [ -f "${BACKUP_DIR}/${CONF_FILE}" ]; then
    mv "${NSM_DIR}/${CONF_FILE}" "${NSM_DIR}/${CONF_FILE}.new"
    echo "*********************************************************************************************************"
    echo "* Retained previous configuration file '${NSM_DIR}/${CONF_FILE}'.  New, unused, file is '${NSM_DIR}/${CONF_FILE}.new'"
    echo "*********************************************************************************************************"
    cp "${BACKUP_DIR}/${CONF_FILE}" "${NSM_DIR}"
fi

rm -rf "${BACKUP_DIR}"

# Make sure we don't exit with an error
exit 0
