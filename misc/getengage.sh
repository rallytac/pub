#!/bin/bash
#
# Copyright (c) 2020 Rally Tactical Systems, Inc.
#

DESIRED_VERSION=$1
UNAME_S=`(uname -s | tr A-Z a-z)`

# What OS is this for?
if [[ "${UNAME_S}" == *"darwin"* ]]; then
	BIN_OS_LIB_EXT="dylib"
	BIN_PLATFORM="darwin_x64"
elif [[ "${UNAME_S}" == *"linux"* ]]; then
	BIN_OS_LIB_EXT="so"
	BIN_PLATFORM="linux"

	ARCH=`(uname -p | tr A-Z a-z)`
	if [[ "${ARCH}" == "x86_64" ]]; then
		BIN_PLATFORM="${BIN_PLATFORM}_x64"
	else
		# TODO: Check for ARM as well as 32-bit Intel
		
		echo "ERROR: Cannot determine CPU architecture"
		exit 1
	fi
else
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
	function fetchArtifactsFile()
	{	
		echo "Fetching ${3} from ${1}/${2} ..."
		rm -rf ${3}

		getFileFromUrl "${3}" "http://artifacts.rallytac.com/artifacts/builds/${1}/${2}/${3}"
		if [[ $? != "0" ]]; then
			rm -rf ${3}
			echo "ERROR: Error while downloading ${3}"
			exit 1
		fi
	}

	fetchArtifactsFile "${DESIRED_VERSION}" "api/c/include" "ConfigurationObjects.h"
	fetchArtifactsFile "${DESIRED_VERSION}" "api/c/include" "EngageAudioDevice.h"
	fetchArtifactsFile "${DESIRED_VERSION}" "api/c/include" "EngageConstants.h"
	fetchArtifactsFile "${DESIRED_VERSION}" "api/c/include" "EngageIntegralDataTypes.h"
	fetchArtifactsFile "${DESIRED_VERSION}" "api/c/include" "EngageInterface.h"
	fetchArtifactsFile "${DESIRED_VERSION}" "api/c/include" "EngageLm.h"
	fetchArtifactsFile "${DESIRED_VERSION}" "api/c/include" "EngageNetworkDevice.h"
	fetchArtifactsFile "${DESIRED_VERSION}" "api/c/include" "EngagePlatformNotifications.h"
	fetchArtifactsFile "${DESIRED_VERSION}" "api/c/include" "Platform.h"
	fetchArtifactsFile "${DESIRED_VERSION}" "${BIN_PLATFORM}" "libengage-shared.${BIN_OS_LIB_EXT}"
	fetchArtifactsFile "${DESIRED_VERSION}" "${BIN_PLATFORM}" "libengage-static.a"
}


function checkIfVersionExistsAndExitIfNot()
{
	TMP_FILE="./checkIfVersionExistsAndExitIfNot.tmp"
	ERROR_ENCOUNTERED=0

	rm -rf "${TMP_FILE}"
	getFileFromUrl "${TMP_FILE}" "http://artifacts.rallytac.com/artifacts/builds/${DESIRED_VERSION}/api/c/include/EngageInterface.h"
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

function determineLatestEngageVersion()
{
	TMP_FILE="./engageVersion.tmp"
	ERROR_ENCOUNTERED=0

	rm -rf "${TMP_FILE}"
	getFileFromUrl "${TMP_FILE}" "http://artifacts.rallytac.com/artifacts/builds"
	if [[ $? != "0" ]]; then
        ERROR_ENCOUNTERED=1
	else
        if [[ ! -f "${TMP_FILE}" ]]; then
                ERROR_ENCOUNTERED=1
		else
			DESIRED_VERSION=`(cat "${TMP_FILE}" | grep "[DIR]" | tail -1 | awk -F'href="' '{print $2}' | awk -F'/' '{print $1}')`
        fi
    fi

	rm -rf "${TMP_FILE}"
}

if [[ "${DESIRED_VERSION}" == "" ]]; then
	determineLatestEngageVersion
fi

if [[ "${DESIRED_VERSION}" != "" ]]; then
	checkIfVersionExistsAndExitIfNot

	fetchVersionFiles
	exit 0
else
	echo "No version specified or determined"
fi
