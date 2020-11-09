#!/bin/bash
#
# Copyright (c) 2020 Rally Tactical Systems, Inc.
#

DESIRED_VERSION=$1
UNAME_S=`(uname -s | tr A-Z a-z)`

# What OS is this for?
if [[ "${UNAME_S}" == *"darwin"* ]]; then
	BIN_PLATFORM="darwin_x64"
	BIN_OS_LIB_EXT="dylib"
else
	ARCH=`(uname -p | tr A-Z a-z)`
	
	echo "ERROR: Cannot determine operating system"
	exit 1
fi

# What should we use to download?
USEWGET=0
USECURL=0
VERINFO=`(wget --version)`
if [[ "${VERINFO}" != "" ]]; then
	USEWGET=1
else
	VERINFO=`(curl --version)`
	if [[ "${VERINFO}" != "" ]]; then
		USECURL=1
	else
		echo "ERROR: Cannot find curl or wget.  Please install one of these tools."
		exit 1
	fi
fi

function getFileFromUrl()
{
	FN="${1}"
	URL="${2}"

	if [[ "${USEWGET}" == "1" ]]; then
		wget -q -O "${FN}" "${URL}"
	elif [[ "${USECURL}" == "1" ]]; then
		curl -f -s -o "${FN}" -L "${URL}"
	else
		return 1
	fi
}

function fetchVersionFiles()
{
	rm -rf "engage"
	mkdir -p "engage"
	cd engage

	# $1 ... version
	# #2 ... platform
	# $3 ... file name
	function fetchBintrayFile()
	{	
		echo "Fetching ${3} from ${1}/${2} ..."
		rm -rf ${3}

		getFileFromUrl "${3}" "https://bintray.com/rallytac/pub/download_file?file_path=${1}/${2}/${3}"
		if [[ $? != "0" ]]; then
			rm -rf ${3}
			echo "ERROR: Error while downloading ${3}"
			exit 1
		fi
	}

	fetchBintrayFile "${DESIRED_VERSION}" "api/c/include" "EngageInterface.h"
	fetchBintrayFile "${DESIRED_VERSION}" "api/c/include" "EngageIntegralDataTypes.h"
	fetchBintrayFile "${DESIRED_VERSION}" "api/c/include" "ConfigurationObjects.h"
	fetchBintrayFile "${DESIRED_VERSION}" "api/c/include" "Constants.h"
	fetchBintrayFile "${DESIRED_VERSION}" "api/c/include" "Platform.h"
	fetchBintrayFile "${DESIRED_VERSION}" "${BIN_PLATFORM}" "libengage-shared.${BIN_OS_LIB_EXT}"
}

function checkIfVersionExistsAndExitIfNot()
{
	TMP_FILE="./checkIfVersionExistsAndExitIfNot.tmp"
	ERROR_ENCOUNTERED=0

	rm -rf "${TMP_FILE}"
	getFileFromUrl "${TMP_FILE}" "https://bintray.com/rallytac/pub/download_file?file_path=${DESIRED_VERSION}/api/c/include/EngageInterface.h"
	if [[ $? != "0" ]]; then
		ERROR_ENCOUNTERED=1
	fi
	
	if [[ ! -f "${TMP_FILE}" ]]; then
		ERROR_ENCOUNTERED=1
	fi

	rm -rf "${TMP_FILE}"

	if [[ "${ERROR_ENCOUNTERED}" == "1" ]]; then
		echo "Error encountered while checking for Engage version ${DESIRED_VERSION}.  This may be an invalid version or a connection could not be established to the file publication system."
		exit 1
	fi
}

function showHelp()
{
	echo "usage: 'getengage.sh <version_number>' or 'make depends VER=<version_number>' if called from make"
}

if [[ "${DESIRED_VERSION}" != "" ]]; then
	checkIfVersionExistsAndExitIfNot
	fetchVersionFiles
else
	echo "No version provided"
	showHelp
	exit 1
fi
