//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

public class PreferenceKeys
{
    public static String APP_FIRST_TIME_RUN = "app.firstTimeRun";//NON-NLS
    public static String LAUNCHER_RUN_BEFORE = "app.launcherRunBefore";//NON-NLS

    public static String UI_MODE = "ui.mode";//NON-NLS

    public static String UI_SHOW_TEXT_MESSAGING = "ui.showTextMessaging";//NON-NLS
    public static String UI_LARGE_PTT_BUTTON = "ui.largePttButton";//NON-NLS
    public static String UI_SHOW_ON_LOCK_SCREEN = "ui.showOnLockScreen";//NON-NLS

    public static String NETWORK_BINDING_NIC_NAME = "network_bindingNic";//NON-NLS
    public static String NETWORK_MULTICAST_FAILOVER_ENABLED = "network_multicastFailover.enabled";//NON-NLS
    //public static String NETWORK_MULTICAST_FAILOVER_SECS = "network_multicastFailover.secs";//NON-NLS


    public static String ACTIVE_MISSION_CONFIGURATION_JSON = "activeConfiguration_jsonTemplate";//NON-NLS
    public static String ACTIVE_MISSION_CONFIGURATION_SELECTED_GROUPS_SINGLE = "activeConfiguration_selectedGroups_single";//NON-NLS
    public static String ACTIVE_MISSION_CONFIGURATION_SELECTED_GROUPS_MULTI = "activeConfiguration_selectedGroups_multi";//NON-NLS

    public static String VOLUME_LEFT_FOR_GROUP_BASE_NAME = "volume.left.";//NON-NLS
    public static String VOLUME_RIGHT_FOR_GROUP_BASE_NAME = "volume.right.";//NON-NLS

    public static String USER_NODE_ID = "user_nodeId";//NON-NLS
    public static String USER_ID = "user_id";//NON-NLS
    public static String USER_DISPLAY_NAME = "user_displayName";//NON-NLS
    public static String USER_ALIAS_ID = "user_Alias";//NON-NLS

    public static String USER_TONE_LEVEL_PTT = "user_toneLevel.ptt";//NON-NLS
    public static String USER_TONE_LEVEL_ERROR = "user_toneLevel.error";//NON-NLS
    public static String USER_TONE_LEVEL_NOTIFICATION = "user_toneLevel.notification";//NON-NLS

    public static String USER_AUDIO_AEC_ENABLED = "user_audio.aec.enabled";//NON-NLS
    public static String USER_AUDIO_AEC_MODE = "user_audio.aec.mode";//NON-NLS
    public static String USER_AUDIO_AEC_CNG = "user_audio.aec.cng";//NON-NLS
    public static String USER_AUDIO_AEC_SPEAKER_TAIL_MS = "user_audio.aec.speakerTailMs";//NON-NLS
    public static String USER_AUDIO_AEC_DISABLE_STEREO = "user_audio.aec.disableStereo";//NON-NLS
    //public static String USER_AUDIO_JITTER_LOW_LATENCY_ENABLED = "user_audio_jitter_buffer_low_latency.enabled";//NON-NLS
    public static String USER_AUDIO_ANDROID_AUDIO_API = "user_audio.android.api";//NON-NLS
    public static String USER_AUDIO_ENGINE_INTERNAL_AUDIO = "user_audio.engine.internal.audio";//NON-NLS

    public static String USER_AUDIO_MICROPHONE_DENOISE = "user_audio.microphone.denoise";//NON-NLS
    public static String USER_AUDIO_MICROPHONE_AGC_LEVEL = "user_audio.microphone.agc.level";//NON-NLS
    public static String USER_AUDIO_SPEAKER_AGC_LEVEL = "user_audio.speaker.agc.level";//NON-NLS

    public static String USER_UI_PTT_LATCHING = "user_ui.pttLatching";//NON-NLS
    public static String USER_UI_PTT_VOICE_CONTROL = "user_ui.pttButtonVoiceControl";//NON-NLS

