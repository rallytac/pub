#!/bin/bash

# To run in test mode for a random number of seconds with constant restart
# set NSM_RUN_SECS to a positive number

while [ true ]
do
    if [[ "${NSM_RUN_SECS}" == "" ]]; then
        ./onIdle.sh ''
        python3 _nsm.py ${1} ${2} ${3} ${4} ${5} ${6}
        ./onIdle.sh ''
        break
    else
        ./onIdle.sh ''
        python3 _nsm.py --max-run-secs $(( ($RANDOM % ${NSM_RUN_SECS}) + ${NSM_RUN_SECS} )) ${1} ${2} ${3} ${4} ${5} ${6}
        ./onIdle.sh ''
    fi
done
