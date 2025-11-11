#!/bin/bash
#
# Copyright (c) 2020 Rally Tactical Systems, Inc.
#

function showHelp()
{
	echo "Usage: getengage.sh <version> <engage-line>"
	echo "<ersion>: The version of the Engage Engine to download - eg: 1.255.9095	"
	echo "<engage-line>: The line of the Engage Engine to download - eg: interim"
	echo "Example: $0 1.255.9095 interim"
	exit 1
}

if [[ "$1" == "--help" ]]; then
	showHelp
fi

DESIRED_VERSION=$1
if [[ "${DESIRED_VERSION}" == "" ]]; then
	echo "No version specified"
	showHelp
	exit 1
fi

ENGAGE_LINE=$2

if [[ "${ENGAGE_LINE}" == "" ]]; then
	echo "No version specified"
	showHelp
	exit 1
fi

UNAME_S=`(uname -s | tr A-Z a-z)`
URL_BASE="https://hq.rallytac.com/builds/${ENGAGE_LINE}"


# What OS is this for?
if [[ "${UNAME_S}" == *"darwin"* ]]; then
	BIN_OS_LIB_EXT="dylib"

	ARCH=`(uname -p | tr A-Z a-z)`
	if [[ "${ARCH}" == "arm" ]]; then
		BIN_PLATFORM="darwin_arm64"
	else
		if [[ "${ARCH}" == "x86_64" ]]; then
			BIN_PLATFORM="darwin_x64"
		else
			echo "ERROR: Cannot determine CPU architecture"
			exit 1
		fi
	fi
elif [[ "${UNAME_S}" == *"linux"* ]]; then
	BIN_OS_LIB_EXT="so"
	BIN_PLATFORM="linux"

	ARCH=`(uname -p | tr A-Z a-z)`
	if [[ "${ARCH}" == "x86_64" ]]; then
		BIN_PLATFORM="${BIN_PLATFORM}_x64"
	else
		# TODO: Check for ARM as well as 32-bit Intel
		if [[ "${ARCH}" == "aarch64" ]]; then
			BIN_PLATFORM="${BIN_PLATFORM}_arm64"
		elif [[ "${ARCH}" == "arm" ]]; then
			BIN_PLATFORM="${BIN_PLATFORM}_arm32"
		else
			BIN_PLATFORM="${BIN_PLATFORM}_x86"
		fi
	fi

	if [[ "${BIN_PLATFORM}" == "" ]]; then
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
DOWNLOADER=$(which wget)
if [[ "${DOWNLOADER}" != "" ]]; then
	USEWGET=1
else
	DOWNLOADER=$(which curl)
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
	echo "Getting ${FN} from ${URL}"

	if [[ "${USEWGET}" == "1" ]]; then
		wget --no-check-certificate -q -O "${FN}" "${URL}"
	elif [[ "${USECURL}" == "1" ]]; then
		curl -f -s -o "${FN}" -L "${URL}"
	else
		return 1
	fi
}

function fetchVersionFiles()
{
	echo "Fetching version files for ${DESIRED_VERSION}"

	rm -rf "engage"
	mkdir -p "engage"
	cd engage

	getFileFromUrl "json.zip" "https://github.com/nlohmann/json/releases/download/v3.12.0/include.zip"
	mkdir nhtmp
	cd nhtmp
	unzip ../json.zip > /dev/null 2>&1
	cd ..
	mv nhtmp/include .
	rm -rf json.zip
	rm -rf nhtmp

	# $1 ... version
	# #2 ... platform
	# $3 ... file name
	function fetchArtifactsFile()
	{	
		echo "Fetching ${3} from ${1}/${2} ..."
		rm -rf ${3}

		getFileFromUrl "${3}" "${URL_BASE}/${1}/${2}/${3}"
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
	fetchArtifactsFile "${DESIRED_VERSION}" "${BIN_PLATFORM}" "libengage-shared.${BIN_OS_LIB_EXT}"
	fetchArtifactsFile "${DESIRED_VERSION}" "${BIN_PLATFORM}" "libengage-static-native.a"
}


function checkIfVersionExistsAndExitIfNot()
{
	echo "Checking if version ${DESIRED_VERSION} exists"
	TMP_FILE="./checkIfVersionExistsAndExitIfNot.tmp"
	ERROR_ENCOUNTERED=0

	rm -rf "${TMP_FILE}"
	getFileFromUrl "${TMP_FILE}" "${URL_BASE}/${DESIRED_VERSION}/api/c/include/EngageInterface.h"
	echo "Downloaded ${URL_BASE}/${DESIRED_VERSION}/api/c/include/EngageInterface.h to ${TMP_FILE}"
	if [[ $? != "0" ]]; then
		echo "Error while downloading ${URL_BASE}/${DESIRED_VERSION}/api/c/include/EngageInterface.h"
		ERROR_ENCOUNTERED=1
	fi
	
	if [[ ! -f "${TMP_FILE}" ]]; then
		echo "Error while downloading ${URL_BASE}/${DESIRED_VERSION}/api/c/include/EngageInterface.h"
		ERROR_ENCOUNTERED=1
	else
		echo "File ${TMP_FILE} exists"
	fi

	rm -rf "${TMP_FILE}"

	if [[ "${ERROR_ENCOUNTERED}" == "1" ]]; then
		echo "Error encountered while checking for Engage version ${DESIRED_VERSION}.  This may be an invalid version or a connection could not be established to the file publication system."
		exit 1
	else
		echo "Version ${DESIRED_VERSION} exists"
	fi
}

# main
checkIfVersionExistsAndExitIfNot

echo "Fetching version files for ${DESIRED_VERSION}"
fetchVersionFiles

echo "Version files fetched successfully"
exit 0