    public static String LAST_QRCODE_DEFLECTION_URL = "lastQrCodeDeflectionUrl";//NON-NLS
    public static String INCOMING_MISSION_PASSWORD = "incomingMissionPassword";//NON-NLS

    public static String MAP_OPTION_BASE = "map.option.";//NON-NLS
    public static String MAP_OPTION_VIEW_INDEX = MAP_OPTION_BASE + "viewIndex";//NON-NLS
    public static String MAP_OPTION_TYPE = MAP_OPTION_BASE + "type";//NON-NLS

    public static String MAP_OPTION_CAM_BASE = "map.option.camera.";//NON-NLS
    public static String MAP_OPTION_CAM_ZOOM = MAP_OPTION_CAM_BASE + "zoom";//NON-NLS
    public static String MAP_OPTION_CAM_BEARING = MAP_OPTION_CAM_BASE + "bearing";//NON-NLS
    public static String MAP_OPTION_CAM_LAT = MAP_OPTION_CAM_BASE + "lat";//NON-NLS
    public static String MAP_OPTION_CAM_LON = MAP_OPTION_CAM_BASE + "lon";//NON-NLS
    public static String MAP_OPTION_CAM_TILT = MAP_OPTION_CAM_BASE + "tilt";//NON-NLS

    public static String USER_LOCATION_SHARED = "user_location.enabled";//NON-NLS
    public static String USER_LOCATION_INTERVAL_SECS = "user_location.intervalSecs";//NON-NLS
    public static String USER_LOCATION_MIN_INTERVAL_SECS = "user_location.minIntervalSecs";//NON-NLS
    public static String USER_LOCATION_ACCURACY = "user_location.accuracy";//NON-NLS
    public static String USER_LOCATION_MIN_DISPLACEMENT = "user_location.minDisplacement";//NON-NLS

    public static String USER_NOTIFY_NODE_JOIN = "user_notify.nodeJoin";//NON-NLS
    public static String USER_NOTIFY_NODE_LEAVE = "user_notify.nodeLeave";//NON-NLS
    public static String USER_NOTIFY_NEW_AUDIO_RX = "user_notify.newAudioRx";//NON-NLS
    public static String USER_NOTIFY_NETWORK_ERROR = "user_notify.networkError";//NON-NLS
    public static String USER_NOTIFY_VIBRATIONS = "user_notify.vibrations";//NON-NLS
    public static String USER_NOTIFY_PTT_EVERY_TIME = "user_notify.ptt_everyTime";//NON-NLS


    public static String USER_EXPERIMENT_ENABLE_TX_SMOOTHING = "user_experiment.tx.smoothing.enable";//NON-NLS
    public static String USER_EXPERIMENT_ALLOW_DTX = "user_experiment.tx.allow_dtx";//NON-NLS

    public static String USER_EXPERIMENT_ENABLE_SSDP_DISCOVERY = "user_experiment.discovery.ssdp.enable";//NON-NLS

    public static String USER_EXPERIMENT_ENABLE_CISTECH_GV1_DISCOVERY = "user_experiment.discovery.cistech.gv1.enable";//NON-NLS
    public static String USER_EXPERIMENT_CISTECH_GV1_DISCOVERY_ADDRESS = "user_experiment.discovery.cistech.gv1.address";//NON-NLS
    public static String USER_EXPERIMENT_CISTECH_GV1_DISCOVERY_PORT = "user_experiment.discovery.cistech.gv1.port";//NON-NLS
    public static String USER_EXPERIMENT_CISTECH_GV1_DISCOVERY_TIMEOUT_SECS = "user_experiment.discovery.cistech.gv1.timeoutSecs";//NON-NLS

    public static String USER_EXPERIMENT_ENABLE_TRELLISWARE_DISCOVERY = "user_experiment.discovery.trellisware.enable";//NON-NLS

