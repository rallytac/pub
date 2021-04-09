//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

using Newtonsoft.Json.Linq;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

public class Engage
{
    #region Interfaces and such
    public class GroupDescriptor
    {
        public string id;
        public string name;
        public bool isEncrypted;
        public bool allowsFullDuplex;
    }

    public interface IEngineNotifications
    {
        void onEngineStarted(string eventExtraJson);
        void onEngineStopped(string eventExtraJson);
        void onEngineAudioDevicesRefreshed(string eventExtraJson);
    }

    public interface ILicenseNotifications
    {
        void onLicenseChanged(string eventExtraJson);
        void onLicenseExpired(string eventExtraJson);
        void onLicenseExpiring(double secondsLeft, string eventExtraJson);
    }

    public interface ILoggingNotifications
    {
        void onEngageLogMessage(int level, string tag, string message);
    }

    public interface IRallypointNotifications
    {
        void onRallypointPausingConnectionAttempt(string id, string eventExtraJson);
        void onRallypointConnecting(string id, string eventExtraJson);
        void onRallypointConnected(string id, string eventExtraJson);
        void onRallypointDisconnected(string id, string eventExtraJson);
        void onRallypointRoundtripReport(string id, int rtMs, int rtRating, string eventExtraJson);
    }

    public interface IGroupNotifications
    {
        void onGroupCreated(string id, string eventExtraJson);
        void onGroupCreateFailed(string id, string eventExtraJson);
        void onGroupDeleted(string id, string eventExtraJson);
        void onGroupConnected(string id, string eventExtraJson);
        void onGroupConnectFailed(string id, string eventExtraJson);
        void onGroupDisconnected(string id, string eventExtraJson);
        void onGroupJoined(string id, string eventExtraJson);
        void onGroupJoinFailed(string id, string eventExtraJson);
        void onGroupLeft(string id, string eventExtraJson);
        void onGroupMemberCountChanged(string id, int newCount, string eventExtraJson);
        void onGroupRxStarted(string id, string eventExtraJson);
        void onGroupRxEnded(string id, string eventExtraJson);
        void onGroupRxMuted(string id, string eventExtraJson);
        void onGroupRxUnmuted(string id, string eventExtraJson);
        void onGroupTxMuted(string id, string eventExtraJson);
        void onGroupTxUnmuted(string id, string eventExtraJson);
        void onGroupRxSpeakersChanged(string id, string groupTalkerJson, string eventExtraJson);
        void onGroupTxStarted(string id, string eventExtraJson);
        void onGroupTxEnded(string id, string eventExtraJson);
        void onGroupTxFailed(string id, string eventExtraJson);
        void onGroupTxUsurpedByPriority(string id, string eventExtraJson);
        void onGroupMaxTxTimeExceeded(string id, string eventExtraJson);
        void onGroupNodeDiscovered(string id, string nodeJson, string eventExtraJson);
        void onGroupNodeRediscovered(string id, string nodeJson, string eventExtraJson);
        void onGroupNodeUndiscovered(string id, string nodeJson, string eventExtraJson);
        void onGroupAssetDiscovered(string id, string nodeJson, string eventExtraJson);
        void onGroupAssetRediscovered(string id, string nodeJson, string eventExtraJson);
        void onGroupAssetUndiscovered(string id, string nodeJson, string eventExtraJson);
        void onGroupBlobSent(string id, string eventExtraJson);
        void onGroupBlobSendFailed(string id, string eventExtraJson);
        void onGroupBlobReceived(string id, string blobInfoJson, byte[] blob, int blobSize, string eventExtraJson);
        void onGroupRtpSent(string id, string eventExtraJson);
        void onGroupRtpSendFailed(string id, string eventExtraJson);
        void onGroupRtpReceived(string id, string rtpInfoJson, byte[] payload, int payloadSize, string eventExtraJson);
        void onGroupRawSent(string id, string eventExtraJson);
        void onGroupRawSendFailed(string id, string eventExtraJson);
        void onGroupRawReceived(string id, byte[] raw, int rawSize, string eventExtraJson);

        void onGroupTimelineEventStarted(string id, string eventJson, string eventExtraJson);
        void onGroupTimelineEventUpdated(string id, string eventJson, string eventExtraJson);
        void onGroupTimelineEventEnded(string id, string eventJson, string eventExtraJson);
        void onGroupTimelineReport(string id, string reportJson, string eventExtraJson);
        void onGroupTimelineReportFailed(string id, string eventExtraJson);
        void onGroupTimelineGroomed(string id, string eventListJson, string eventExtraJson);        

        void onGroupHealthReport(string id, string healthReportJson, string eventExtraJson);
        void onGroupHealthReportFailed(string id, string eventExtraJson);        

        void onGroupStatsReport(string id, string statsReportJson, string eventExtraJson);
        void onGroupStatsReportFailed(string id, string eventExtraJson);

        void onGroupRxVolumeChanged(string id, int leftLevelPerc, int rightLevelPerc, string eventExtraJson);
        void onGroupRxDtmf(string id, string dtmfJson, string eventExtraJson);

        void onGroupReconfigured(const char *pszId, const char *pEventExtraJson);
        void onGroupReconfigurationFailed(const char *pszId, const char *pEventExtraJson);
    }

    public interface IHumanBiometricsNotifications
    {
        void onHumanBiometricsReceived(string groupId, string nodeId, string hbmJson, string eventExtraJson);
	}

    public interface IBridgeNotifications
    {
        void onBridgeCreated(string id, string eventExtraJson);
        void onBridgeCreateFailed(string id, string eventExtraJson);
        void onBridgeDeleted(string id, string eventExtraJson);
    }
    #endregion


    // The Engage DLL
    private const string ENGAGE_DLL = "engage-shared.dll";

    // Limits
    public const int ENGAGE_MAX_GROUP_ID_SZ = 64;
    public const int ENGAGE_MAX_GROUP_NAME_SZ = 128;

    // Result codes
    public const int ENGAGE_RESULT_OK = 0;
    public const int ENGAGE_RESULT_INVALID_PARAMETERS = -1;
    public const int ENGAGE_RESULT_NOT_INITIALIZED = -2;
    public const int ENGAGE_RESULT_ALREADY_INITIALIZED = -3;
    public const int ENGAGE_RESULT_GENERAL_FAILURE = -4;
    public const int ENGAGE_RESULT_NOT_STARTED = -5;
    public const int ENGAGE_RESULT_ALREADY_STARTED = -6;

    // Jitter Buffer Latency types
    public enum JitterBufferLatency : int
    {
        STANDARD = 0,
        LOW_LATENCY = 1
    }

    // Connection Types
    public enum ConnectionType : int
    {
        UNDEFINED = 0,
        IP_MULTICAST = 1,
        RALLYPOINT = 2
    }

    // TX status codes
    public enum TxStatus : int
    {
        ERR_UNDEFINED = 0,
        OK_STARTED = 1,
        OK_ENDED = 2,
        ERR_NOT_AN_AUDIO_GROUP = -1,
        ERR_NOT_JOINED = -2,
        ERR_NOT_CONNECTED = -3,
        ERR_ALREADY_TRANSMITTING = -4,
        ERR_INVALID_PARAMS = -5,
        ERR_PRIORITY_TOO_LOW = -6,
        ERR_RX_ACTIVE_ON_NON_FDX = -7,
        ERR_CANNOT_SUBSCRIBE_TO_MIC = -8,
        ERR_INVALID_ID = -9
    }

    // License status codes
    public enum LicensingStatusCode : int
    {
        OK = 0,
        ERR_NULL_ENTITLEMENT_KEY = -1,
        ERR_NULL_LICENSE_KEY = -2,
        ERR_INVALID_LICENSE_KEY_LEN = -3,
        ERR_LICENSE_KEY_VERIFICATION_FAILURE = -4,
        ERR_ACTIVATION_CODE_VERIFICATION_FAILURE = -5,
        ERR_INVALID_EXPIRATION_DATE = -6,
        ERR_GENERAL_FAILURE = -7,
        ERR_NOT_INITIALIZED = -8,
        ERR_REQUIRES_ACTIVATION = -9
    }

    // Logging levels
    public enum LoggingLevel : int
    {
        FATAL = 0,
        ERROR = 1,
        WARNING = 2,
        INFORMATION = 3,
        DEBUG = 4
    }

    // Blob payload types
    public const byte ENGAGE_BLOB_PT_UNDEFINED = 0;
    public const byte ENGAGE_BLOB_PT_APP_TEXT_UTF8 = 1;
    public const byte ENGAGE_BLOB_PT_JSON_TEXT_UTF8 = 2;
    public const byte ENGAGE_BLOB_PT_APP_BINARY = 3;
    public const byte ENGAGE_BLOB_PT_ENGAGE_BINARY_HUMAN_BIOMETRICS = 4;

    // Human biometrics types
    public const byte ENGAGE_HBM_HEART_RATE = 1;
    public const byte ENGAGE_HBM_SKIN_TEMP = 2;
    public const byte ENGAGE_HBM_CORE_TEMP = 3;
    public const byte ENGAGE_HBM_HYDRATION = 4;
    public const byte ENGAGE_HBM_BLOOD_OXYGENATION = 5;
    public const byte ENGAGE_HBM_FATIGUE_LEVEL = 6;
    public const byte ENGAGE_HBM_TASK_EFFECTIVENESS = 7;

    // Group sources
    public const String GROUP_SOURCE_ENGAGE_INTERNAL = "com.rallytac.engage.internal";
    public const String GROUP_SOURCE_ENGAGE_MAGELLAN_CORE = "com.rallytac.magellan.core";
    public const String GROUP_SOURCE_ENGAGE_MAGELLAN_CISTECH = "com.rallytac.engage.magellan.cistech";
    public const String GROUP_SOURCE_ENGAGE_MAGELLAN_TRELLISWARE = "com.rallytac.engage.magellan.trellisware";
    public const String GROUP_SOURCE_ENGAGE_MAGELLAN_SILVUS = "com.rallytac.engage.magellan.silvus";
    public const String GROUP_SOURCE_ENGAGE_MAGELLAN_PERSISTENT = "com.rallytac.engage.magellan.persistent";
    public const String GROUP_SOURCE_ENGAGE_MAGELLAN_DOMO = "com.rallytac.engage.magellan.domo";
    public const String GROUP_SOURCE_ENGAGE_MAGELLAN_KENWOOD = "com.rallytac.engage.magellan.kenwood";
    public const String GROUP_SOURCE_ENGAGE_MAGELLAN_TAIT = "com.rallytac.engage.magellan.tait";
    public const String GROUP_SOURCE_ENGAGE_MAGELLAN_VOCALITY= "com.rallytac.engage.magellan.vocality";

