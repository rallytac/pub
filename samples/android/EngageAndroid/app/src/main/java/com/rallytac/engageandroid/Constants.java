//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import com.rallytac.engage.engine.Engine;

public class Constants
{
    public static String KEY_LAUNCH_ACTIVITY = "Launcher.LAUNCH_ACTIVITY";//NON-NLS

    public static String QR_CODE_HEADER = "&*3$e1@E";//NON-NLS
    public static String QR_VERSION = "001";//NON-NLS
    public static String QR_DEFLECTION_URL_SEP = "/??";//NON-NLS

    public static String CERTSTORE_JSON_INTERNAL_PASSWORD_HEX_STRING = "__app__internal__pwd_hexstring";//NON-NLS

    public enum UiMode {vSingle, vMulti}

    public final static int PTT_KEY_DOUBLE_CLICK_LATCH_THRESHOLD_MS = 500;

    public final static int LAUNCH_TIMEOUT_MS = 5000;

    public final static int MIN_IP_PORT = 1;
    public final static int MAX_IP_PORT = 65535;

    public final static String CHARSET = "UTF-8";//NON-NLS

    public final static UiMode DEF_UI_MODE = UiMode.vSingle;
    public final static boolean DEF_UI_SHOW_TEXT_MESSAGING = false;

    public final static float DEF_PTT_TONE_LEVEL = (float)0.015;
    public final static float DEF_NOTIFICATION_TONE_LEVEL = (float)0.015;
    public final static float DEF_ERROR_TONE_LEVEL = (float)0.2;

    public final static boolean DEF_AEC_ENABLED = false;
    public final static boolean DEF_AEC_CNG = true;
    public final static int DEF_AEC_MODE = 0;
    public final static int DEF_AEC_SPEAKER_TAIL_MS = 60;
    public final static boolean DEF_AEC_STEREO_DISABLED = true;
    public final static int DEF_ANDROID_AUDIO_API = 0;
    public final static int DEF_ANDROID_AUDIO_SHARING_MODE = 0;         // Exclusive
    public final static int DEF_ANDROID_AUDIO_PERFORMANCE_MODE = 12;    // Low latency
    public final static int DEF_ANDROID_AUDIO_USAGE = 2;                // Voice communication
    public final static int DEF_ANDROID_AUDIO_CONTENT_TYPE = 1;         // Speech
    public final static int DEF_ANDROID_AUDIO_INPUT_PRESET = 7;         // Voice communication

    public final static int DEF_ENGINE_INTERNAL_AUDIO = 0;
    public final static boolean DEF_MICROPHONE_NOISE_REDUCTION = false;

    public final static boolean DEF_USE_RP = false;
    public final static String DEF_RP_ADDRESS = "";
    public final static int DEF_RP_PORT = 7443;

    public final static String DEF_BINDING_NIC_NAME = "wlan0";//NON-NLS

    public final static String DEF_USER_NODE_ID = "";
    public final static String DEF_USER_ID = "";
    public final static String DEF_USER_DISPLAY_NAME = "";
    public final static String DEF_USER_ALIAS_ID = "";

    public final static boolean DEF_LOCATION_ENABLED = true;
    public final static int DEF_LOCATION_ACCURACY = LocationManager.PRIORITY_BALANCED_POWER_ACCURACY;
    public final static int DEF_LOCATION_INTERVAL_SECS = 60;
    public final static int DEF_LOCATION_MIN_INTERVAL_SECS = 60;
    public final static float DEF_LOCATION_MIN_DISPLACEMENT = (float)5.0;

    public final static boolean DEF_MULTICAST_FAILOVER_ENABLED = true;
    public final static int DEF_MULTICAST_FAILOVER_THRESHOLD_SECS = 10;

    public final static boolean DEF_USER_AUDIO_JITTER_LOW_LATENCY_ENABLED = false;

    public final static int QR_CODE_WIDTH = 800;
    public final static int QR_CODE_HEIGHT = 800;

    public final static int RX_IDLE_SECS_BEFORE_NOTIFICATION = 30;
    public final static int TX_IDLE_SECS_BEFORE_NOTIFICATION = (RX_IDLE_SECS_BEFORE_NOTIFICATION / 2);

    public final static boolean DEF_NOTIFY_NODE_JOIN = true;
    public final static boolean DEF_NOTIFY_NODE_LEAVE = true;
    public final static boolean DEF_NOTIFY_NEW_AUDIO_RX = true;
    public final static boolean DEF_NOTIFY_NETWORK_ERROR = true;
    public final static boolean DEF_NOTIFY_VIBRATIONS = true;
    public final static boolean DEF_NOTIFY_PTT_EVERY_TIME = false;

    public final static int GROUP_HEALTH_CHECK_TIMER_INITIAL_DELAY_MS = 2000;
    public final static int GROUP_HEALTH_CHECK_TIMER_INTERVAL_MS = 2000;
    public final static int GROUP_HEALTH_CHECK_NETWORK_ERROR_NOTIFICATION_MIN_INTERVAL_MS = 10000;

