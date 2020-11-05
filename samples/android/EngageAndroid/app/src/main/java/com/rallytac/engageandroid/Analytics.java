package com.rallytac.engageandroid;

public class Analytics
{
    public static final String ENGINE_STARTED = "ENGINE_STARTED";//NON-NLS
    public static final String ENGINE_START_FAILED = "ENGINE_START_FAILED";//NON-NLS
    public static final String ENGINE_STOPPED = "ENGINE_STOPPED";//NON-NLS

    public static final String GROUP_CREATED = "GRP_CREATED";//NON-NLS
    public static final String GROUP_CREATE_FAILED = "GRP_CREATE_FAILED";//NON-NLS
    public static final String GROUP_DELETED = "GRP_DELETED";//NON-NLS

    public static final String GROUP_CONNECTED_MC = "GRP_CONN_MC";//NON-NLS
    public static final String GROUP_CONNECTED_RP = "GRP_CONN_RP";//NON-NLS
    public static final String GROUP_CONNECTED_MC_FAILOVER = "GRP_CONN_MC_FO";//NON-NLS
    public static final String GROUP_CONNECTED_OTHER = "GRP_CONN_OTHER";//NON-NLS

    public static final String GROUP_CONNECT_FAILED_MC = "GRP_CONN_FAILED_MC";//NON-NLS
    public static final String GROUP_CONNECT_FAILED_RP = "GRP_CONN_FAILED_RP";//NON-NLS
    public static final String GROUP_CONNECT_FAILED_MC_FAILOVER = "GRP_CONN_FAILED_MC_FO";//NON-NLS
    public static final String GROUP_CONNECT_FAILED_OTHER = "GRP_CONN_FAILED_OTHER";//NON-NLS

    public static final String GROUP_DISCONNECTED_MC = "GRP_DISCONN_MC";//NON-NLS
    public static final String GROUP_DISCONNECTED_RP = "GRP_DISCONN_RP";//NON-NLS
    public static final String GROUP_DISCONNECTED_MC_FAILOVER = "GRP_DISCONN_MC_FO";//NON-NLS
    public static final String GROUP_DISCONNECTED_OTHER = "GRP_DISCONN_OTHER";//NON-NLS

    public static final String GROUP_JOINED = "GRP_JOINED";//NON-NLS
    public static final String GROUP_JOIN_FAILED = "GRP_JOIN_FAILED";//NON-NLS
    public static final String GROUP_LEFT = "GRP_LEFT";//NON-NLS

    //public static final String GROUP_MEMBER_COUNT_CHANGED= "GRP_MEMBER_COUNT_CHANGED";//NON-NLS

    public static final String GROUP_RX_STARTED = "GRP_RX_STARTED";//NON-NLS
    public static final String GROUP_RX_ENDED = "GRP_RX_ENDED";//NON-NLS
    //public static final String GROUP_RX_SPEAKER_COUNT_CHANGED = "GRP_RX_SPEAKER_COUNT_CHANGED";//NON-NLS

    public static final String GROUP_RX_MUTED = "GRP_RX_MUTED";//NON-NLS
    public static final String GROUP_RX_UNMUTED = "GRP_RX_UNMUTED";//NON-NLS

    public static final String GROUP_TX_REQUESTED_SINGLE = "GRP_TX_REQ_SINGLE";//NON-NLS
    public static final String GROUP_TX_REQUESTED_MULTIPLE = "GRP_TX_REQ_MULTIPLE";//NON-NLS

    public static final String GROUP_TX_STARTED = "GRP_TX_STARTED";//NON-NLS
    public static final String GROUP_TX_FAILED = "GRP_TX_FAILED";//NON-NLS
    public static final String GROUP_TX_ENDED = "GRP_TX_ENDED";//NON-NLS
    public static final String GROUP_TX_MAX_EXCEEDED = "GRP_TX_MAX_EXC";//NON-NLS
    public static final String GROUP_TX_USURPED = "GRP_TX_USURPED";//NON-NLS
    public static final String GROUP_TX_DURATION = "GROUP_TX_DURATION";//NON-NLS

    //public static final String GROUP_TX_MUTED = "GRP_TX_MUTED";//NON-NLS
    //public static final String GROUP_TX_UNMUTED = "GRP_TX_UNMUTED";//NON-NLS

    //public static final String GROUP_NODE_DISCOVERED = "GRP_NODE_DISCOVERED";//NON-NLS
    //public static final String GROUP_NODE_REDISCOVERED = "GRP_NODE_REDISCOVERED";//NON-NLS
    //public static final String GROUP_NODE_UNDISCOVERED = "GRP_NODE_UNDISCOVERED";//NON-NLS

    public static final String LICENSE_CHANGED = "LIC_CHANGED";//NON-NLS
    public static final String LICENSE_EXPIRED = "LIC_EXPIRED";//NON-NLS
    //public static final String LICENSE_EXPIRING = "LIC_EXPIRING";//NON-NLS

