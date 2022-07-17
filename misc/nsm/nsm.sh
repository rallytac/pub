#!/bin/bash

# To run in test mode for a random number of seconds with constant restart
# set NSM_MAX_RUN_SECS to a positive number

while [ true ]
do
    if [[ "${NSM_MAX_RUN_SECS}" == "" ]]; then
        ./onIdle.sh ''
        python3 nsm.py --log-level 4 ${1} ${2} ${3} ${4} ${5} ${6}
        ./onIdle.sh ''
        break
    else
        ./onIdle.sh ''
        python3 nsm.py --max-run-secs $(( $RANDOM % ${NSM_MAX_RUN_SECS} + ${NSM_MAX_RUN_SECS} )) --log-level 4 ${1} ${2} ${3} ${4} ${5} ${6}
        ./onIdle.sh ''
    fi
done
