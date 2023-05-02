#!/bin/bash

#-----------------------------------------------------------------------------------------
# Engage Linguistics Meta Compiler v0.1
# Copyright (c) 2023 Rally Tactical Systems, Inc.
#-----------------------------------------------------------------------------------------

LMC_FILE="lmc.json"
RP_ID="local"
RP_ADDRESS="127.0.0.1"
RP_PORT="7443"
ENCODER_ID="25"

LINE_NUM=0
SESSION_TEMPLATE="{\"id\":\"@CURRENT_SESSION@\",\"groups\":[@SESSION_GROUPS@]}"
GROUP_TEMPLATE="{\
	\"id\":\"@GROUP_ID@\", \
	\"type\":1, \
	\"languageCode\":\"@LANGUAGE_CODE@\", \
	\"cryptoPassword\":\"@CRYPTO_PASSWORD@\", \
	\"txAudio\":{\"encoder\":${ENCODER_ID}}, \
	\"rallypoints\":[{\"id\":\"${RP_ID}\", \"host\":{\"address\":\"${RP_ADDRESS}\", \"port\":${RP_PORT}}}] \
	}"
	
ALL_SESSIONS=""
ALL_GROUPS=""
SESSION_ID=""
CURRENT_SESSION=""
SESSION_GROUPS=""


function is_valid_lc()
{
	QRY=".speechToText[] | select(.languageCode == \"$1\")"
	CHK=$(jq "${QRY}" ${LMC_FILE})
	if [ "${CHK}" != "" ]; then
		echo "1"
	else
		echo "0"
	fi
}


function show_langs()
{
	jq -r '["LanguageCode", "Name"], (.speechToText[] | [.languageCode, .name]) | @tsv ' ${LMC_FILE}
}


function show_help()
{
	echo "usage $0 [--langs] [< input_meta_file]"
}


function print_error()
{
	echo "${1}" >&2
}


function does_session_exist()
{
	QRY=".voiceToVoiceSessions[] | select(.id == \"$1\")"
	CHK=$(echo ${ALL_CONFIG} | jq "${QRY}")
	if [ "${CHK}" != "" ]; then
		echo "1"
	else
		echo "0"
	fi
}


function does_group_exist()
{
	QRY=".groups[] | select(.id == \"$1\")"
	CHK=$(echo ${ALL_CONFIG} | jq "${QRY}")
	if [ "${CHK}" != "" ]; then
		echo "1"
	else
		echo "0"
	fi
}


function finalize_current_session_and_reset()
{
	if [ "${CURRENT_SESSION}" != "" ]; then
		if [ "${SESSION_GROUPS}" != "" ]; then
			if [ "${ALL_SESSIONS}" != "" ]; then
				ALL_SESSIONS="${ALL_SESSIONS},"
			fi
			CURRENT_SESSION=$(echo ${CURRENT_SESSION} | sed "s/@SESSION_GROUPS@/${SESSION_GROUPS}/g")
			ALL_SESSIONS="${ALL_SESSIONS}${CURRENT_SESSION}"
		else
			print_error "No groups for session [${SESSION_ID}] on line $LINE_NUM"
			exit 1
		fi
	fi
	
	SESSION_ID=""
	SESSION_GROUPS=""
	CURRENT_SESSION=""
	
	ALL_CONFIG="{\"voiceToVoiceSessions\":[${ALL_SESSIONS}],\"groups\":[${ALL_GROUPS}]}"
}


function process_meta_file()
{
	while read -r INPUT_LINE; do
		LINE_NUM=$((LINE_NUM+1))
		
		if [ "${INPUT_LINE}" != "" ]; then
			FIRST_CHAR="${INPUT_LINE:0:1}"
			REST_OF_LINE="${INPUT_LINE:1}"		
			
			if [ "${FIRST_CHAR}" == "+" ]; then
				finalize_current_session_and_reset
				
				SESSION_ID=${REST_OF_LINE}
				
				if [ "${SESSION_ID}" == "" ]; then
					print_error "No session ID on line $LINE_NUM"
					exit 1
				fi
				
				if [ $(does_session_exist ${SESSION_ID}) == 1 ]; then
					print_error "Session ID [${SESSION_ID}] duplicate on line $LINE_NUM"
					exit 1
				fi
				
				CURRENT_SESSION=$(echo "${SESSION_TEMPLATE}" | sed "s/@CURRENT_SESSION@/${SESSION_ID}/g")
				SESSION_GROUPS=""
				
			elif [ "$FIRST_CHAR" == "-" ]; then
				if [ "${SESSION_ID}" == "" ]; then
					print_error "No session ID yet for group defined on line $LINE_NUM"
					exit 1
				fi
				
				GROUP_ID=$(echo ${REST_OF_LINE} | awk -F: '{print $1}')
				LANGUAGE_CODE=$(echo ${REST_OF_LINE} | awk -F: '{print $2}')
				CRYPTO_PASSWORD=$(echo ${REST_OF_LINE} | awk -F: '{print $3}')
				
				if [ "${GROUP_ID}" == "" ]; then
					print_error "No group ID on line $LINE_NUM"
					exit 1
					
				elif [ "${LANGUAGE_CODE}" == "" ]; then
					print_error "No language code on line $LINE_NUM"
					exit 1
				fi

				if [ $(does_group_exist ${GROUP_ID}) == 1 ]; then
					print_error "Group ID [${GROUP_ID}] on line $LINE_NUM is already used elsewhere"
					exit 1
				fi
				
				if [ $(is_valid_lc ${LANGUAGE_CODE}) == 0 ]; then
					print_error "Language code [${LANGUAGE_CODE}] on line $LINE_NUM is invalid"
					exit 1
				fi
										
				NEW_GROUP="${GROUP_TEMPLATE}"
				NEW_GROUP=$(echo ${NEW_GROUP} | sed "s/@GROUP_ID@/${GROUP_ID}/g")
				NEW_GROUP=$(echo ${NEW_GROUP} | sed "s/@LANGUAGE_CODE@/${LANGUAGE_CODE}/g")
				NEW_GROUP=$(echo ${NEW_GROUP} | sed "s/@CRYPTO_PASSWORD@/${CRYPTO_PASSWORD}/g")
						
				if [ "${ALL_GROUPS}" != "" ]; then
					ALL_GROUPS="${ALL_GROUPS},"
				fi
					
				ALL_GROUPS="${ALL_GROUPS}${NEW_GROUP}"
				
				if [ "${SESSION_GROUPS}" != "" ]; then
					SESSION_GROUPS="${SESSION_GROUPS},"
				fi			
				SESSION_GROUPS="${SESSION_GROUPS}\"${GROUP_ID}\""		
				
			elif [ "${FIRST_CHAR}" != "#" ]; then
				print_error "Invalid first character [${FIRST_CHAR}] on line ${LINE_NUM}"
				exit 1
			fi
		fi
	done

	finalize_current_session_and_reset
	echo ${ALL_CONFIG} | jq .
}

if [ "${1}" == "--help" ]; then
	show_help
elif [ "${1}" == "--langs" ]; then
	show_langs
else
	process_meta_file
fi

# END

