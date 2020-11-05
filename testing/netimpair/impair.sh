#!/bin/bash

echo "================================================================================"
echo "Linux Network Impairment Tool"
echo "Copyright (c) 2019 Rally Tactical Systems, Inc."
echo "================================================================================"

INTF=${1}

function show_help()
{
    echo "usage: impair.sh <network_interface> <none|lan|wifi|wan|isp|3g|lte|sat|cecom1|cecom2|awful>"
}

function abort_if_interface_does_not_exist()
{
	TMP=$(ifconfig | grep "^[a-z,0-9]*:" | awk -F':' '{print $1}')":"
	NICS=$(echo ${TMP} | sed 's/ /:/g')
	CHK="${INTF}:"
	if [[ "${CHK}" == ":" ]]; then
		echo "ERROR: no network interface specified"
		show_help
		exit 1
	fi

	if [[ "${NICS}" != *"${CHK}"* ]]; then
		echo "ERROR: network interface '${INTF}' not found"
		show_help
		exit 1
	fi
}

function impair_reset()
{
	tc qdisc del dev ${INTF} root handle 1 2> /dev/null
}

function apply_tc()
{
	impair_reset

	echo "Applying impairment '${1}' on ${INTF}.  Delay=${2}ms, Jitter=${3}ms, Loss=${4}%, Duplication=${5}%."
	tc qdisc add dev ${INTF} root handle 1: netem delay ${2}ms ${3}ms loss ${4}% duplicate ${5}%
}

# Check if the network interface exists and abort if it doesn't
abort_if_interface_does_not_exist

# Remove anything we have already
impair_reset

if [ "$2" == "none" ]; then
	echo "Removing current impairments on ${INTF}."
	impair_reset

elif [ "$2" == "lan" ]; then
	apply_tc "${2}" "5" "5" "0.5" "0.1"

elif [ "$2" == "wifi" ]; then
	apply_tc "${2}" "20" "10" "1" "0.5"

elif [ "$2" == "wan" ]; then
	apply_tc "${2}" "80" "20" "1.2" "0.8"
	
elif [ "$2" == "isp" ]; then
	apply_tc "${2}" "120" "30" "1.5" "0.9"
	
elif [ "$2" == "3g" ]; then
	apply_tc "${2}" "300" "50" "2.5" "1"
	
elif [ "$2" == "lte" ]; then
	apply_tc "${2}" "150" "25" "0.9" "0.5"
	
elif [ "$2" == "sat" ]; then
	apply_tc "${2}" "900" "100" "5" "0.8"
	
elif [ "$2" == "cecom1" ]; then
	apply_tc "${2}" "750" "80" "12" "0.1"
	
elif [ "$2" == "cecom2" ]; then
	apply_tc "${2}" "1500" "160" "25" "0.5"
	
elif [ "$2" == "awful" ]; then
	apply_tc "${2}" "3000" "300" "30" "3.0"
	
else
	echo "ERROR: Unknown profile name '${2}'."
	show_help
fi
