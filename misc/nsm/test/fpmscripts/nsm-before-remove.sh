#!/bin/bash

systemctl stop nsm 2> /dev/null
systemctl disable nsm 2> /dev/null

# Make sure we don't exit with an error
exit 0