    public static String USER_EXPERIMENT_ENABLE_DEVICE_REPORT_CONNECTIVITY = "user_experiment.deviceReport.connectivity.enable";//NON-NLS
    public static String USER_EXPERIMENT_ENABLE_DEVICE_REPORT_POWER = "user_experiment.deviceReport.power.enable";//NON-NLS

    public static String USER_EXPERIMENT_ENABLE_HBM = "user_experiment.hbm.simulate.enable";//NON-NLS
    public static String USER_EXPERIMENT_HBM_INTERVAL_SECS = "user_experiment.hbm.simulate.intervalSecs";//NON-NLS
    public static String USER_EXPERIMENT_HBM_ENABLE_HEART_RATE = "user_experiment.hbm.simulate.heartRate.enable";//NON-NLS
    public static String USER_EXPERIMENT_HBM_ENABLE_SKIN_TEMP = "user_experiment.hbm.simulate.skinTemp.enable";//NON-NLS
    public static String USER_EXPERIMENT_HBM_ENABLE_CORE_TEMP = "user_experiment.hbm.simulate.coreTemp.enable";//NON-NLS
    public static String USER_EXPERIMENT_HBM_ENABLE_BLOOD_OXY = "user_experiment.hbm.simulate.bloodOxygenationPerc.enable";//NON-NLS
    public static String USER_EXPERIMENT_HBM_ENABLE_BLOOD_HYDRO = "user_experiment.hbm.simulate.bloodHydrationPerc.enable";//NON-NLS
    public static String USER_EXPERIMENT_HBM_ENABLE_FATIGUE_LEVEL = "user_experiment.hbm.simulate.fatigueLevel.enable";//NON-NLS
    public static String USER_EXPERIMENT_HBM_ENABLE_TASK_EFFECTIVENESS_LEVEL = "user_experiment.hbm.simulate.taskEffectivenessLevel.enable";//NON-NLS

    public static String CONFIGHUB_BASE_URL = "confighub.baseUrl";//NON-NLS

    public static String USER_BT_DEVICE_USE = "user_bt.use";//NON-NLS
    public static String USER_BT_DEVICE_ADDRESS = "user_bt.deviceAddress";//NON-NLS

    public static String USER_AUDIO_INPUT_DEVICE = "user_audio.inputDevice";//NON-NLS
    public static String USER_AUDIO_OUTPUT_DEVICE = "user_audio.outputDevice";//NON-NLS

    public static String USER_LICENSING_KEY = "user_licensing.key";//NON-NLS
    public static String USER_LICENSING_ACTIVATION_CODE = "user_licensing.activationCode";//NON-NLS
    public static String CHECKED_FOR_LICENSING_DONE = "checkedForLicensingDone";//NON-NLS
    public static String USER_ENTERPRISE_ID = "user_enterprise_id";//NON-NLS

    public static String DEVELOPER_MODE_ACTIVE = "developer_modeActive";//NON-NLS
    public static String DEVELOPER_USE_DEV_LICENSING_SYSTEM = "developer_useDevLicensingSystem";//NON-NLS
    public static String ADVANCED_MODE_ACTIVE = "advanced_modeActive";//NON-NLS

    public static String USER_CERT_STORE_FILE_NAME = "user_activeCertStoreFileName";//NON-NLS
    public static String USER_CERT_STORE_PASSWORD_SET = "user_certStore.passwordSet";//NON-NLS
    public static String USER_CERT_DEFAULT_ID = "user_cert.default.id";//NON-NLS
    public static String USER_CERT_DEFAULT_KEY = "user_cert.default.key";//NON-NLS
    public static String USER_CERT_DEFAULT_CA = "user_cert.default.ca";//NON-NLS
    public static String USER_THEME_DARK_MODE = "user_theme.mode.dark";//NON-NLS

    public static String ENGINE_POLICY_JSON = "engine_policy_json";//NON-NLS
}