    // Group disconnected reasons
    public const String GROUP_DISCONNECTED_REASON_NO_REAON = "NoReason";
    public const String GROUP_DISCONNECTED_REASON_NO_LINK = "NoLink";
    public const String GROUP_DISCONNECTED_REASON_UNREGISTERED = "Unregistered";
    public const String GROUP_DISCONNECTED_REASON_NOT_ALLOWED = "NotAllowed";
    public const String GROUP_DISCONNECTED_REASON_GENERAL_DENIAL = "GeneralDenial";

    // Presence descriptor group item status flags
    public const int ENGAGE_PDGI_FLAG_JOINED = 0x0001;
    public const int ENGAGE_PDGI_FLAG_CONNECTED = 0x0002;
    public const int ENGAGE_PDGI_FLAG_RX_MUTED = 0x0004;
    public const int ENGAGE_PDGI_FLAG_TX_MUTED = 0x0008;

    // NetworkTxPriority
    public enum NetworkTxPriority : int
    {
        PRI_BEST_EFFORT = 0,
        PRI_SIGNALING = 2,
        PRI_VIDEO = 3,
        PRI_VOICE = 4
    }

    public class JsonFields
    {
        public class GroupCreationDetail
        {
            public static String objectName = "groupCreationDetail";
            public static String id = "id";
            public static String status = "status";
        }

        public class GroupTxDetail
        {
            public static String objectName = "groupTxDetail";
            public static String id = "id";
            public static String status = "status";
            public static String localPriority = "localPriority";
            public static String remotePriority = "remotePriority";
            public static String nonFdxMsHangRemaining = "nonFdxMsHangRemaining";
        }

        public class RallypointConnectionDetail
        {
            public static String objectName = "rallypointConnectionDetail";
            public static String internalId = "internalId";
            public static String host = "host";
            public static String port = "port";
            public static String msToNextConnectionAttempt = "msToNextConnectionAttempt";
        }

        public class GroupConnectionDetail
        {
            public static String objectName = "groupConnectionDetail";
            public static String id = "id";
            public static String connectionType = "connectionType";
            public static String peer = "peer";
            public static String asFailover = "asFailover";
            public static String reason = "reason";
        }

        public class CertStoreCertificateElement
        {
            public static String objectName = "certStoreCertificateElement";
            public static String arrayName = "certificates";
            public static String id = "id";
            public static String hasPrivateKey = "hasPrivateKey";
            public static String tags = "tags";
        }

        public class CertStoreDescriptor
        {
            public static String objectName = "certStoreDescriptor";
            public static String id = "id";
            public static String fileName = "fileName";
            public static String version = "version";
            public static String flags = "flags";
            public static String certificates = "certificates";
        }

        public class ListOfAudioDevice
        {
            public static String objectName = "list";
        }

        public class AudioDevice
        {
            public static String objectName = "audioDevice";
            public static String deviceId = "deviceId";
            public static String samplingRate = "samplingRate";
            public static String msPerBuffer = "msPerBuffer";
            public static String bufferCount = "bufferCount";
            public static String channels = "channels";
            public static String direction = "direction";
            public static String boostPercentage = "boostPercentage";
            public static String isAdad = "isAdad";
            public static String name = "name";
            public static String manufacturer = "manufacturer";
            public static String model = "model";
            public static String hardwareId = "hardwareId";
            public static String serialNumber = "serialNumber";
            public static String isDefault = "isDefault";
            public static String extra = "extra";
            public static String type = "type";
            public static String isPresent = "isPresent";
        }

        public class ListOfNetworkInterfaceDevice
        {
            public static String objectName = "list";
        }

        public class NetworkInterfaceDevice
        {
            public static String objectName = "networkInterfaceDevice";
            public static String name = "name";
            public static String friendlyName = "friendlyName";
            public static String description = "description";
            public static String family = "family";
            public static String address = "address";
            public static String available = "available";
            public static String isLoopback = "isLoopback";
            public static String supportsMulticast = "supportsMulticast";
            public static String hardwareAddress = "hardwareAddress";
        }

        public class AdvancedTxParams
        {
            public static String objectName = "advancedTxParams";
            public static String flags = "flags";
            public static String priority = "priority";
            public static String subchannelTag = "subchannelTag";
            public static String includeNodeId = "includeNodeId";
            public static String alias = "alias";
            public static String muted = "muted";
            public static String txId = "txId";

            public class AudioUri
            {
                public static String objectName = "audioUri";
                public static String uri = "uri";
                public static String repeatCount = "repeatCount";
            }
        }

        public class License
        {
            public static String objectName = "license";
            public static String entitlement = "entitlement";
            public static String key = "key";
            public static String activationCode = "activationCode";
            public static String deviceId = "deviceId";
            public static String type = "type";
            public static String expires = "expires";
            public static String expiresFormatted = "expiresFormatted";
            public static String manufacturerId = "manufacturerId";
        }

        public class TalkerInformation
        {
            public static String objectName = "talkerInformation";
            public static String alias = "alias";
            public static String nodeId = "nodeId";
            public static String rxFlags = "rxFlags";
            public static String txPriority = "txPriority";
            public static String txId = "txId";
        }

        public class GroupTalkers
        {
            public static String objectName = "GroupTalkers";
            public static String list = "list";
        }

        public class EnginePolicy
        {
            public class Database
            {
                public static String objectName = "database";
                public static String enabled = "enabled";
                public static String type = "type";
                public static String fixedFileName = "fixedFileName";
            }

            public class Internals
            {
                public static String objectName = "internals";
                public static String disableWatchdog = "disableWatchdog";
                public static String watchdogIntervalMs = "watchdogIntervalMs";
                public static String watchdogHangDetectionMs = "watchdogHangDetectionMs";
                public static String housekeeperIntervalMs = "housekeeperIntervalMs";
                public static String logTaskQueueStatsIntervalMs = "logTaskQueueStatsIntervalMs";
                public static String maxTxSecs = "maxTxSecs";
                public static String maxRxSecs = "maxRxSecs";
                public static String enableLazySpeakerClosure = "enableLazySpeakerClosure";
                public static String rtpExpirationCheckIntervalMs = "rtpExpirationCheckIntervalMs";                
            }

            public class Timelines
            {
                public static String objectName = "timelines";
                public static String enabled = "enabled";
                public static String maxEventAgeSecs = "maxEventAgeSecs";
	            public static String storageRoot = "storageRoot";
                public static String maxStorageMb = "maxStorageMb";
	            public static String maxEvents = "maxEvents";
                public static String groomingIntervalSecs = "groomingIntervalSecs";
                public static String autosaveIntervalSecs = "autosaveIntervalSecs";
                public static String disableSigningAndVerification = "disableSigningAndVerification";
                public static String ephemeral = "ephemeral";                
            }

            public class Security
            {
                public static String objectName = "security";
            }

            public class Certificate
            {
                public static String objectName = "certificate";
                public static String certificate = "certificate";
                public static String key = "key";
            }

            public class Licensing
            {
                public static String objectName = "licensing";
                public static String entitlement = "entitlement";
                public static String key = "key";
                public static String activationCode = "activationCode";
                public static String manufacturerId = "manufacturerId";
            }

            public class Networking
            {
                public static String objectName = "networking";
                public static String defaultNic = "defaultNic";
                public static String maxOutputQueuePackets = "maxOutputQueuePackets";
                public static String rtpJitterMinMs = "rtpJitterMinMs";
                public static String rtpJitterMaxFactor = "rtpJitterMaxFactor";
                public static String rtpJitterMaxMs = "rtpJitterMaxMs";
                public static String rtpLatePacketSequenceRange = "rtpLatePacketSequenceRange";
                public static String rtpJitterTrimPercentage = "rtpJitterTrimPercentage";
                public static String rtpJitterUnderrunReductionThresholdMs = "rtpJitterUnderrunReductionThresholdMs";
                public static String rtpJitterUnderrunReductionAger = "rtpJitterUnderrunReductionAger";
                public static String rtpJitterForceTrimAtMs = "rtpJitterForceTrimAtMs";
                public static String rtpLatePacketTimestampRangeMs = "rtpLatePacketTimestampRangeMs";
                public static String rtpInboundProcessorInactivityMs = "rtpInboundProcessorInactivityMs";
                public static String multicastRejoinSecs = "multicastRejoinSecs";
                public static String rpLeafConnectTimeoutSecs = "rpLeafConnectTimeoutSecs";
                public static String maxReconnectPauseMs = "maxReconnectPauseMs";
                public static String reconnectFailurePauseIncrementMs = "reconnectFailurePauseIncrementMs";
                public static String sendFailurePauseMs = "sendFailurePauseMs";
                public static String rallypointRtTestIntervalMs = "rallypointRtTestIntervalMs";
                public static String logRtpJitterBufferStats = "logRtpJitterBufferStats";
                public static String preventMulticastFailover = "preventMulticastFailover";
                public static String rtcpPresenceTimeoutMs = "rtcpPresenceTimeoutMs";
                public static String rtpJtterLatencyMode = "rtpJtterLatencyMode"; 
                public static String rtpJitterMaxExceededClipPerc = "rtpJitterMaxExceededClipPerc";
                public static String rtpJitterMaxExceededClipHangMs = "rtpJitterMaxExceededClipHangMs";
                public static String rtpZombieLifetimeMs = "rtpZombieLifetimeMs";
                public static String rtpMaxTrimMs = "rtpMaxTrimMs";
            }

            public class Audio
            {
                public static String objectName = "audio";
                public static String enabled = "enabled";
                public static String internalRate = "internalRate";
                public static String internalChannels = "internalChannels";
                public static String allowOutputOnTransmit = "allowOutputOnTransmit";
                public static String muteTxOnTx = "muteTxOnTx";
                public static String denoiseInput = "denoiseInput";
                public static String denoiseOutput = "denoiseOutput";

