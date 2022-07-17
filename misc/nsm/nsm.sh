#!/bin/bash
while [ true ]
do
    ./onIdle.sh ''
    python3 nsm.py --max-run-secs $(( $RANDOM % 30 + 30 )) --log-level 4 ${1} ${2} ${3} ${4} ${5} ${6}
    ./onIdle.sh ''
done