    public final static String MISSION_DATABASE_NAME = "MissionDatabase";//NON-NLS
    public final static String MISSION_EDIT_EXTRA_JSON = "MissionJson";//NON-NLS
    public final static String MISSION_ACTIVATED_ID = "ActivatedMissionId";//NON-NLS

    public final static long MIN_LICENSE_ACTIVATION_DELAY_MS = 60000;
    public final static long MAX_LICENSE_ACTIVATION_DELAY_MS = (86400 * 1000);

    public final static boolean DEF_USER_UI_PTT_LATCHING = false;
    public final static boolean DEF_USER_UI_PTT_VOICE_CONTROL = false;

    public final static int TX_UNMUTE_DELAY_MS_AFTER_GRANT_TONE = 120;

    public final static int UNLIMITED_TX_SECS = 86400;
    public final static int DEFAULT_TX_SECS = 120;

    public final static int DEFAULT_ENCODER = 25;
    public final static int DEFAULT_TX_FRAMING_MS = 60;
    public final static String CERTSTORE_CHANGED_TO_FN = "CertStoreChangedToFn";//NON-NLS

    public final static int MAX_TEXT_MESSAGE_INPUT_SIZE = 256;
    public final static int TEXT_MESSAGE_BLOB_RTP_PAYLOAD_TYPE = 66;

    public final static int INVALID_AUDIO_DEVICE_ID = -1;

    public final static int DEF_MAX_GROUPS_ALLOWED = 4;

    public final static int DEFAULT_NETWORK_TX_TTL = 64;
    public final static int DEFAULT_NETWORK_QOS_PRIORITY = 4;

    public final static long ENGAGE_RXFLAG_EMERGENCY = 0x1L;

    public final static boolean DEF_MICROPHONE_AGC_ENABLED = true;
    public final static int DEF_MICROPHONE_AGC_MIN_LEVEL = 0;
    public final static int DEF_MICROPHONE_AGC_MAX_LEVEL = 255;
    public final static boolean DEF_MICROPHONE_AGC_LIMIT_ENABLED = false;
    public final static int DEF_MICROPHONE_AGC_TARGET_DB = 3;

    public final static boolean DEF_SPEAKER_AGC_ENABLED = false;
    public final static int DEF_SPEAKER_AGC_MIN_LEVEL = 0;
    public final static int DEF_SPEAKER_AGC_MAX_LEVEL = 255;
    public final static boolean DEF_SPEAKER_AGC_LIMIT_ENABLED = false;
    public final static int DEF_SPEAKER_AGC_TARGET_DB = 3;

    public final static String INTERNAL_DEFAULT_CERTSTORE_FN = "{7662214c-e79e-436f-8e93-7cf75cbac682}.certstore";//NON-NLS

    public final static String EPT_ELEMENT_NAME = "ept";//NON-NLS
    public final static int EPT_STATIC_VALUE = 100;

    public final static String ANONYMOUS_ALIAS_ELEMENT_NAME = "anonymousAlias";//NON-NLS

    public final static int GMT_STATUS_FLAG_JOINED = 0x01;
    public final static int GMT_STATUS_FLAG_CONNECTED = 0x02;

    // Activity request
    public final static int SETTINGS_REQUEST_CODE = 42;
    public final static int MISSION_LISTING_REQUEST_CODE = 43;
    public final static int CERTIFICATE_MANAGER_REQUEST_CODE = 45;
    public final static int ENGINE_POLICY_EDIT_REQUEST_CODE = 46;
    public final static int MISSION_QR_CODE_SCAN = 47;
    public final static int OFFLINE_ACTIVATION_CODE_REQUEST_CODE = 48;
    public final static int EDIT_ACTION_REQUEST_CODE = 49;
    public final static int PICK_MISSION_FILE_REQUEST_CODE = 50;
    public final static int OFFLINE_ACTIVATION_REQUEST_CODE = 51;

    public final static int PICK_LICENSE_FILE_REQUEST_CODE = 52;
    public final static int PICK_QR_LICENSE_FILE_REQUEST_CODE = 53;
    public final static int PICK_QR_ACTIVATION_FILE_REQUEST_CODE = 54;
    public final static int PICK_CERTSTORE_FILE_REQUEST_CODE = 55;
    public final static int PICK_QR_MISSION_FILE_REQUEST_CODE = 56;

    public final static int PICK_ENTERPRISE_ID_FILE_REQUEST_CODE = 57;
    public final static int PICK_QR_ENTERPRISE_ID_FILE_REQUEST_CODE = 58;

    public final static int KEYCODE_PTT_MODE_NONE = 0;
    public final static int KEYCODE_PTT_MODE_PUSH_ON_AT_FIRST_REPEAT_RELEASE_OFF = 1;
    public final static int KEYCODE_PTT_MODE_PUSH_ON_PUSH_OFF = 2;
}