                public class Aec
                {
                    public static String objectName = "aec";
                    public static String enabled = "enabled";
                    public static String mode = "mode";
                    public static String speakerTailMs = "speakerTailMs";
                    public static String cng = "cng";
                }

                public class Vad
                {
                    public static String objectName = "vad";
                    public static String enabled = "enabled";
                    public static String mode = "mode";
                }

                public class Android
                {
                    public static String objectName = "android";
                    public static String api = "api";
                }

                public class InputAgc
                {
                    public static String objectName = "inputAgc";
                    public static String enabled = "enabled";
                    public static String minLevel = "minLevel";
                    public static String maxLevel = "maxLevel";
                    public static String compressionGainDb = "compressionGainDb";
                    public static String enableLimiter = "enableLimiter";
                    public static String targetLevelDb = "targetLevelDb";
                }

                public class OutputAgc
                {
                    public static String objectName = "outputAgc";
                    public static String enabled = "enabled";
                    public static String minLevel = "minLevel";
                    public static String maxLevel = "maxLevel";
                    public static String compressionGainDb = "compressionGainDb";
                    public static String enableLimiter = "enableLimiter";
                    public static String targetLevelDb = "targetLevelDb";
                }            
            }

            public class Discovery
            {
                public static String objectName = "discovery";

                public class Ssdp
                {
                    public static String objectName = "ssdp";
                    public static String enabled = "enabled";
                    public static String ageTimeoutMs = "ageTimeoutMs";
                    public static String address = "address";
                }

                public class Cistech
                {
                    public static String objectName = "cistech";
                    public static String enabled = "enabled";
                    public static String ageTimeoutMs = "ageTimeoutMs";
                    public static String address = "address";
                }

                public class Trellisware
                {
                    public static String objectName = "trellisware";
                    public static String enabled = "enabled";
                }
            }

            public static String dataDirectory = "dataDirectory";
        }

        public class Mission
        {
            public static String id = "id";
            public static String name = "name";
            public static String description = "description";
            public static String modPin = "modPin";
            public static String certStoreId = "certStoreId";
            public static String multicastFailoverPolicy = "multicastFailoverPolicy";
        }

        public class Rallypoint
        {
            public static String objectName = "rallypoint";
            public static String arrayName = "rallypoints";

            public class Host
            {
                public static String objectName = "host";
                public static String address = "address";
                public static String port = "port";
            }

            public static String certificate = "certificate";
            public static String certificateKey = "certificateKey";
            public static String verifyPeer = "verifyPeer";
            public static String allowSelfSignedCertificate = "allowSelfSignedCertificate";
            public static String transactionTimeoutMs = "transactionTimeoutMs";
            public static String connectionTimeoutSecs = "connectionTimeoutSecs";            
            public static String disableMessageSigning = "disableMessageSigning";
            public static String use = "use";
        }

        public class Address
        {
            public static String objectName = "address";
            public static String address = "address";
            public static String port = "port";
        }

        public class Rx
        {
            public static String objectName = "rx";
            public static String address = "address";
            public static String port = "port";
        }

        public class Tx
        {
            public static String objectName = "tx";
            public static String address = "address";
            public static String port = "port";
        }

        public class Group
        {
            public static String objectName = "group";
            public static String arrayName = "groups";
            public static String id = "id";
            public static String name = "name";
            public static String type = "type";
            public static String source = "source";
            public static String cryptoPassword = "cryptoPassword";
            public static String alias = "alias";
            public static String maxRxSecs = "maxRxSecs";
            public static String enableMulticastFailover = "enableMulticastFailover";
            public static String multicastFailoverSecs = "multicastFailoverSecs";
            public static String interfaceName = "interfaceName";
            public static String anonymousAlias = "anonymousAlias";
            public static String lbCrypto = "lbCrypto";

            public class Timeline
            {
                public static String objectName = "timeline";
                public static String enabled = "enabled";
            }

            public class Audio
            {
                public static String objectName = "audio";
                public static String inputId = "inputId";
                public static String outputId = "outputId";
            }

            public class PriorityTranslation
            {
                public static String objectName = "priorityTranslation";
                public static String priority = "priority";
                public static String rx = "rx";
                public static String tx = "tx";
            }
        }

        public class TxAudio
        {
            public static String objectName = "txAudio";
            public static String fdx = "fdx";
            public static String encoder = "encoder";
            public static String framingMs = "framingMs";
            public static String maxTxSecs = "maxTxSecs";
            public static String noHdrExt = "noHdrExt";
            public static String customRtpPayloadType = "customRtpPayloadType";
            public static String encoderName = "encoderName";
            public static String extensionSendInterval = "extensionSendInterval";
            public static String initialHeaderBurst = "initialHeaderBurst";
            public static String trailingHeaderBurst = "initialHeaderBurst";

        }

        public class NetworkTxOptions
        {
            public static String objectName = "txOptions";
            public static String priority = "priority";
            public static String ttl = "ttl";
        }

        public class Presence
        {
            public static String objectName = "presence";
            public static String format = "format";
            public static String intervalSecs = "intervalSecs";
            public static String listenOnly = "listenOnly";
            public static String minIntervalSecs = "minIntervalSecs";
        }

        public class PresenceDescriptor
        {
            public static String objectName = "presence";
            public static String self = "self";
            public static String comment = "comment";
            public static String custom = "custom";

            public class GroupItem
            {
                public static String arrayName = "groupAliases";
                public static String id = "groupId";
                public static String alias = "alias";
                public static String status = "status";
            }
        }

        public class Identity
        {
            public static String objectName = "identity";
            public static String nodeId = "nodeId";
            public static String userId = "userId";
            public static String displayName = "displayName";
            public static String type = "type";
            public static String format = "format";
            public static String avatar = "avatar";
        }

        public class Location
        {
            public static String objectName = "location";
            public static String longitude = "longitude";
            public static String latitude = "latitude";
            public static String altitude = "altitude";
            public static String direction = "direction";
            public static String speed = "speed";
        }

        public class Connectivity
        {
            public static String objectName = "connectivity";
            public static String type = "type";
            public static String strength = "strength";
            public static String rating = "rating";
        }

        public class Power
        {
            public static String objectName = "power";
            public static String source = "source";
            public static String state = "state";
            public static String level = "level";
        }

        public class RtpHeader
        {
            public static String objectName = "rtpHeader";
            public static String pt = "pt";
            public static String marker = "marker";
            public static String seq = "seq";
            public static String ssrc = "ssrc";
            public static String ts = "ts";
        }

        public class BlobInfo
        {
            public static String objectName = "blobHeader";
            public static String source = "source";
            public static String target = "target";
            public static String payloadType = "payloadType";
            public static String blobSize = "size";
            public static String rtpHeader = "rtpHeader";
        }

        public class TimelineEvent
        {
            public class Audio
            {
                public static String objectName = "audio";
                public static String ms = "ms";
                public static String samples = "samples";
            }

            public static String objectName = "event";
            public static String alias = "alias";
            public static String direction = "direction";
            public static String ended = "ended";
            public static String groupId = "groupId";
            public static String id = "id";
            public static String inProgress = "inProgress";
            public static String nodeId = "nodeId";
            public static String started = "started";
            public static String thisNodeId = "thisNodeId";
            public static String type = "type";
            public static String uri = "uri";
        }

        public class TimelineQuery
        {
            public static String maxCount = "maxCount";
            public static String mostRecentFirst = "mostRecentFirst";
            public static String startedOnOrAfter = "startedOnOrAfter";
            public static String endedOnOrBefore = "endedOnOrBefore";
            public static String onlyDirection = "onlyDirection";
            public static String onlyType = "onlyType";
            public static String onlyCommitted = "onlyCommitted";
            public static String onlyAlias = "onlyAlias";
            public static String onlyNodeId = "onlyNodeId";
            public static String onlyTxId = "onlyTxId";
            public static String sql = "sql";
        }

        public class TimelineReport
        {
            public static String success = "success";
            public static String errorMessage = "errorMessage";
            public static String started = "started";
            public static String ended = "ended";
            public static String execMs = "execMs";
            public static String records = "records";
            public static String events = "events";
            public static String count = "count";
        }
    }

    #region Callback delegate types
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void EngageVoidCallback(string eventExtraJson);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void EngageStringCallback(string s, string eventExtraJson);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void EngageString2Callback(string s1, string s2, string eventExtraJson);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void EngageStringAndIntCallback(string s, int i, string eventExtraJson);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void EngageString2AndInt2Callback(string s, int i1, int i2, string eventExtraJson);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void EngageStringAndArgvCallback(string s, IntPtr ptr, string eventExtraJson);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void EngageStringAndBlobCallback(string s, IntPtr ptr, int i, string eventExtraJson);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void EngageString2AndBlobCallback(string s, string j, IntPtr ptr, int i, string eventExtraJson);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void EngageStringAndTwoIntCallback(string s, int i1, int i2, string eventExtraJson);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void EngageLoggingCallback(int level, string tag, string message);
    #endregion

