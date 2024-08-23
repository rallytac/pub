#!/bin/bash

systemctl enable nsm
systemctl start nsm

if [ $? != "0" ]; then
    exit 1
fi

echo "NSM has been installed and started.  To change configuration, edit /etc/nsm/nsm_conf.json and restart the service."