    public static final String LICENSE_ACT_OK = "LIC_ACT_OK";//NON-NLS
    public static final String LICENSE_ACT_OK_ALREADY = "LIC_ACT_OK_ALREADY";//NON-NLS
    public static final String LICENSE_ACT_FAILED_NO_KEY = "LIC_ACT_FAILED_NO_KEY";//NON-NLS
    public static final String LICENSE_ACT_FAILED = "LIC_ACT_FAILED";//NON-NLS
    public static final String LICENSE_DEACTIVATED = "LIC_DEACT";//NON-NLS

    public static final String GROUP_ASSET_DISCOVERED = "GRP_ASSET_DISCOVERED";//NON-NLS
    //public static final String GROUP_ASSET_REDISCOVERED = "GRP_ASSET_REDISCOVERED";//NON-NLS
    public static final String GROUP_ASSET_UNDISCOVERED = "GRP_ASSET_UNDISCOVERED";//NON-NLS

    public static final String GROUP_TIMELINE_REPORT = "GRP_TIMELINE_REPORT";//NON-NLS
    public static final String GROUP_TIMELINE_REPORT_FAILED = "GRP_TIMELINE_REPORT_FAILED";//NON-NLS

    public static final String GROUP_HEALTH_REPORT = "GRP_HEALTH_REPORT";//NON-NLS
    public static final String GROUP_HEALTH_REPORT_FAILED = "GRP_HEALTH_REPORT_FAILED";//NON-NLS

    public static final String GROUP_STATS_REPORT = "GRP_STATS_REPORT";//NON-NLS
    public static final String GROUP_STATS_REPORT_FAILED = "GRP_STATS_REPORT_FAILED";//NON-NLS

    public static final String GROUP_RP_CONNECTED = "GRP_RP_CONN";//NON-NLS
    public static final String GROUP_RP_DISCONNECTED = "GRP_RP_DISCONN";//NON-NLS

    public static final String GROUP_RP_RT_100 = "GRP_RP_RT_100";//NON-NLS
    public static final String GROUP_RP_RT_75 = "GRP_RP_RT_75";//NON-NLS
    public static final String GROUP_RP_RT_50 = "GRP_RP_RT_50";//NON-NLS
    public static final String GROUP_RP_RT_25 = "GRP_RP_RT_25";//NON-NLS
    public static final String GROUP_RP_RT_10 = "GRP_RP_RT_10";//NON-NLS
    public static final String GROUP_RP_RT_0 = "GRP_RP_RT_0";//NON-NLS

    public static final String VIEW_SINGLE_MODE = "VIEW_SINGLE_MODE";//NON-NLS
    public static final String VIEW_MULTI_MODE = "VIEW_MULTI_MODE";//NON-NLS
    public static final String VIEW_TEAM = "VIEW_TEAM";//NON-NLS
    public static final String VIEW_MAP = "VIEW_MAP";//NON-NLS
    public static final String VIEW_SHARE_MISSION = "VIEW_SHARE_MISSION";//NON-NLS
    public static final String VIEW_SETTINGS = "VIEW_SETTINGS";//NON-NLS
    public static final String VIEW_MISSION_LIST = "VIEW_MISSION_LIST";//NON-NLS
    public static final String VIEW_ABOUT = "VIEW_ABOUT";//NON-NLS
    public static final String VIEW_CONTACT = "VIEW_CONTACT";//NON-NLS
    public static final String VIEW_GROUP_LIST = "VIEW_GROUP_LIST";//NON-NLS
    public static final String VIEW_TIMELINE = "VIEW_TIMELINE";//NON-NLS
    public static final String VIEW_CERTIFICATES = "VIEW_CERTIFICATES";//NON-NLS

    public static final String TOGGLE_NETWORKING = "TOGGLE_NETWORKING";//NON-NLS
    public static final String MISSION_CHANGED = "MISS_CHANGED";//NON-NLS
    public static final String SINGLE_VIEW_GROUP_CHANGED = "SINGLE_VIEW_GROUP_CHANGED";//NON-NLS

    public static final String MISSION_QR_CODE_DISPLAYED_FOR_SHARE = "MISS_QR_CODE_DISP_FOR_SHARE";//NON-NLS
    public static final String MISSION_QR_CODE_FAILED_CREATE = "MISS_QR_CODE_FAILED_CREATE";//NON-NLS
    public static final String MISSION_SHARE_JSON = "MISS_SHARE_JSON";//NON-NLS
    public static final String MISSION_SHARE_QR = "MISS_SHARE_QR";//NON-NLS
    public static final String MISSION_SHARE_EXCEPTION = "MISS_SHARE_EXCEPTION";//NON-NLS
    public static final String MISSION_UPLOAD_REQUESTED = "MISS_UPLOAD_REQUESTED";//NON-NLS

    public static final String NEW_LICENSE_FROM_USER = "NEW_LICENSE_FROM_USER";//NON-NLS
}