    #region Structures
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    private struct EngageGroupDescriptor_t
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = ENGAGE_MAX_GROUP_ID_SZ)]
        public byte[] id;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = ENGAGE_MAX_GROUP_NAME_SZ)]
        public byte[] name;

        [MarshalAs(UnmanagedType.U1)]
        public Boolean isEncrypted;

        [MarshalAs(UnmanagedType.U1)]
        public Boolean allowsFullDuplex;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    private struct EngageEvents_t
    {
        public EngageVoidCallback PFN_ENGAGE_ENGINE_STARTED;
        public EngageVoidCallback PFN_ENGAGE_ENGINE_STOPPED;

        public EngageStringCallback PFN_ENGAGE_RP_PAUSING_CONNECTION_ATTEMPT;
        public EngageStringCallback PFN_ENGAGE_RP_CONNECTING;
        public EngageStringCallback PFN_ENGAGE_RP_CONNECTED;
        public EngageStringCallback PFN_ENGAGE_RP_DISCONNECTED;
        public EngageStringAndTwoIntCallback PFN_ENGAGE_RP_ROUNDTRIP_REPORT;

        public EngageStringCallback PFN_ENGAGE_GROUP_CREATED;
        public EngageStringCallback PFN_ENGAGE_GROUP_CREATE_FAILED;
        public EngageStringCallback PFN_ENGAGE_GROUP_DELETED;

        public EngageStringCallback PFN_ENGAGE_GROUP_CONNECTED;
        public EngageStringCallback PFN_ENGAGE_GROUP_CONNECT_FAILED;
        public EngageStringCallback PFN_ENGAGE_GROUP_DISCONNECTED;

        public EngageStringCallback PFN_ENGAGE_GROUP_JOINED;
        public EngageStringCallback PFN_ENGAGE_GROUP_JOIN_FAILED;
        public EngageStringCallback PFN_ENGAGE_GROUP_LEFT;

        public EngageStringAndIntCallback PFN_ENGAGE_GROUP_MEMBER_COUNT_CHANGED;

        public EngageString2Callback PFN_ENGAGE_GROUP_NODE_DISCOVERED;
        public EngageString2Callback PFN_ENGAGE_GROUP_NODE_REDISCOVERED;
        public EngageString2Callback PFN_ENGAGE_GROUP_NODE_UNDISCOVERED;

        public EngageStringCallback PFN_ENGAGE_GROUP_RX_STARTED;
        public EngageStringCallback PFN_ENGAGE_GROUP_RX_ENDED;
        public EngageString2Callback PFN_ENGAGE_GROUP_RX_SPEAKERS_CHANGED;
        public EngageStringCallback PFN_ENGAGE_GROUP_RX_MUTED;
        public EngageStringCallback PFN_ENGAGE_GROUP_RX_UNMUTED;

        public EngageStringCallback PFN_ENGAGE_GROUP_TX_STARTED;
        public EngageStringCallback PFN_ENGAGE_GROUP_TX_ENDED;
        public EngageStringCallback PFN_ENGAGE_GROUP_TX_FAILED;
        public EngageStringCallback PFN_ENGAGE_GROUP_TX_USURPED_BY_PRIORITY;
        public EngageStringCallback PFN_ENGAGE_GROUP_MAX_TX_TIME_EXCEEDED;
        public EngageStringCallback PFN_ENGAGE_GROUP_TX_MUTED;
        public EngageStringCallback PFN_ENGAGE_GROUP_TX_UNMUTED;

        public EngageString2Callback PFN_ENGAGE_GROUP_ASSET_DISCOVERED;
        public EngageString2Callback PFN_ENGAGE_GROUP_ASSET_REDISCOVERED;
        public EngageString2Callback PFN_ENGAGE_GROUP_ASSET_UNDISCOVERED;

        public EngageVoidCallback PFN_ENGAGE_LICENSE_CHANGED;
        public EngageVoidCallback PFN_ENGAGE_LICENSE_EXPIRED;
        public EngageStringCallback PFN_ENGAGE_LICENSE_EXPIRING;

        public EngageStringCallback PFN_ENGAGE_GROUP_BLOB_SENT;
        public EngageStringCallback PFN_ENGAGE_GROUP_BLOB_SEND_FAILED;
        public EngageString2AndBlobCallback PFN_ENGAGE_GROUP_BLOB_RECEIVED;

        public EngageStringCallback PFN_ENGAGE_GROUP_RTP_SENT;
        public EngageStringCallback PFN_ENGAGE_GROUP_RTP_SEND_FAILED;
        public EngageString2AndBlobCallback PFN_ENGAGE_GROUP_RTP_RECEIVED;

        public EngageStringCallback PFN_ENGAGE_GROUP_RAW_SENT;
        public EngageStringCallback PFN_ENGAGE_GROUP_RAW_SEND_FAILED;
        public EngageStringAndBlobCallback PFN_ENGAGE_GROUP_RAW_RECEIVED;

        public EngageString2Callback PFN_ENGAGE_GROUP_TIMELINE_EVENT_STARTED;
        public EngageString2Callback PFN_ENGAGE_GROUP_TIMELINE_EVENT_UPDATED;
        public EngageString2Callback PFN_ENGAGE_GROUP_TIMELINE_EVENT_ENDED;
        public EngageString2Callback PFN_ENGAGE_GROUP_TIMELINE_REPORT;
        public EngageStringCallback PFN_ENGAGE_GROUP_TIMELINE_REPORT_FAILED;
        public EngageString2Callback PFN_ENGAGE_GROUP_TIMELINE_GROOMED;

        public EngageString2Callback PFN_ENGAGE_GROUP_HEALTH_REPORT;
        public EngageStringCallback PFN_ENGAGE_GROUP_HEALTH_REPORT_FAILED;

        public EngageStringCallback PFN_ENGAGE_BRIDGE_CREATED;
        public EngageStringCallback PFN_ENGAGE_BRIDGE_CREATE_FAILED;
        public EngageStringCallback PFN_ENGAGE_BRIDGE_DELETED;

        public EngageString2Callback PFN_ENGAGE_GROUP_STATS_REPORT;
        public EngageStringCallback PFN_ENGAGE_GROUP_STATS_REPORT_FAILED;

        public EngageString2AndInt2Callback PFN_ENGAGE_GROUP_RX_VOLUME_CHANGED;
        public EngageString2Callback PFN_ENGAGE_GROUP_RX_DTMF;

        public EngageVoidCallback PFN_ENGAGE_ENGINE_AUDIO_DEVICES_REFRESHED;

        public EngageStringCallback PFN_ENGAGE_GROUP_RECONFIGURED;
        public EngageStringCallback PFN_ENGAGE_GROUP_RECONFIGURATION_FAILED;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]     // 9 bytes
    public struct DataSeriesHeader
    {
        [MarshalAs(UnmanagedType.U1)]
        public byte t;

        [MarshalAs(UnmanagedType.U4)]
        public uint ts;

        [MarshalAs(UnmanagedType.U1)]
        public byte it;

        [MarshalAs(UnmanagedType.U1)]
        public byte im;

        [MarshalAs(UnmanagedType.U1)]
        public byte vt;

        [MarshalAs(UnmanagedType.U1)]
        public byte ss;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)] // 2 bytes
    public struct DataElementUint8
    {
        [MarshalAs(UnmanagedType.U1)]
        public byte ofs;

        [MarshalAs(UnmanagedType.U1)]
        public byte val;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)] // 3 bytes
    public struct DataElementUint16
    {
        [MarshalAs(UnmanagedType.U1)]
        public byte ofs;

        [MarshalAs(UnmanagedType.U2)]
        public ushort val;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)] // 5 bytes
    public struct DataElementUint32
    {
        [MarshalAs(UnmanagedType.U1)]
        public byte ofs;

        [MarshalAs(UnmanagedType.U4)]
        public uint val;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)] // 9 bytes
    public struct DataElementUint64
    {
        [MarshalAs(UnmanagedType.U1)]
        public byte ofs;

        [MarshalAs(UnmanagedType.U8)]
        public ulong val;
    }
    #endregion

    #region Library functions
    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageRegisterEventCallbacks(ref EngageEvents_t callbacks);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageInitialize(string enginePolicyConfiguration, string userIdentity, string tempStoragePath);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageShutdown();

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageStart();

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageStop();

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageCreateGroup(string jsonConfiguration);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageDeleteGroup(string id);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageJoinGroup(string id);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageLeaveGroup(string id);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageBeginGroupTx(string id, int txPriority, int txFlags);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageBeginGroupTxAdvanced(string id, string jsonParams);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageEndGroupTx(string id);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageSetGroupRxTag(string id, int tag);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageMuteGroupRx(string id);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageUnmuteGroupRx(string id);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageMuteGroupTx(string id);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageUnmuteGroupTx(string id);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageSetGroupRxVolume(string id, int left, int right);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr engageGetVersion();

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr engageGetHardwareReport();

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr engageGetActiveLicenseDescriptor();

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr engageGetLicenseDescriptor(string entitlement, string key, string activationCode, string manufacturerId);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageUpdateLicense(string entitlement, string key, string activationCode, string manufacturerId);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageUpdatePresenceDescriptor(string id, string jsonDescriptor, int forceBeacon);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageSendGroupBlob(string id, IntPtr blob, int blobSize, string jsonBlobParams);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageSendGroupRtp(string id, IntPtr payload, int payloadSize, string jsonRtpHeader);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageSendGroupRaw(string id, IntPtr raw, int rawSize, string jsonRtpHeader);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageQueryGroupTimeline(string id, string jsonParams);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageSetLogLevel(int level);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageSetLogTagExtension(string tagExtension);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageSetLoggingOutputOverride(EngageLoggingCallback hookFn);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageEnableSyslog(int enable);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageWatchdog(int enable);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageLogMsg(int level, string tag, string msg);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr engageGetNetworkInterfaceDevices();

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr engageGetAudioDevices();

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr engageGenerateMission(string keyPhrase, int audioGroupCount, string rallypointHost, string missionName);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr engageGenerateMissionUsingCertStore(string keyPhrase, int audioGroupCount, string rallypointHost, string missionName, string certStoreFn, string certStorePasswordHexByteString, string certStoreElement);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageSetMissionId(string missionId);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageOpenCertStore(string fileName, string passwordHexByteString);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr engageGetCertStoreDescriptor();

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageCloseCertStore();

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageSetCertStoreCertificatePem(string id, string certificatePem, string privateKeyPem, string tags);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageSetCertStoreCertificateP12(string id,IntPtr data, int size, string password, string tags);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageDeleteCertStoreCertificate(string id);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr engageGetCertStoreCertificatePem(string id);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr engageGetCertificateDescriptorFromPem(string pem);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageImportCertStoreElementFromCertStore(string id, string srcId, string srcFileName, string srcPasswordHexByteString, string tags);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr engageQueryCertStoreContents(string fileName, string passwordHexByteString);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageQueryGroupHealth(string id);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageQueryGroupStats(string id);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageCreateBridge(string jsonConfiguration);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageDeleteBridge(string id);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageEncrypt(IntPtr src, int size, IntPtr dst, string passwordHexByteString);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engageDecrypt(IntPtr src, int size, IntPtr dst, string passwordHexByteString);

    [DllImport(ENGAGE_DLL, CallingConvention = CallingConvention.Cdecl)]
    private static extern int engagePlatformNotifyChanges(string jsonChangesArray);

    #endregion

    #region Internal functions
    private static string[] stringArrayFromArgvStrPtrArray(IntPtr ptr)
    {
        string[] rc = null;
        int count = 0;
        IntPtr arrayPtr;
        IntPtr strPtr;

        // First count how many we have
        arrayPtr = ptr;
        while (true)
        {
            strPtr = Marshal.ReadIntPtr(arrayPtr);
            if (strPtr == (IntPtr)0)
            {
                break;
            }

            count++;
            arrayPtr += Marshal.SizeOf(arrayPtr);
        }

        // Now, allocate the array and copy over the strings
        if (count > 0)
        {
            rc = new string[count];

            int idx = 0;
            arrayPtr = ptr;
            while (true)
            {
                strPtr = Marshal.ReadIntPtr(arrayPtr);
                if (strPtr == (IntPtr)0)
                {
                    break;
                }

                rc[idx] = Marshal.PtrToStringAnsi(strPtr);

                arrayPtr += Marshal.SizeOf(arrayPtr);
                idx++;
            }
        }

        return rc;
    }

    private int registerEventCallbacks()
    {
        EngageEvents_t cb = new EngageEvents_t();

        cb.PFN_ENGAGE_ENGINE_STARTED = on_ENGAGE_ENGINE_STARTED;
        cb.PFN_ENGAGE_ENGINE_STOPPED = on_ENGAGE_ENGINE_STOPPED;
        cb.PFN_ENGAGE_ENGINE_AUDIO_DEVICES_REFRESHED = on_ENGAGE_ENGINE_AUDIO_DEVICES_REFRESHED;

        cb.PFN_ENGAGE_RP_PAUSING_CONNECTION_ATTEMPT = on_ENGAGE_RP_PAUSING_CONNECTION_ATTEMPT;
        cb.PFN_ENGAGE_RP_CONNECTING = on_ENGAGE_RP_CONNECTING;
        cb.PFN_ENGAGE_RP_CONNECTED = on_ENGAGE_RP_CONNECTED;
        cb.PFN_ENGAGE_RP_DISCONNECTED = on_ENGAGE_RP_DISCONNECTED;
        cb.PFN_ENGAGE_RP_ROUNDTRIP_REPORT = on_ENGAGE_RP_ROUNDTRIP_REPORT;

        cb.PFN_ENGAGE_GROUP_CREATED = on_ENGAGE_GROUP_CREATED;
        cb.PFN_ENGAGE_GROUP_CREATE_FAILED = on_ENGAGE_GROUP_CREATE_FAILED;
        cb.PFN_ENGAGE_GROUP_DELETED = on_ENGAGE_GROUP_DELETED;

        cb.PFN_ENGAGE_GROUP_CONNECTED = on_ENGAGE_GROUP_CONNECTED;
        cb.PFN_ENGAGE_GROUP_CONNECT_FAILED = on_ENGAGE_GROUP_CONNECT_FAILED;
        cb.PFN_ENGAGE_GROUP_DISCONNECTED = on_ENGAGE_GROUP_DISCONNECTED;

        cb.PFN_ENGAGE_GROUP_JOINED = on_ENGAGE_GROUP_JOINED;
        cb.PFN_ENGAGE_GROUP_JOIN_FAILED = on_ENGAGE_GROUP_JOIN_FAILED;
        cb.PFN_ENGAGE_GROUP_LEFT = on_ENGAGE_GROUP_LEFT;

        // TODO: FIXME!
        cb.PFN_ENGAGE_GROUP_MEMBER_COUNT_CHANGED = null;
        //cb.PFN_ENGAGE_GROUP_MEMBER_COUNT_CHANGED = on_ENGAGE_GROUP_MEMBER_COUNT_CHANGED;

        cb.PFN_ENGAGE_GROUP_RX_STARTED = on_ENGAGE_GROUP_RX_STARTED;
        cb.PFN_ENGAGE_GROUP_RX_ENDED = on_ENGAGE_GROUP_RX_ENDED;

        cb.PFN_ENGAGE_GROUP_RX_MUTED = on_ENGAGE_GROUP_RX_MUTED;
        cb.PFN_ENGAGE_GROUP_RX_UNMUTED = on_ENGAGE_GROUP_RX_UNMUTED;

        cb.PFN_ENGAGE_GROUP_TX_MUTED = on_ENGAGE_GROUP_TX_MUTED;
        cb.PFN_ENGAGE_GROUP_TX_UNMUTED = on_ENGAGE_GROUP_TX_UNMUTED;

        cb.PFN_ENGAGE_GROUP_RX_SPEAKERS_CHANGED = on_ENGAGE_GROUP_RX_SPEAKERS_CHANGED;

        cb.PFN_ENGAGE_GROUP_TX_STARTED = on_ENGAGE_GROUP_TX_STARTED;
        cb.PFN_ENGAGE_GROUP_TX_ENDED = on_ENGAGE_GROUP_TX_ENDED;
        cb.PFN_ENGAGE_GROUP_TX_FAILED = on_ENGAGE_GROUP_TX_FAILED;
        cb.PFN_ENGAGE_GROUP_TX_USURPED_BY_PRIORITY = on_ENGAGE_GROUP_TX_USURPED_BY_PRIORITY;
        cb.PFN_ENGAGE_GROUP_MAX_TX_TIME_EXCEEDED = on_ENGAGE_GROUP_MAX_TX_TIME_EXCEEDED;

        cb.PFN_ENGAGE_GROUP_NODE_DISCOVERED = on_ENGAGE_GROUP_NODE_DISCOVERED;
        cb.PFN_ENGAGE_GROUP_NODE_REDISCOVERED = on_ENGAGE_GROUP_NODE_REDISCOVERED;
        cb.PFN_ENGAGE_GROUP_NODE_UNDISCOVERED = on_ENGAGE_GROUP_NODE_UNDISCOVERED;

        cb.PFN_ENGAGE_GROUP_ASSET_DISCOVERED = on_ENGAGE_GROUP_ASSET_DISCOVERED;
        cb.PFN_ENGAGE_GROUP_ASSET_REDISCOVERED = on_ENGAGE_GROUP_ASSET_REDISCOVERED;
        cb.PFN_ENGAGE_GROUP_ASSET_UNDISCOVERED = on_ENGAGE_GROUP_ASSET_UNDISCOVERED;

        cb.PFN_ENGAGE_LICENSE_CHANGED = on_ENGAGE_LICENSE_CHANGED;
        cb.PFN_ENGAGE_LICENSE_EXPIRED = on_ENGAGE_LICENSE_EXPIRED;
        cb.PFN_ENGAGE_LICENSE_EXPIRING = on_ENGAGE_LICENSE_EXPIRING;

        cb.PFN_ENGAGE_GROUP_BLOB_SENT = on_ENGAGE_GROUP_BLOB_SENT;
        cb.PFN_ENGAGE_GROUP_BLOB_SEND_FAILED = on_ENGAGE_GROUP_BLOB_SEND_FAILED;
        cb.PFN_ENGAGE_GROUP_BLOB_RECEIVED = on_ENGAGE_GROUP_BLOB_RECEIVED;

        cb.PFN_ENGAGE_GROUP_RTP_SENT = on_ENGAGE_GROUP_RTP_SENT;
        cb.PFN_ENGAGE_GROUP_RTP_SEND_FAILED = on_ENGAGE_GROUP_RTP_SEND_FAILED;
        cb.PFN_ENGAGE_GROUP_RTP_RECEIVED = on_ENGAGE_GROUP_RTP_RECEIVED;

        cb.PFN_ENGAGE_GROUP_RAW_SENT = on_ENGAGE_GROUP_RAW_SENT;
        cb.PFN_ENGAGE_GROUP_RAW_SEND_FAILED = on_ENGAGE_GROUP_RAW_SEND_FAILED;
        cb.PFN_ENGAGE_GROUP_RAW_RECEIVED = on_ENGAGE_GROUP_RAW_RECEIVED;

        cb.PFN_ENGAGE_GROUP_TIMELINE_EVENT_STARTED = on_ENGAGE_GROUP_TIMELINE_EVENT_STARTED;
        cb.PFN_ENGAGE_GROUP_TIMELINE_EVENT_UPDATED = on_ENGAGE_GROUP_TIMELINE_EVENT_UPDATED;
        cb.PFN_ENGAGE_GROUP_TIMELINE_EVENT_ENDED = on_ENGAGE_GROUP_TIMELINE_EVENT_ENDED;

        cb.PFN_ENGAGE_GROUP_TIMELINE_REPORT = on_ENGAGE_GROUP_TIMELINE_REPORT;
        cb.PFN_ENGAGE_GROUP_TIMELINE_REPORT_FAILED = on_ENGAGE_GROUP_TIMELINE_REPORT_FAILED;
        cb.PFN_ENGAGE_GROUP_TIMELINE_GROOMED = on_ENGAGE_GROUP_TIMELINE_GROOMED;

        cb.PFN_ENGAGE_GROUP_HEALTH_REPORT = on_ENGAGE_GROUP_HEALTH_REPORT;
        cb.PFN_ENGAGE_GROUP_HEALTH_REPORT_FAILED = on_ENGAGE_GROUP_HEALTH_REPORT_FAILED;

        cb.PFN_ENGAGE_BRIDGE_CREATED = on_ENGAGE_BRIDGE_CREATED;
        cb.PFN_ENGAGE_BRIDGE_CREATE_FAILED = on_ENGAGE_BRIDGE_CREATE_FAILED;
        cb.PFN_ENGAGE_BRIDGE_DELETED = on_ENGAGE_BRIDGE_DELETED;

        cb.PFN_ENGAGE_GROUP_RX_VOLUME_CHANGED = on_ENGAGE_GROUP_RX_VOLUME_CHANGED;
        cb.PFN_ENGAGE_GROUP_RX_DTMF = on_ENGAGE_GROUP_RX_DTMF;

        cb.PFN_ENGAGE_GROUP_RECONFIGURED = on_ENGAGE_GROUP_RECONFIGURED;
        cb.PFN_ENGAGE_GROUP_RECONFIGURATION_FAILED = on_ENGAGE_GROUP_RECONFIGURATION_FAILED;

        return engageRegisterEventCallbacks(ref cb);
    }

    private string makeUserJsonConfiguration(string alias, string displayName, int txPriority)
    {
        StringBuilder sb = new StringBuilder();

        // Note:  Alias is maxed at 16, so if we precede it with "C#", we
        // can only use 14 hex characters for our random number portion of the ID.

        string myAlias = alias;
        string myDisplayName = displayName;
        int myTransmitPirority = txPriority;

        if (myAlias == null || myAlias.Length == 0)
        {
            myAlias = String.Format("C#{0:X14}", new Random().Next());
        }

        if (myDisplayName == null || myDisplayName.Length == 0)
        {
            myDisplayName = "C# User " + myAlias;
        }

        if(myTransmitPirority < 0)
        {
            myTransmitPirority = 0;
        }

        sb.Append("{");
            sb.Append("\"alias\":");
                sb.Append("\"" + myAlias + "\"");

            sb.Append(",\"displayName\":");
                sb.Append("\"" + myDisplayName + "\"");

            sb.Append(",\"txPriority\":");
                sb.Append(myTransmitPirority);
        sb.Append("}");

        return sb.ToString();
    }
    #endregion

    #region Member variables
    private static List<IEngineNotifications> _engineNotificationSubscribers = new List<IEngineNotifications>();
    private static List<IRallypointNotifications> _rallypointNotificationSubscribers = new List<IRallypointNotifications>();
    private static List<IGroupNotifications> _groupNotificationSubscribers = new List<IGroupNotifications>();
    private static List<ILicenseNotifications> _licenseNotificationSubscribers = new List<ILicenseNotifications>();
    private static List<IHumanBiometricsNotifications> _humanBiometricsNotifications = new List<IHumanBiometricsNotifications>();
    private static List<IBridgeNotifications> _bridgeNotificationSubscribers = new List<IBridgeNotifications>();
    private static List<ILoggingNotifications> _loggingNotificationSubscribers = new List<ILoggingNotifications>();
    #endregion

    #region Callback delegates
    private EngageVoidCallback on_ENGAGE_ENGINE_STARTED = (string eventExtraJson) =>
    {
        lock (_engineNotificationSubscribers)
        {
            foreach (IEngineNotifications n in _engineNotificationSubscribers)
            {
                n.onEngineStarted(eventExtraJson);
            }
        }
    };

    private EngageVoidCallback on_ENGAGE_ENGINE_STOPPED = (string eventExtraJson) =>
    {
        lock (_engineNotificationSubscribers)
        {
            foreach (IEngineNotifications n in _engineNotificationSubscribers)
            {
                n.onEngineStopped(eventExtraJson);
            }
        }
    };

    private EngageVoidCallback on_ENGAGE_ENGINE_AUDIO_DEVICES_REFRESHED = (string eventExtraJson) =>
    {
        lock (_engineNotificationSubscribers)
        {
            foreach (IEngineNotifications n in _engineNotificationSubscribers)
            {
                n.onEngineAudioDevicesRefreshed(eventExtraJson);
            }
        }
    };    

    private EngageStringCallback on_ENGAGE_RP_PAUSING_CONNECTION_ATTEMPT = (string id, string eventExtraJson) =>
    {
        lock (_rallypointNotificationSubscribers)
        {
            foreach (IRallypointNotifications n in _rallypointNotificationSubscribers)
            {
                n.onRallypointPausingConnectionAttempt(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_RP_CONNECTING = (string id, string eventExtraJson) =>
    {
        lock (_rallypointNotificationSubscribers)
        {
            foreach (IRallypointNotifications n in _rallypointNotificationSubscribers)
            {
                n.onRallypointConnecting(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_RP_CONNECTED = (string id, string eventExtraJson) =>
    {
        lock (_rallypointNotificationSubscribers)
        {
            foreach (IRallypointNotifications n in _rallypointNotificationSubscribers)
            {
                n.onRallypointConnected(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_RP_DISCONNECTED = (string id, string eventExtraJson) =>
    {
        lock (_rallypointNotificationSubscribers)
        {
            foreach (IRallypointNotifications n in _rallypointNotificationSubscribers)
            {
                n.onRallypointDisconnected(id, eventExtraJson);
            }
        }
    };

    private EngageStringAndTwoIntCallback on_ENGAGE_RP_ROUNDTRIP_REPORT = (string id, int rtMs, int rtRating, string eventExtraJson) =>
    {
        lock (_rallypointNotificationSubscribers)
        {
            foreach (IRallypointNotifications n in _rallypointNotificationSubscribers)
            {
                n.onRallypointRoundtripReport(id, rtMs, rtRating, eventExtraJson);
            }
        }
    };

    private EngageLoggingCallback on_ENGAGE_LOG_MESSAGE_HOOK_CALLBACK = (int level, string tag, string message) =>
    {
        lock (_loggingNotificationSubscribers)
        {
            foreach (ILoggingNotifications n in _loggingNotificationSubscribers)
            {
                n.onEngageLogMessage(level, tag, message);
            }
        }
    };


    private EngageStringCallback on_ENGAGE_GROUP_CREATED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupCreated(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_CREATE_FAILED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupCreateFailed(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_DELETED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupDeleted(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_CONNECTED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupConnected(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_CONNECT_FAILED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupConnectFailed(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_DISCONNECTED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupDisconnected(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_JOINED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupJoined(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_JOIN_FAILED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupJoinFailed(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_LEFT = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupLeft(id, eventExtraJson);
            }
        }
    };

    private EngageStringAndIntCallback on_ENGAGE_GROUP_MEMBER_COUNT_CHANGED = (string id, int newCount, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupMemberCountChanged(id, newCount, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_RX_STARTED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupRxStarted(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_RX_ENDED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupRxEnded(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_RX_MUTED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupRxMuted(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_RX_UNMUTED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupRxUnmuted(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_TX_MUTED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupTxMuted(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_TX_UNMUTED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupTxUnmuted(id, eventExtraJson);
            }
        }
    };

    private EngageString2Callback on_ENGAGE_GROUP_RX_SPEAKERS_CHANGED = (string id, string speakerjson, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupRxSpeakersChanged(id, speakerjson, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_TX_STARTED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupTxStarted(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_TX_ENDED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupTxEnded(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_TX_FAILED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupTxFailed(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_TX_USURPED_BY_PRIORITY = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupTxUsurpedByPriority(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_MAX_TX_TIME_EXCEEDED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupMaxTxTimeExceeded(id, eventExtraJson);
            }
        }
    };

    private EngageString2Callback on_ENGAGE_GROUP_NODE_DISCOVERED = (string id, string nodeJson, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupNodeDiscovered(id, nodeJson, eventExtraJson);
            }
        }
    };

    private EngageString2Callback on_ENGAGE_GROUP_NODE_REDISCOVERED = (string id, string nodeJson, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupNodeRediscovered(id, nodeJson, eventExtraJson);
            }
        }
    };

    private EngageString2Callback on_ENGAGE_GROUP_NODE_UNDISCOVERED = (string id, string nodeJson, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupNodeUndiscovered(id, nodeJson, eventExtraJson);
            }
        }
    };

    private EngageString2Callback on_ENGAGE_GROUP_ASSET_DISCOVERED = (string id, string nodeJson, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupAssetDiscovered(id, nodeJson, eventExtraJson);
            }
        }
    };

    private EngageString2Callback on_ENGAGE_GROUP_ASSET_REDISCOVERED = (string id, string nodeJson, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupAssetRediscovered(id, nodeJson, eventExtraJson);
            }
        }
    };

    private EngageString2Callback on_ENGAGE_GROUP_ASSET_UNDISCOVERED = (string id, string nodeJson, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupAssetUndiscovered(id, nodeJson, eventExtraJson);
            }
        }
    };

    private EngageVoidCallback on_ENGAGE_LICENSE_CHANGED = (string eventExtraJson) =>
    {
        lock (_licenseNotificationSubscribers)
        {
            foreach (ILicenseNotifications n in _licenseNotificationSubscribers)
            {
                n.onLicenseChanged(eventExtraJson);
            }
        }
    };

    private EngageVoidCallback on_ENGAGE_LICENSE_EXPIRED = (string eventExtraJson) =>
    {
        lock (_licenseNotificationSubscribers)
        {
            foreach (ILicenseNotifications n in _licenseNotificationSubscribers)
            {
                n.onLicenseExpired(eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_LICENSE_EXPIRING = (string secondsLeft, string eventExtraJson) =>
    {
        lock (_licenseNotificationSubscribers)
        {
            foreach (ILicenseNotifications n in _licenseNotificationSubscribers)
            {
                n.onLicenseExpiring(Double.Parse(secondsLeft), eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_BLOB_SENT = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupBlobSent(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_BLOB_SEND_FAILED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupBlobSendFailed(id, eventExtraJson);
            }
        }
    };

    private EngageString2AndBlobCallback on_ENGAGE_GROUP_BLOB_RECEIVED = (string id, string blobInfoJson, IntPtr blob, int blobSize, string eventExtraJson) =>
    {
        byte[] csBlob = new byte[blobSize];
        Marshal.Copy(blob, csBlob, 0, blobSize);

        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupBlobReceived(id, blobInfoJson, csBlob, blobSize, eventExtraJson);
            }
        }

        // Fire some additional goodies based on the blob info payload type
        JObject blobInfo = JObject.Parse(blobInfoJson);
        if(blobInfo != null)
        {
            int payloadType = (int)blobInfo["payloadType"];
            string nodeId = (string)blobInfo["source"];

            // Human biometrics ... ?
            if (payloadType == Engage.ENGAGE_BLOB_PT_ENGAGE_BINARY_HUMAN_BIOMETRICS)
            {
                lock (_humanBiometricsNotifications)
                {
                    if (_humanBiometricsNotifications.Count > 0)
                    {
                        // Get the array of biometrics items from the blob
                        string hbmJson = humanBiometricsFromBlob(csBlob);

                        if (hbmJson != null)
                        {
                            foreach (IHumanBiometricsNotifications n in _humanBiometricsNotifications)
                            {
                                n.onHumanBiometricsReceived(id, nodeId, hbmJson, eventExtraJson);
                            }
                        }
                    }
                }
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_RTP_SENT = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupRtpSent(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_RTP_SEND_FAILED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupRtpSendFailed(id, eventExtraJson);
            }
        }
    };

    private EngageString2AndBlobCallback on_ENGAGE_GROUP_RTP_RECEIVED = (string id, string rtpHeaderJson, IntPtr payload, int payloadSize, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            byte[] csPayload = new byte[payloadSize];
            Marshal.Copy(payload, csPayload, 0, payloadSize);

            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupRtpReceived(id, rtpHeaderJson, csPayload, payloadSize, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_RAW_SENT = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupRawSent(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_RAW_SEND_FAILED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupRawSendFailed(id, eventExtraJson);
            }
        }
    };

    private EngageStringAndBlobCallback on_ENGAGE_GROUP_RAW_RECEIVED = (string id, IntPtr raw, int rawSize, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            byte[] csRaw = new byte[rawSize];
            Marshal.Copy(raw, csRaw, 0, rawSize);

            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupRawReceived(id, csRaw, rawSize, eventExtraJson);
            }
        }
    };

    private EngageString2Callback on_ENGAGE_GROUP_TIMELINE_EVENT_STARTED = (string id, string eventJson, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupTimelineEventStarted(id, eventJson, eventExtraJson);
            }
        }
    };

    private EngageString2Callback on_ENGAGE_GROUP_TIMELINE_EVENT_UPDATED = (string id, string eventJson, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupTimelineEventUpdated(id, eventJson, eventExtraJson);
            }
        }
    };

    private EngageString2Callback on_ENGAGE_GROUP_TIMELINE_EVENT_ENDED = (string id, string eventJson, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupTimelineEventEnded(id, eventJson, eventExtraJson);
            }
        }
    };

    private EngageString2Callback on_ENGAGE_GROUP_TIMELINE_REPORT = (string id, string reportJson, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupTimelineReport(id, reportJson, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_TIMELINE_REPORT_FAILED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupTimelineReportFailed(id, eventExtraJson);
            }
        }
    };

    private EngageString2Callback on_ENGAGE_GROUP_TIMELINE_GROOMED = (string id, string eventListJson, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupTimelineGroomed(id, eventListJson, eventExtraJson);
            }
        }
    };

    private EngageString2Callback on_ENGAGE_GROUP_HEALTH_REPORT = (string id, string healthReportJson, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupHealthReport(id, healthReportJson, eventExtraJson);
            }
        }
    };   

    private EngageStringCallback on_ENGAGE_GROUP_HEALTH_REPORT_FAILED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupHealthReportFailed(id, eventExtraJson);
            }
        }
    };

    private EngageString2Callback on_ENGAGE_GROUP_STATS_REPORT = (string id, string statsReportJson, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupStatsReport(id, statsReportJson, eventExtraJson);
            }
        }
    };   

    private EngageStringCallback on_ENGAGE_GROUP_STATS_REPORT_FAILED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupStatsReportFailed(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_BRIDGE_CREATED = (string id, string eventExtraJson) =>
    {
        lock (_bridgeNotificationSubscribers)
        {
            foreach (IBridgeNotifications n in _bridgeNotificationSubscribers)
            {
                n.onBridgeCreated(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_BRIDGE_CREATE_FAILED = (string id, string eventExtraJson) =>
    {
        lock (_bridgeNotificationSubscribers)
        {
            foreach (IBridgeNotifications n in _bridgeNotificationSubscribers)
            {
                n.onBridgeCreateFailed(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_BRIDGE_DELETED = (string id, string eventExtraJson) =>
    {
        lock (_bridgeNotificationSubscribers)
        {
            foreach (IBridgeNotifications n in _bridgeNotificationSubscribers)
            {
                n.onBridgeDeleted(id, eventExtraJson);
            }
        }
    };

    private EngageString2AndInt2Callback on_ENGAGE_GROUP_RX_VOLUME_CHANGED = (string id, int leftLevelPerc, int rightLevelPerc, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupRxVolumeChanged(id, leftLevelPerc, rightLevelPerc, eventExtraJson);
            }
        }
    };

    private EngageString2Callback on_ENGAGE_GROUP_RX_DTMF = (string id, string dtmfJson, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupRxDtmf(id, dtmfJson, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_RECONFIGURED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupReconfigured(id, eventExtraJson);
            }
        }
    };

    private EngageStringCallback on_ENGAGE_GROUP_RECONFIGURATION_FAILED = (string id, string eventExtraJson) =>
    {
        lock (_groupNotificationSubscribers)
        {
            foreach (IGroupNotifications n in _groupNotificationSubscribers)
            {
                n.onGroupReconfigurationFailed(id, eventExtraJson);
            }
        }
    };

    #endregion

    #region Public functions
    public void subscribe(IEngineNotifications n)
    {
        lock(_engineNotificationSubscribers)
        {
            _engineNotificationSubscribers.Add(n);
        }
    }

    public void unsubscribe(IEngineNotifications n)
    {
        lock (_engineNotificationSubscribers)
        {
            _engineNotificationSubscribers.Remove(n);
        }
    }

    public void subscribe(IRallypointNotifications n)
    {
        lock (_rallypointNotificationSubscribers)
        {
            _rallypointNotificationSubscribers.Add(n);
        }
    }

    public void unsubscribe(IRallypointNotifications n)
    {
        lock (_rallypointNotificationSubscribers)
        {
            _rallypointNotificationSubscribers.Remove(n);
        }
    }

    public void subscribe(IGroupNotifications n)
    {
        lock (_groupNotificationSubscribers)
        {
            _groupNotificationSubscribers.Add(n);
        }
    }

    public void unsubscribe(IGroupNotifications n)
    {
        lock (_groupNotificationSubscribers)
        {
            _groupNotificationSubscribers.Remove(n);
        }
    }

    public void subscribe(ILicenseNotifications n)
    {
        lock (_licenseNotificationSubscribers)
        {
            _licenseNotificationSubscribers.Add(n);
        }
    }

    public void unsubscribe(ILicenseNotifications n)
    {
        lock (_licenseNotificationSubscribers)
        {
            _licenseNotificationSubscribers.Remove(n);
        }
    }

    public void subscribe(IHumanBiometricsNotifications n)
    {
        lock (_humanBiometricsNotifications)
        {
            _humanBiometricsNotifications.Add(n);
        }
    }

    public void unsubscribe(IHumanBiometricsNotifications n)
    {
        lock (_humanBiometricsNotifications)
        {
            _humanBiometricsNotifications.Remove(n);
        }
    }

    public void subscribe(IBridgeNotifications n)
    {
        lock (_bridgeNotificationSubscribers)
        {
            _bridgeNotificationSubscribers.Add(n);
        }
    }

    public void unsubscribe(IBridgeNotifications n)
    {
        lock (_bridgeNotificationSubscribers)
        {
            _bridgeNotificationSubscribers.Remove(n);
        }
    }

    public void subscribe(ILoggingNotifications n)
    {
        lock (_loggingNotificationSubscribers)
        {
            _loggingNotificationSubscribers.Add(n);

            if(_loggingNotificationSubscribers.size() == 1)
            {
                engageSetLoggingOutputOverride(on_ENGAGE_LOG_MESSAGE_HOOK_CALLBACK);
            }
        }
    }

    public void unsubscribe(ILoggingNotifications n)
    {
        lock (_loggingNotificationSubscribers)
        {
            _loggingNotificationSubscribers.Remove(n);
            
            if(_loggingNotificationSubscribers.size() == 0)
            {
                engageSetLoggingOutputOverride(null);
            }
        }
    }

    public int initialize(string enginePolicyConfiguration, string userIdentity, string tempStoragePath)
    {
        int rc;

        rc = registerEventCallbacks();
        if(rc != ENGAGE_RESULT_OK)
        {
            return rc;
        }

        return engageInitialize(enginePolicyConfiguration, userIdentity, tempStoragePath);
    }

    public int shutdown()
    {
        return engageShutdown();
    }

    public int start()
    {
        return engageStart();
    }

    public int stop()
    {
        return engageStop();
    }

    public int createGroup(string jsonConfiguration)
    {
        return engageCreateGroup(jsonConfiguration);
    }

    public int deleteGroup(string id)
    {
        return engageDeleteGroup(id);
    }

    public int joinGroup(string id)
    {
        return engageJoinGroup(id);
    }

    public int leaveGroup(string id)
    {
        return engageLeaveGroup(id);
    }

    public int beginGroupTx(string id, int txPriority, int txFlags)
    {
        return engageBeginGroupTx(id, txPriority, txFlags);
    }

    public int beginGroupTxAdvanced(string id, string jsonParams)
    {
        return engageBeginGroupTxAdvanced(id, jsonParams);
    }

    public int endGroupTx(string id)
    {
        return engageEndGroupTx(id);
    }

    public int setGroupRxTag(string id, int tag)
    {
        return engageSetGroupRxTag(id, tag);
    }

    public int muteGroupRx(string id)
    {
        return engageMuteGroupRx(id);
    }

    public int unmuteGroupRx(string id)
    {
        return engageUnmuteGroupRx(id);
    }

    public int muteGroupTx(string id)
    {
        return engageMuteGroupTx(id);
    }

    public int unmuteGroupTx(string id)
    {
        return engageUnmuteGroupTx(id);
    }

    public int setGroupRxVolume(string id, int left, int right)
    {
        return engageSetGroupRxVolume(id, left, right);
    }

    public int queryGroupTimeline(string id, string jsonParams)
    {
        return engageQueryGroupTimeline(id, jsonParams);
    }

    public int queryGroupHealth(string id)
    {
        return engageQueryGroupHealth(id);
    }

    public int queryGroupStats(string id)
    {
        return engageQueryGroupStats(id);
    }

    public int logMsg(int level, string tag, string msg)
    {
        return engageLogMsg(level, tag, msg);
    }

    public int setLogLevel(int level)
    {
        return engageSetLogLevel(level);
    }    

    public int setLogTagExtensionLevel(string tagExtension)
    {
        return engageSetLogTagExtension(tagExtension);
    }    

    public int enableSyslog(bool enable)
    {
        return engageEnableSyslog(enable ? 1 : 0);
    }    

    public int enableWatchdog(bool enable)
    {
        return engageWatchdog(enable ? 1 : 0);
    }    

    public String getVersion()
    {
        IntPtr ptr = engageGetVersion();

        if (ptr == IntPtr.Zero)
        {
            return null;
        }
        else
        {
            return Marshal.PtrToStringAnsi(ptr);
        }
    }

    public String getHardwareReport()
    {
        IntPtr ptr = engageGetHardwareReport();

        if (ptr == IntPtr.Zero)
        {
            return null;
        }
        else
        {
            return Marshal.PtrToStringAnsi(ptr);
        }
    }
    
    public String getActiveLicenseDescriptor()
    {
        IntPtr ptr = engageGetActiveLicenseDescriptor();

        if (ptr == IntPtr.Zero)
        {
            return null;
        }
        else
        {
            return Marshal.PtrToStringAnsi(ptr);
        }
    }

    public String getLicenseDescriptor(string entitlement, string key, string activationCode, string manufacturerId)
    {
        IntPtr ptr = engageGetLicenseDescriptor(entitlement, key, activationCode, manufacturerId);

        if (ptr == IntPtr.Zero)
        {
            return null;
        }
        else
        {
            return Marshal.PtrToStringAnsi(ptr);
        }
    }

    public int updateLicense(string entitlement, string key, string activationCode, string manufacturerId)
    {
        return engageUpdateLicense(entitlement, key, activationCode, manufacturerId);
    }

    public String getNetworkInterfaceDevices()
    {
        IntPtr ptr = engageGetNetworkInterfaceDevices();

        if (ptr == IntPtr.Zero)
        {
            return null;
        }
        else
        {
            return Marshal.PtrToStringAnsi(ptr);
        }
    }

    public String getAudioDevices()
    {
        IntPtr ptr = engageGetAudioDevices();

        if (ptr == IntPtr.Zero)
        {
            return null;
        }
        else
        {
            return Marshal.PtrToStringAnsi(ptr);
        }
    }

    public int setMissionId(string missionId)
    {
        return engageSetMissionId(missionId);
    }

    public int openCertStore(string fileName, string passwordHexByteString)
    {
        return engageOpenCertStore(fileName, passwordHexByteString);
    }

    public String getCertStoreDescriptor()
    {
        IntPtr ptr = engageGetCertStoreDescriptor();

        if (ptr == IntPtr.Zero)
        {
            return null;
        }
        else
        {
            return Marshal.PtrToStringAnsi(ptr);
        }
    }

    public int closeCertStore()
    {
        return engageCloseCertStore();
    }

    public int setCertStoreCertificatePem(string id, string certificatePem, string privateKeyPem, string tags)
    {
        return engageSetCertStoreCertificatePem(id, certificatePem, privateKeyPem, tags);
    }

    public int setCertStoreCertificateP12(string id, byte[] data, int size, string password, string tags)
    {
        IntPtr pinned_data = Marshal.AllocHGlobal(size);
        Marshal.Copy(data, 0, pinned_data, size);
        int rc = engageSetCertStoreCertificateP12(id, pinned_data, size, password, tags);
        Marshal.FreeHGlobal(pinned_data);

        return rc;
    }

    public int deleteCertStoreCertificate(string id)
    {
        return engageDeleteCertStoreCertificate(id);
    }

    public String getCertStoreCertificatePem(string id)
    {
        IntPtr ptr = engageGetCertStoreCertificatePem(id);

        if (ptr == IntPtr.Zero)
        {
            return null;
        }
        else
        {
            return Marshal.PtrToStringAnsi(ptr);
        }
    }

    public String getCertificateDescriptorFromPem(string pem)
    {
        IntPtr ptr = engageGetCertificateDescriptorFromPem(pem);

        if (ptr == IntPtr.Zero)
        {
            return null;
        }
        else
        {
            return Marshal.PtrToStringAnsi(ptr);
        }
    }
    
    public int importCertStoreElementFromCertStore(string id, string srcId, string srcFileName, string srcPasswordHexByteString, string tags)
    {
        return engageImportCertStoreElementFromCertStore(id, srcId, srcFileName, srcPasswordHexByteString, tags);
    }

    public String queryCertStoreContents(string fileName, string passwordHexByteString)
    {
        IntPtr ptr = engageQueryCertStoreContents(fileName, passwordHexByteString);

        if (ptr == IntPtr.Zero)
        {
            return null;
        }
        else
        {
            return Marshal.PtrToStringAnsi(ptr);
        }
    }

    public int encrypt(byte[] src, int size, out byte[] dst, string passwordHexByteString)
    {
        // Make a copy of the source
        IntPtr pinned_src = Marshal.AllocHGlobal(size);
        Marshal.Copy(src, 0, pinned_src, size);

        // Allocate a destination - at least the size of the source + 16 for AES sizing + 16 for IV
        IntPtr pinned_dst = Marshal.AllocHGlobal(size + 16 + 16);
       
        int bytesEncrypted = engageEncrypt(pinned_src, size, pinned_dst, passwordHexByteString);

        if(bytesEncrypted > 0)
        {
            dst = new byte[bytesEncrypted];
            Marshal.Copy(pinned_dst, dst, 0, bytesEncrypted);
        }
        else
        {
            dst = null;
        }        

        // Cleanup
        Marshal.FreeHGlobal(pinned_src);
        Marshal.FreeHGlobal(pinned_dst);

        return bytesEncrypted;
    }

    public int decrypt(byte[] src, int size, out byte[] dst, string passwordHexByteString)
    {
        // Make a copy of the source
        IntPtr pinned_src = Marshal.AllocHGlobal(size);
        Marshal.Copy(src, 0, pinned_src, size);

        // Allocate a destination - at least the size of the source + 16 for AES sizing
        IntPtr pinned_dst = Marshal.AllocHGlobal(size + 16);

        int bytesDecrypted = engageDecrypt(pinned_src, size, pinned_dst, passwordHexByteString);

        if (bytesDecrypted > 0)
        {
            dst = new byte[bytesDecrypted];
            Marshal.Copy(pinned_dst, dst, 0, bytesDecrypted);
        }
        else
        {
            dst = null;
        }

        // Cleanup
        Marshal.FreeHGlobal(pinned_src);
        Marshal.FreeHGlobal(pinned_dst);

        return bytesDecrypted;
    }

    public String generateMission(string keyPhrase, int audioGroupCount, string rallypointHost, string missionName)
    {
        IntPtr ptr = engageGenerateMission(keyPhrase, audioGroupCount, rallypointHost, missionName);

        if (ptr == IntPtr.Zero)
        {
            return null;
        }
        else
        {
            return Marshal.PtrToStringAnsi(ptr);
        }
    }

    public String generateMissionUsingCertStore(string keyPhrase, int audioGroupCount, string rallypointHost, string missionName, string certStoreFn, string certStorePasswordHexByteString, string certStoreElement)
    {
        IntPtr ptr = engageGenerateMissionUsingCertStore(keyPhrase, audioGroupCount, rallypointHost, missionName, certStoreFn, certStorePasswordHexByteString, certStoreElement);

        if (ptr == IntPtr.Zero)
        {
            return null;
        }
        else
        {
            return Marshal.PtrToStringAnsi(ptr);
        }
    }

    public int updatePresenceDescriptor(string id, string jsonDescriptor, bool forceBeacon)
    {
        return engageUpdatePresenceDescriptor(id, jsonDescriptor, (forceBeacon ? 1 : 0));
    }
    #endregion

    #region Helpers

    public static uint swapEndianness(uint x)
    {
        return ((x & 0x000000ff) << 24) +  // First byte
               ((x & 0x0000ff00) << 8) +   // Second byte
               ((x & 0x00ff0000) >> 8) +   // Third byte
               ((x & 0xff000000) >> 24);   // Fourth byte
    }

    public static string humanBiometricsFromBlob(byte[] blob)
    {
        JArray dataSeriesArray;

        try
        {
            // Create our enclosing human biometrics object - its a JSON array
            dataSeriesArray = new JArray();

            //  The total number of bytes we have available to us
            int bytesLeftInTheBlob = blob.Length;

            // Lock down the blob's memory
            GCHandle pinnedBlob = GCHandle.Alloc(blob, GCHandleType.Pinned);

            // Get the pointer to the start of the byte array
            IntPtr ptr = pinnedBlob.AddrOfPinnedObject();

            // Our blob may have multiple elements, so we'll loop
            while(bytesLeftInTheBlob > 0)
            {
                // Marshal in the header
                DataSeriesHeader hdr = (DataSeriesHeader)Marshal.PtrToStructure(ptr, typeof(DataSeriesHeader));

                // On little endian CPUs we need to swap from big endian (network byte order)
                if (BitConverter.IsLittleEndian)
                {
                    hdr.ts = swapEndianness(hdr.ts);
                }

                // Make a series element
                JObject se = new JObject();

                // Fill out its basic data
                se["t"] = (int)hdr.t;
                se["ts"] = hdr.ts;
                se["it"] = (int)hdr.it;
                se["im"] = (int)hdr.im;
                se["vt"] = (int)hdr.vt;

                // Jump forward by the size of the header (9 bytes) to point at the beginning of the data
                ptr = IntPtr.Add(ptr, 9);
                bytesLeftInTheBlob -= 9;

                // Now go through the data if we have any
                if (hdr.ss > 0)
                {
                    JArray s = new JArray();

                    if (hdr.vt == 1)
                    {
                        for (byte x = 0; x < hdr.ss; x++)
                        {
                            DataElementUint8 de = (DataElementUint8)Marshal.PtrToStructure(ptr, typeof(DataElementUint8));

                            s.Add((int)de.ofs);
                            s.Add((int)de.val);

                            ptr = IntPtr.Add(ptr, 2);
                            bytesLeftInTheBlob -= 2;
                        }
                    }
                    else if (hdr.vt == 2)
                    {
                        // TODO : process 16-bit numbers
                    }
                    else if (hdr.vt == 3)
                    {
                        // TODO : process 32-bit numbers
                    }
                    else if (hdr.vt == 4)
                    {
                        // TODO : process 64-bit numbers
                    }

                    // Plug the series array into the current seriesElement
                    se["s"] = s;
                }

                // Add the series element
                dataSeriesArray.Add(se);
            }

            pinnedBlob.Free();
        }
        catch(Exception e)
        {
            dataSeriesArray = null;
            Trace.WriteLine(e.StackTrace);
        }

        string rc = null;

        if(dataSeriesArray != null)
        {
            JObject hbmData = new JObject();
            hbmData["data"] = dataSeriesArray;
            rc = hbmData.ToString();
        }

        return rc;
    }

    public int platformNotifyChanges(string jsonChangesArray)
    {
        return engagePlatformNotifyChanges(jsonChangesArray);
    }

    #endregion
}
