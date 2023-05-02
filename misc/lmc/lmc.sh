#!/bin/bash

#-----------------------------------------------------------------------------------------
# Engage Linguistics Meta Compiler v0.1
# Copyright (c) 2023 Rally Tactical Systems, Inc.
#-----------------------------------------------------------------------------------------

declare -a VALID_LC=("af-ZA" "am-ET" "ar-AE" "ar-BH" "ar-DZ" "ar-EG" "ar-IL" "ar-IQ" "ar-JO" "ar-KW" "ar-LB" "ar-LY" "ar-MA" "ar-OM" "ar-PS" "ar-QA" "ar-SA" "ar-SY" "ar-TN" "ar-YE" "az-AZ" "bg-BG" "bn-IN" "bs-BA" "ca-ES" "cs-CZ" "cy-GB" "da-DK" "de-AT" "de-CH" "de-DE" "el-GR" "en-AU" "en-CA" "en-GB" "en-GH" "en-HK" "en-IE" "en-IN" "en-KE" "en-NG" "en-NZ" "en-PH" "en-SG" "en-TZ" "en-US" "en-ZA" "es-AR" "es-BO" "es-CL" "es-CO" "es-CR" "es-CU" "es-DO" "es-EC" "es-ES" "es-GQ" "es-GT" "es-HN" "es-MX" "es-NI" "es-PA" "es-PE" "es-PR" "es-PY" "es-SV" "es-US" "es-UY" "es-VE" "et-EE" "eu-ES" "fa-IR" "fi-FI" "fil-PH" "fr-BE" "fr-CA" "fr-CH" "fr-FR" "ga-IE" "gl-ES" "gu-IN" "he-IL" "hi-IN" "hr-HR" "hu-HU" "hy-AM" "id-ID" "is-IS" "it-CH" "it-IT" "ja-JP" "jv-ID" "ka-GE" "kk-KZ" "km-KH" "kn-IN" "ko-KR" "lo-LA" "lt-LT" "lv-LV" "mk-MK" "ml-IN" "mn-MN" "mr-IN" "ms-MY" "mt-MT" "my-MM" "nb-NO" "ne-NP" "nl-BE" "nl-NL" "pl-PL" "ps-AF" "pt-BR" "pt-PT" "ro-RO" "ru-RU" "si-LK" "sk-SK" "sl-SI" "so-SO" "sq-AL" "sr-RS" "sv-SE" "sw-KE" "sw-TZ" "ta-IN" "te-IN" "th-TH" "tr-TR" "uk-UA" "uz-UZ" "vi-VN" "wuu-CN" "yue-CN" "zh-CN" "zh-CN-sichuan" "zh-HK" "zh-TW" "zu-ZA")

RP_ID="local"
RP_ADDRES="127.0.0.1"
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
	\"rallypoints\":[{\"id\":\"${RP_ID}\", \"host\":{\"address\":\"${RP}\", \"port\":${RP_PORT}}}] \
	}"
	
ALL_SESSIONS=""
ALL_GROUPS=""
SESSION_ID=""
CURRENT_SESSION=""
SESSION_GROUPS=""

function is_valid_lc()
{
	for X in ${VALID_LC[@]}; do
		if [ "${X}" == "${1}" ]; then
			echo "1"
			return
		fi
	done
		
	echo "0"
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
done < $1

finalize_current_session_and_reset
echo ${ALL_CONFIG} | jq .

# END

