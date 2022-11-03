
/*! \file EngageInterface.h
      \brief API functions, codes, and more

*/

//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

#ifndef EngageInterface_h
#define EngageInterface_h

#include <stdint.h>
#include "EngageConstants.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined(ENGAGE_API)
    #if defined(WIN32)
        #ifdef ENGAGE_EXPORTS
            // Windows needs dllexport to produce an import lib without a .DEF file
            #define ENGAGE_API  __declspec(dllexport) extern
        #else
            #define ENGAGE_API  extern
        #endif
    #else
        #define ENGAGE_API  __attribute__ ((visibility ("default")))
    #endif
#endif

#if !defined(__clang__)
    #define _Nullable
    #define _Nonnull
#endif

/** @brief Prototype for logging callbacks */
typedef void (*PFN_ENGAGE_LOG_HOOK)(int level, const char * _Nonnull tag, const char * _Nonnull message);

// Structures (all packed on 1-byte boundaries)
#pragma pack(push, 1)

/** @addtogroup eventHandlers Event Handlers
 *
 * A structure containing pointers to event handlers called by the Engage Engine.
 * @{
 */
/** @brief Event Handlers Struct */
typedef struct _EngageEvents_t
{
    /** @brief The Engine has started */
    void (* _Nullable PFN_ENGAGE_ENGINE_STARTED)(const char * _Nullable eventExtraJson);

    /** @brief The Engine has stopped */
    void (* _Nullable PFN_ENGAGE_ENGINE_STOPPED)(const char * _Nullable eventExtraJson);


    /** @brief Connection establishment to the RP has paused */
    void (* _Nullable PFN_ENGAGE_RP_PAUSING_CONNECTION_ATTEMPT)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief RP connection is underway */
    void (* _Nullable PFN_ENGAGE_RP_CONNECTING)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief RP has been connected */
    void (* _Nullable PFN_ENGAGE_RP_CONNECTED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief RP has disconnected */
    void (* _Nullable PFN_ENGAGE_RP_DISCONNECTED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief Fired on a roundtrip report */
    void (* _Nullable PFN_ENGAGE_RP_ROUNDTRIP_REPORT)(const char * _Nonnull pId, uint32_t rtMs, uint32_t rtQualityRating, const char * _Nullable eventExtraJson);


    /** @brief The group has been created */
    void (* _Nullable PFN_ENGAGE_GROUP_CREATED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief Group creation failed */
    void (* _Nullable PFN_ENGAGE_GROUP_CREATE_FAILED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief Group has been deleted */
    void (* _Nullable PFN_ENGAGE_GROUP_DELETED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);


    /** @brief Group has connected */
    void (* _Nullable PFN_ENGAGE_GROUP_CONNECTED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief Group connection has failed */
    void (* _Nullable PFN_ENGAGE_GROUP_CONNECT_FAILED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief Group has been disconnected */
    void (* _Nullable PFN_ENGAGE_GROUP_DISCONNECTED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);


    /** @brief Group has been joined */
    void (* _Nullable PFN_ENGAGE_GROUP_JOINED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief Attempt to join the group failed */
    void (* _Nullable PFN_ENGAGE_GROUP_JOIN_FAILED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief The group has been left */
    void (* _Nullable PFN_ENGAGE_GROUP_LEFT)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);


    /** @brief TODO: */
    void (* _Nullable PFN_ENGAGE_GROUP_MEMBER_COUNT_CHANGED)(const char * _Nonnull pId, size_t newCount, const char * _Nullable eventExtraJson);


    /** @brief A node has been discovered for the first time */
    void (* _Nullable PFN_ENGAGE_GROUP_NODE_DISCOVERED)(const char * _Nonnull pId, const char * _Nonnull nodeJson, const char * _Nullable eventExtraJson);

    /** @brief A node has been discovered again (i.e. not the first time) */
    void (* _Nullable PFN_ENGAGE_GROUP_NODE_REDISCOVERED)(const char * _Nonnull pId, const char * _Nonnull nodeJson, const char * _Nullable eventExtraJson);

    /** @brief A previously discovered node has disappeared */
    void (* _Nullable PFN_ENGAGE_GROUP_NODE_UNDISCOVERED)(const char * _Nonnull pId, const char * _Nonnull nodeJson, const char * _Nullable eventExtraJson);


    /** @brief RX has begun on the group */
    void (* _Nullable PFN_ENGAGE_GROUP_RX_STARTED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief RX on the group has ended */
    void (* _Nullable PFN_ENGAGE_GROUP_RX_ENDED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief The active speakers (talkers) on the group has changed */
    void (* _Nullable PFN_ENGAGE_GROUP_RX_SPEAKERS_CHANGED)(const char * _Nonnull pId, const char * _Nonnull groupTalkerJson, const char * _Nullable eventExtraJson);

    /** @brief RX for the group has been muted */
    void (* _Nullable PFN_ENGAGE_GROUP_RX_MUTED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief RX for the group has been unmuted */
    void (* _Nullable PFN_ENGAGE_GROUP_RX_UNMUTED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);


    /** @brief TX for the group has started */
    void (* _Nullable PFN_ENGAGE_GROUP_TX_STARTED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief TX for the group has ended */
    void (* _Nullable PFN_ENGAGE_GROUP_TX_ENDED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief TX attempt for the group has failed */
    void (* _Nullable PFN_ENGAGE_GROUP_TX_FAILED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief Active TX on the group has been usurped by another with higher priority */
    void (* _Nullable PFN_ENGAGE_GROUP_TX_USURPED_BY_PRIORITY)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief TX on the group has exceeded maximum TX time */
    void (* _Nullable PFN_ENGAGE_GROUP_MAX_TX_TIME_EXCEEDED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief TX for the group has been muted */
    void (* _Nullable PFN_ENGAGE_GROUP_TX_MUTED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief TX for the group has been unmuted */
    void (* _Nullable PFN_ENGAGE_GROUP_TX_UNMUTED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief An asset has been discovered for the first time */
    void (* _Nullable PFN_ENGAGE_GROUP_ASSET_DISCOVERED)(const char * _Nonnull pId, const char * _Nonnull nodeJson, const char * _Nullable eventExtraJson);

    /** @brief An asset has been discovered again (i.e. not the first time) */
    void (* _Nullable PFN_ENGAGE_GROUP_ASSET_REDISCOVERED)(const char * _Nonnull pId, const char * _Nonnull nodeJson, const char * _Nullable eventExtraJson);

    /** @brief A previously discovered asset has disappeared */
    void (* _Nullable PFN_ENGAGE_GROUP_ASSET_UNDISCOVERED)(const char * _Nonnull pId, const char * _Nonnull nodeJson, const char * _Nullable eventExtraJson);


    /** @brief The license has changed */
    void (* _Nullable PFN_ENGAGE_LICENSE_CHANGED)(const char * _Nullable eventExtraJson);

    /** @brief The license has expired */
    void (* _Nullable PFN_ENGAGE_LICENSE_EXPIRED)(const char * _Nullable eventExtraJson);

    /** @brief The license will expire soon */
    void (* _Nullable PFN_ENGAGE_LICENSE_EXPIRING)(const char * _Nonnull pSecsLeft, const char * _Nullable eventExtraJson);


    /** @brief A blob has been sent on the group */
    void (* _Nullable PFN_ENGAGE_GROUP_BLOB_SENT)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief The attempt to send a blob on the group has failed */
    void (* _Nullable PFN_ENGAGE_GROUP_BLOB_SEND_FAILED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief A blob has been received from the group */
    void (* _Nullable PFN_ENGAGE_GROUP_BLOB_RECEIVED)(const char * _Nonnull pId, const char * _Nonnull blobInfoJson, const uint8_t * _Nonnull blob, size_t blobSize, const char * _Nullable eventExtraJson);


    /** @brief Custom RTP has been sent to the group */
    void (* _Nullable PFN_ENGAGE_GROUP_RTP_SENT)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief Attempt to send custom RTP to the group has failed */
    void (* _Nullable PFN_ENGAGE_GROUP_RTP_SEND_FAILED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief Custom RTP has been received from the group */
    void (* _Nullable PFN_ENGAGE_GROUP_RTP_RECEIVED)(const char * _Nonnull pId, const char * _Nonnull rtpHeaderJson, const uint8_t * _Nonnull payload, size_t payloadSize, const char * _Nullable eventExtraJson);


    /** @brief A raw packet has been sent to the group */
    void (* _Nullable PFN_ENGAGE_GROUP_RAW_SENT)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief An attempt to send a raw packet to the group has failed */
    void (* _Nullable PFN_ENGAGE_GROUP_RAW_SEND_FAILED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief A raw packet has been received from the group */
    void (* _Nullable PFN_ENGAGE_GROUP_RAW_RECEIVED)(const char * _Nonnull pId, const uint8_t * _Nonnull raw, size_t rawSize, const char * _Nullable eventExtraJson);


    /** @brief A timeline event has started on the group */
    void (* _Nullable PFN_ENGAGE_GROUP_TIMELINE_EVENT_STARTED)(const char * _Nonnull pId, const char * _Nonnull eventJson, const char * _Nullable eventExtraJson);

    /** @brief A previously started time event for the group has changed */
    void (* _Nullable PFN_ENGAGE_GROUP_TIMELINE_EVENT_UPDATED)(const char * _Nonnull pId, const char * _Nonnull eventJson, const char * _Nullable eventExtraJson);

    /** @brief A previously started timeline event on the group has ended */
    void (* _Nullable PFN_ENGAGE_GROUP_TIMELINE_EVENT_ENDED)(const char * _Nonnull pId, const char * _Nonnull eventJson, const char * _Nullable eventExtraJson);

    /** @brief A timeline report for the group has been generated */
    void (* _Nullable PFN_ENGAGE_GROUP_TIMELINE_REPORT)(const char * _Nonnull pId, const char * _Nonnull timelineJson, const char * _Nullable eventExtraJson);

    /** @brief The attempt to generate a timeline report for the group has failed */
    void (* _Nullable PFN_ENGAGE_GROUP_TIMELINE_REPORT_FAILED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief The timeline for the group has been groomed */
    void (* _Nullable PFN_ENGAGE_GROUP_TIMELINE_GROOMED)(const char * _Nonnull pId, const char * _Nonnull eventListJson, const char * _Nullable eventExtraJson);    

    /** @brief A health report for the group has been generated */
    void (* _Nullable PFN_ENGAGE_GROUP_HEALTH_REPORT)(const char * _Nonnull pId, const char * _Nonnull healthReportJson, const char * _Nullable eventExtraJson);    

    /** @brief The attempt to generate a group health report has failed */
    void (* _Nullable PFN_ENGAGE_GROUP_HEALTH_REPORT_FAILED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief The bridge has been created */
    void (* _Nullable PFN_ENGAGE_BRIDGE_CREATED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief The attempt to create the bridge has failed */
    void (* _Nullable PFN_ENGAGE_BRIDGE_CREATE_FAILED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief The bridge has been deleted */
    void (* _Nullable PFN_ENGAGE_BRIDGE_DELETED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);
    
    /** @brief A statistics report for the group has been generated */
    void (* _Nullable PFN_ENGAGE_GROUP_STATS_REPORT)(const char * _Nonnull pId, const char * _Nonnull statsReportJson, const char * _Nullable eventExtraJson);    

    /** @brief The attempt to generate a statistics report for the group has failed */
    void (* _Nullable PFN_ENGAGE_GROUP_STATS_REPORT_FAILED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief The RX volume for the group has changed */
    void (* _Nullable PFN_ENGAGE_GROUP_RX_VOLUME_CHANGED)(const char * _Nonnull pId, int16_t leftLevelPerc, int16_t rightLevelPerc, const char * _Nullable eventExtraJson);

    /** @brief DTMF has been received on the group */
    void (* _Nullable PFN_ENGAGE_GROUP_RX_DTMF)(const char * _Nonnull pId, const char * _Nonnull dtmfJson, const char * _Nullable eventExtraJson);

    /** 
     * @brief Fired when the Engine has refreshed its audio device registry 
     * 
     * @param eventExtraJson Zero-terminated JSON string of array of @ref Configuration::AudioDeviceDescriptor objects
     * @see engageGetAudioDevices
     */
    void (* _Nullable PFN_ENGAGE_ENGINE_AUDIO_DEVICES_REFRESHED)(const char * _Nullable eventExtraJson);

    /** @brief Fired when a group has been reconfigured */
    void (* _Nullable PFN_ENGAGE_GROUP_RECONFIGURED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief Fired when a group reconfiguration attempt failed */
    void (* _Nullable PFN_ENGAGE_GROUP_RECONFIGURATION_FAILED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief Fired when an audio recording has started */
    void (* _Nullable PFN_ENGAGE_AUDIO_RECORDING_STARTED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief Fired the attempt to start an audio recording has failed */
    void (* _Nullable PFN_ENGAGE_AUDIO_RECORDING_FAILED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);

    /** @brief Fired when an audio recording has ended */    
    void (* _Nullable PFN_ENGAGE_AUDIO_RECORDING_ENDED)(const char * _Nonnull pId, const char * _Nullable eventExtraJson);    

    /** @brief Fired whenever the previously-registered high-resolution timer interval ticks.  'tickType' is 1, 2, or 3 for pre-tick, tick, and post-tick respectively */    
    void (* _Nullable PFN_ENGAGE_ON_HIGH_RESOLUTION_TIMER_TICK)(uint32_t tickType, uint32_t ticksSoFarLow, uint32_t ticksSoFarHigh, const char * _Nullable eventExtraJson);    
} EngageEvents_t;
/** @} */

#pragma pack(pop)

#if defined(WIN32)
ENGAGE_API void engageWin32LibraryInit();
ENGAGE_API void engageWin32LibraryDeinit();
#endif

// Library functions
#if !defined(ENGAGEJNI)

/**
 * @brief [SYNC] Sets the Engine's logging level.
 *
 * The logging level takes effect immediately and affects output to all targets
 * including the console, system debug log (Windows), and syslog.
 *
 * No events are generated by this API call.
 *
 * @see loggingLevels, engageEnableSyslog(), resultCodes
 *
 * @param level Desired debugging level
 * @return ENGAGE_RESULT_OK if successful
 */
ENGAGE_API int engageSetLogLevel(int level);


/**
 * @brief [SYNC] Sets the Engine's log tag extension.
 *
 * The tag extension takes effect immediately and affects output to all targets
 * including the console, system debug log (Windows), and syslog.
 *
 * No events are generated by this API call.
 *
 * @see engageEnableSyslog(), resultCodes
 *
 * @param tagExtension As string to be added to all tags.  A null or zero-length string clears the tag extension.
 * @return ENGAGE_RESULT_OK if successful
 */
ENGAGE_API int engageSetLogTagExtension(const char * _Nullable tagExtension);


/**
 * @brief [SYNC] Sets/clears the output override callback for log messages.
 *
 * No events are generated by this API call.
 *
 * @param pfnHook Pointer to the callback function. In a Java environment, this needs to be the null-terminated string name of a 
 *                Java method.  A non-null value causes all logging to be done exclusively through the override function - i.e.
 *                the Engine will not perform any output of it's own - either to consoles, files, or syslog.  A null/empoty value
 *                causes the Engine to take over output logging.
 * @return ENGAGE_RESULT_OK if successful
 */
ENGAGE_API int engageSetLoggingOutputOverride(const _Nullable PFN_ENGAGE_LOG_HOOK pfnHook);


/**
 * @brief [SYNC] Enables or disables logging to syslog.
 *
 * This output to syslog has no effect on platforms which do not support syslog
 * such as Windows.
 *
 * No events are generated by this API call.
 *
 * @see sysLogEnableDisable, engageSetLogLevel()
 *
 * @param enable ENGAGE_SYSLOG_ENABLE or ENGAGE_SYSLOG_DISABLE
 * @return ENGAGE_RESULT_OK if successful
 */
ENGAGE_API int engageEnableSyslog(int enable);


/**
 * @brief [SYNC] Registers application event handlers with the Engine
 *
 * The EngageEvents_t structure contains function pointers to callbacks defined in
 * the application for events generated by the Engine which the applications wishes to
 * handle.  Event handlers that are set to null will cause the Engine to generate a warning
 * in the log that the event handler is not found.
 *
 * No events are generated by this API call.
 *
 * @param pEvents pointer to an EngageEvents_t structure
 * @return ENGAGE_RESULT_OK if successful
 * @see eventCallbacks
 */
ENGAGE_API int engageRegisterEventCallbacks(const EngageEvents_t * _Nonnull pEvents);


/**
 * @brief [SYNC] Initializes the Engine.
 *
 * Call this function to initialize the Engine prior starting main operation.
 *
 * Note that calling this function does not start the Engine.  Rather, it prepares
 * the Engine for operation.  The application needs to call engageStart() to begin
 * Engine processing operations.
 *
 * No events are generated by this API call.
 *
 * @param enginePolicyConfiguration An optional JSON object of type @ref ConfigurationObjects::EnginePolicy
 * @param userIdentity An optional JSON object of type @ref ConfigurationObjects::Identity
 * @param tempStoragePath An optional read/write path for temporary storage
 *
 * @return ENGAGE_RESULT_OK if successful
 *
 * @see engageStart(), ConfigurationObjects::EnginePolicy, ConfigurationObjects::Identity
 */
ENGAGE_API int engageInitialize(const char * _Nullable enginePolicyConfiguration,
                                const char * _Nullable userIdentity,
                                const char * _Nullable tempStoragePath);


/**
 * @brief [SYNC] Shuts down the Engine
 *
 * Calling this function will carry out the inverse of engageInitialize(), leaving all
 * groups, destroying all objects and freeing up resources since the last call to
 * engageInitialize().  Typically this function is called as part of the application
 * shutdown procedure or when a full restart of the Engine is required.  While this function
 * will close down all resources, be aware that race conditions may arise if the process is
 * in an unstable state during shutdown.  Therefore, the application should, instead, perform
 * cleanup of its own by leaving all groups, and destroying all previously-created groups.
 *
 * Furthermore, as this is a synchronous function call, the application should take care
 * to call it from a thread that will not be impacted by a delay in processing.
 *
 * No events are generated by this API call.
 *
 * @return ENGAGE_RESULT_OK if successful
 * @see engageInitialize()
*/
ENGAGE_API int engageShutdown();


/**
 * @brief [ASYNC] Starts the Engine
 *
 * Once initialized through a call to engageInitialize(), the application must call this
 * function to start the Engine's internal processing logic.
 *
 * - PFN_ENGAGE_ENGINE_STARTED is fired when the request completes.
 *
 * As this is a synchronous function call, the application should take care
 * to call it from a thread that will not be impacted by a delay in processing.
 *
 * @return ENGAGE_RESULT_OK if successful
 * @see engageInitialize()
 */
ENGAGE_API int engageStart(void);


/**
 * @brief [ASYNC] Stops the Engine's internal processing
 *
 * This function is the inverse of engageStart().  It will cease all processing in
 * the Engine, including leaving all groups.  However, resources that were allocated
 * such as the creation of groups, will not be destroyed by this call.
 *
 * - ENGAGE_ENGINE_STOPPED is fired when the request completes.
 *
 * @return ENGAGE_RESULT_OK if successful
 * @see engageStart()
 */
ENGAGE_API int engageStop(void);


/**
 * @brief [ASYNC] Creates a group object
 *
 * Requests creation of a group object according to elements in the JSON
 * configuration parameter.  This function is different from other group-related
 * APIs in that it does not have a group ID as a parameter.  Rather, the group ID
 * along with other configuration elements for the group are contained in the JSON
 * configuration.  In contrast, other group-related APIs need only use the group ID
 * as the identifier and, by then, the object has already been created and is keyed
 * by the ID.
 *
 * - ENGAGE_GROUP_CREATED is fired when the group is created.
 * - ENGAGE_GROUP_CREATE_FAILED is fired if the request fails due to invalid configuration or
 *   due to the fact that the group object already exists.
 *
 * @param jsonConfiguration A JSON object of type @ref ConfigurationObjects::Group
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see engageDeleteGroup(), ConfigurationObjects::Group
 */
ENGAGE_API int engageCreateGroup(const char * _Nonnull jsonConfiguration);


/**
 * @brief [ASYNC] Deletes a previously-created group object
 *
 * The Engine will delete the group, leaving it if necessary, and releasing any resources
 * allocated for the object.  If the group to be deleted does not exist, the Engine will
 * simply continue and not report an error.
 *
 * - ENGAGE_GROUP_DELETED is fired when the group is deleted.
 *
 * @param id The ID of the group to be deleted
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see engageCreateGroup()
 */
ENGAGE_API int engageDeleteGroup(const char * _Nonnull id);


/**
 * @brief [ASYNC] Joins a group
 *
 * The Engine will join the group, including connection to a multicast network or
 * a unicast Rallypoint link as dictated by the group's configuration.
 *
 * Note that joining a group does not mean that it is ready to receive and/or transmit
 * data and/or audio.  Rather, this API is a request to the Engine to begin (and continue)
 * processing for the group - including retries needed for connectivity to a local multicast
 * network or Rallypoint link.
 *
 * Only once a joined group is actually connected to the network infrastructure will the
 * ENGAGE_GROUP_CONNECTED event fire.  Applications should therefore update their UIs according
 * to connection-related events rather than join-related events; unless, of course, join failures
 * are encountered.
 *
 * - ENGAGE_GROUP_JOINED is fired when the group has been joined.
 * - ENGAGE_GROUP_JOIN_FAILED is fired if the Engine fails to begin join processing for the group
 *
 * @param id The ID of the group to be joined
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see engageLeaveGroup()
 */
ENGAGE_API int engageJoinGroup(const char * _Nonnull id);


/**
 * @brief [ASYNC] Leaves a group
 *
 * This API call will result in the Engine leaving a group, freeing up runtime resources needed
 * for the group's operation.  The group object remains, however, and can be joined again at
 * the application's discretion.
 *
 * - ENGAGE_GROUP_DISCONNECTED is fired whether the group had successfully connected or not
 * - ENGAGE_GROUP_LEFT is fired
 *
 * @param id The ID of the group to be left
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see engageJoinGroup()
 */
ENGAGE_API int engageLeaveGroup(const char * _Nonnull id);


/**
 * @brief [ASYNC] Begin audio transmission on a group
 *
 * Begin audio transmission on a group that is configured for audio (as opposed to data or raw
 * group types).
 *
 * The application may also set a priority level between 0 and 255 for the transmission burst.  This
 * is typically used when a high-priority user needs to override other people transmitting on the
 * group who have lower priorities.  Upon receiving prioritized audio packets, other Engage Engines
 * will determine whether their user, if they are already transmitting, should have their transmission
 * usurped or allowed to continue.
 *
 * Applications should be judicious in how they utilize the transmit priority field as unplanned
 * operation can lead to a confusing and complex user experience in large systems.  As a guideline,
 * applications should generally use 0 as the transmit priority for all users and, if the need arises to
 * create priority levels, that those levels be assigned to users according to their role and position
 * in the organization and/or the task at hand.
 *
 * In addition to transmit priority, the application may also embed bit flags indicating the nature of
 * the transmission.  At this time @ref ENGAGE_TXFLAG_EMERGENCY and @ref ENGAGE_TXFLAG_AUTOMATED_SYSTEM are defined.
 *
 * @ref ENGAGE_TXFLAG_EMERGENCY indicates that the user/entity is transmitting in emergency mode - i.e. the
 * user is in distress or encountering some other kind of emergency.  Be aware that this flag is not
 * associated with the transmit priority value.  Rather, it is meant to be used as an indicator on receiving
 * user interfaces that the transmitting entity is an emergency state of some sort.
 *
 * The @ref ENGAGE_TXFLAG_AUTOMATED_SYSTEM flag indicates that the transmitting entity is not a human but rather
 * a mechanical or electronic - such as an AI or other automated system.
 *
 * Note that transmit priority and transmit flags are only included in the transmitted stream
 * if the group allows for RTP header extensions.  For audio groups that have RTP header extensions enabled,
 * the header extension includes the priority and the flags, along with the 16-character alias
 * defined in the Identity JSON object passed in the group creation JSON or, if that was empty, the
 * Identity object passed to engageInitialize().
 *
 * - ENGAGE_GROUP_TX_STARTED is fired when transmission begins
 * - ENGAGE_GROUP_TX_FAILED is fired if the attempt to transmit fails
 * - ENGAGE_GROUP_TX_USURPED_BY_PRIORITY is fired if a higher-priority transmission is received
 * - ENGAGE_GROUP_MAX_TX_TIME_EXCEEDED is fired if/when the maximum transmit time for the group is exceeded
 * - ENGAGE_GROUP_TX_ENDED when transmission ends
 *
 * @param id The ID of the group to transmit to
 * @param txPriority A transmit priority of 0-225 for this transmission
 * @param txFlags 0 | ENGAGE_TXFLAG_EMERGENCY | ENGAGE_TXFLAG_AUTOMATED_SYSTEM
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see txFlags, engageInitialize(), engageBeginGroupTxAdvanced(), engageEndGroupTx(), ConfigurationObjects::Group
 */
ENGAGE_API int engageBeginGroupTx(const char * _Nonnull id, int txPriority, uint32_t txFlags);


/**
 * @brief [ASYNC] Begin audio transmission on a group with additional parameters
 *
 * An extension of engageBeginGroupTx() with parameters incorporated in a JSON object of
 * type AdvancedTxParams.  As with engageBeginGroupTx(), these parameters only have meaning if
 * the audio group is configured for RTP header extensions.
 *
 * Note that transmission of audio generates a great deal of network traffic and therefore this API
 * call, if AdvancedTxParams.includeNodeId is specified, will include an additional 16 bytes of data
 * for every packet that carries the header.  If the AdvancedTxParams.alias is also included that will
 * add yet another 16 bytes of data to the RTP packet - driving up bandwidth usage significantly.
 *
 * Please refer to the "Engage Best Practices" document for more information on how to make the most
 * efficient use of network resources.
 *
 * @param id The ID of the group to transmit to
 * @param jsonParams JSON parameters of type @ref ConfigurationObjects::AdvancedTxParams
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see txFlags, engageInitialize(), engageBeginGroupTx(), engageEndGroupTx(), ConfigurationObjects::Group, ConfigurationObjects::AdvancedTxParams
 */
ENGAGE_API int engageBeginGroupTxAdvanced(const char * _Nonnull id, const char * _Nonnull jsonParams); /* TODO: define JSON */


/**
 * @brief [ASYNC] Ends transmission on an audio group
 *
 * - ENGAGE_GROUP_TX_ENDED when transmission ends
 *
 * @param id The ID of the group on which to end transmission
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see txFlags, engageInitialize(), engageBeginGroupTx(), engageEndGroupTx()
 */
ENGAGE_API int engageEndGroupTx(const char * _Nonnull id);


/**
 * @brief [ASYNC] Sets/clear a audio stream subchannel tag
 *
 * This API instructs the Engine to process RTP streams that have a particular subchannel
 * tag associated with them.  A subchannel is a means by which an existing group (including all
 * its network connections and infrastructure) can be used to transmit traffic that, while received
 * by all Engage Engines on the "main" group, is ignored by Engines that are not actively processing
 * the subchannel tag.  Subchannels are typically used to create "private" streams within existing
 * streams - generally for purposes of a private communication between a subset of the users on an
 * Engage group.  For example, assuming an application has the means by which to coordinate with
 * other instances of itself a numeric identifier (between 1 and 65535) to be used as a "private
 * group", and the tag is set in transmission by calling engageBeginGroupTxAdvanced() with that
 * value; other instances that call engageSetGroupRxTag() with that tag value will "hear" that
 * traffic.  Instances that are not aware of the tag will simply ignore the tagged packets.
 *
 * While this is not a foolproof method by which to create "private" streams between users, it
 * is very efficient and substantially less complicated than coordinating the creation of groups
 * dynamically across an enterprise network that do not run the risk of interfering with hitherto
 * unknown multicast address and port numbers.
 *
 * To clear the tag on the group, call this function passing 0 as the tag value.
 *
 * No events are generated by this API call.
 *
 * @param id The ID of the group on which to set the subchannel tag
 * @param tag The numeric value of the tag (1-65535), 0 to clear the tag
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see engageBeginGroupTxAdvanced()
 */
ENGAGE_API int engageSetGroupRxTag(const char * _Nonnull id, uint16_t tag);


/**
 * @brief [ASYNC] Mutes play-out of received audio
 *
 * Following successful execution of the request, the audio for the group will
 * not be played to the audio output device.  However, received audio will continue to
 * buffer inside the active transaction if local buffering has been enabled for
 * the group.
 *
 * - ENGAGE_GROUP_RX_MUTED
 *
 * @param id The ID of the group to be muted
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see engageUnmuteGroupRx()
 */
ENGAGE_API int engageMuteGroupRx(const char * _Nonnull id);


/**
 * @brief [ASYNC] Unmutes play-out of received audio
 *
 * Following successful execution of the request, the audio for the group will
 * be played to the audio output device.
 *
 * - ENGAGE_GROUP_RX_UNMUTED
 *
 * @param id The ID of the group to be unmuted
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see engageMuteGroupRx()
 */
ENGAGE_API int engageUnmuteGroupRx(const char * _Nonnull id);


/**
 * @brief [ASYNC] Mutes microphone audio capture
 *
 * Following successful execution of the request, audio captured from the microphone will
 * not be tramsitted to the network or written to the timeline (if any).
 *
 * - ENGAGE_GROUP_TX_MUTED
 *
 * @param id The ID of the group TX to be muted
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see engageUnmuteGroupTx()
 */
ENGAGE_API int engageMuteGroupTx(const char * _Nonnull id);


/**
 * @brief [ASYNC] Unmutes microphone audio capture
 *
 * Following successful execution of the request, the audio captured for the group will
 * be transmitted to the network and written to the timeline (if any)
 *
 * - ENGAGE_GROUP_TX_UNMUTED
 *
 * @param id The ID of the group TX to be unmuted
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see engageMuteGroupTx()
 */
ENGAGE_API int engageUnmuteGroupTx(const char * _Nonnull id);

/**
 * @brief [ASYNC] Sets the audio play-out volume of the group
 *
 * This API call changes the gain of the left and right audio play-out levels
 * relative to the current hardware-defined play-out level by applying the
 * requested levels as a percentage.  Gain levels of 100 percent result in
 * the volume remaining unchanged.  Gain levels less that 100 percent cause
 * the volume to be decreased, whereas levels greater than 100 cause the volume
 * to be increased.
 *
 * The Engine assumes the audio hardware to be capable of stereo output - therefore
 * the reason for separate left and right levels.  However, on devices that do not
 * support stereo output, the hardware typically merges the stereo levels to obtain
 * an averaged value for mono output.
 *
 * On hardware that does support stereo, setting different levels for left and right
 * result in stereo panning - allowing, for example, one group's audio to be played
 * to the left channel with another group's audio played to the right channel.
 *
 * Note: This call does NOT change the physical hardware audio level as most platforms
 * do not allow for this programmatically as doing so would interfere with audio
 * levels in other applications.
 *
 * No events are generated by this API call.
 *
 * @param id The ID of the group
 * @param left Left volume level
 * @param right Right volume level
 * @return ENGAGE_RESULT_OK if the submission request was successful
 */
ENGAGE_API int engageSetGroupRxVolume(const char * _Nonnull id, int left, int right);


/**
 * @brief [ASYNC] Updates the application-defined elements of a presence descriptor
 *
 * Call this function to pass into the Engine that information which the application
 * wishes to be transmitted as part of the Engine's normal presence declarations.  The next
 * time the Engine sends its presence descriptor on the group specified, the new data will
 * be included.
 *
 * As presence declarations are only sent periodically according to a bandwidth-sensitive
 * algorithm, the updated presence descriptor may not be sent right away.  To request the
 * Engine to send the updated presence descriptor as soon as possible, set forceBeacon to 1.
 *
 * No events are generated by this API call.
 *
 * @param id The ID of the group
 * @param descriptorJson A required JSON object of type @ref ConfigurationObjects::PresenceDescriptor
 * @param forceBeacon 1 to force the Engine to send the presence descriptor immediately
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see jsonPresenceDescriptor
 */
ENGAGE_API int engageUpdatePresenceDescriptor(const char * _Nonnull id, const char * _Nonnull descriptorJson, int forceBeacon);


/**
 * @brief [SYNC] Encrypt data using the Engine's cryptography module
 *
 * Applications can use this function call to encrypt data using the Engine's currently-
 * active symmetric encryption cipher.  Typically this cipher is AES256 operating in CBC mode but
 * may be different if a market- or customer-specific cryptographic module is in use.
 *
 * It is important that the calling application allocate sufficient space in the destination to
 * accommodate the encrypted output.
 *
 * In the case of AES256-CBC, the minimum output size is in block sizes of 16 bytes for the actual encrypted
 * data PLUS and additional 16 bytes for the CBC Initialization Vector.
 *
 * For example: assuming a source array size of 12 bytes to be encrypted, the Engine will output
 * 32 bytes to the destination.  The first 16 bytes is the Initialization Vector (always 16 bytes for
 * AES), the remaining 16 bytes is the original 12 bytes rounded up to a block size of 16.
 *
 * For other source sizes, the output size will vary accordingly, for instance:
 *
 * An input of 3 bytes results in 32 bytes (3 rounded to 16 + 16 for IV)
 * An input of 15 bytes results in 32 bytes (15 rounded to 16 + 16 for IV)
 * An input of 16 results in 48 bytes (16 rounded to 32 + 16 for IV)
 * An input of 104 results in 128 bytes (104 rounded to 112 + 16 for IV)
 *
 * The password parameter is the hexadecimal representation of the binary data used as basis key
 * material in generating the symmetric key used for encryption.  It is important to understand that
 * this is NOT the actual encryption key, rather, it is used as input to a key derivation function
 * in the Engine's cryptography module that generates a symmetric key using a combination of the provided
 * password and an internal salt value.  The password is a required value and, while it may be as short as
 * 1 byte; it is recommended that the application utilize passwords that are much longer.
 *
 * For more information concerning security and encryption, consult the "Security" section of
 * the Engage Developer's Guide.
 *
 * No events are generated by this API call.
 *
 * @param src Pointer to the source byte array to be encrypted
 * @param size Number of bytes in the source byte array
 * @param dst Pointer to the destination where encrypted data is to be output
 * @param passwordHexByteString Hexadecimal representation of the password to be used for key generation
 * @return Number of bytes output to the destination (<= 0 if an error occurred)
 * @see engageDecrypt()
 */
ENGAGE_API int engageEncrypt(const uint8_t * _Nonnull src, size_t size, uint8_t * _Nonnull dst, const char * _Nonnull passwordHexByteString);


/**
 * @brief [SYNC] Decrypt data using the Engine's cryptography module
 *
 * The inverse of engageEncrypt().  Consult the discussion for engageEncrypt() for more information.
 *
 * No events are generated by this API call.
 *
 * @param src Pointer to the source byte array to be decrypted
 * @param size Number of bytes in the source byte array
 * @param dst Pointer to the destination where decrypted data is to be output
 * @param passwordHexByteString Hexadecimal representation of the password to be used for key generation
 * @return Number of bytes output to the destination (<= 0 if an error occurred)
 * @see engageEncrypt()
 */
ENGAGE_API int engageDecrypt(const uint8_t * _Nonnull src, size_t size, uint8_t * _Nonnull dst, const char * _Nonnull passwordHexByteString);


/**
 * @brief [SYNC] Returns the version of the Engine
 *
 * The version of the Engine is in the format \<major>.\<minor>.\<build>.  For example: 1.17.8907.
 *
 * Major version changes represent substantial changes in the Engine such as entirely new capabilities and
 * generally change very seldom.  Minor version changes represent incremental updates such as bug fixes, performance
 * enhancements, and API changes that do not require substantial changes in consuming applications.  The minor
 * version resets to 0 whenever a major version change is made.
 *
 * The build number is an internal tracking number and increments each time the Engine is fully rebuilt for release purposes.
 *
 * No events are generated by this API call.
 *
 * @return Zero-terminated string in the format \<major>.\<minor>.\<build>
 */
ENGAGE_API const char * _Nonnull engageGetVersion();


/**
 * @brief [SYNC] Returns the currently-active license descriptor
 *
 * Call this function to obtain a JSON descriptor of the license currently in use by the Engine.  If
 * the Engine is operating in unlicensed mode, a descriptor is still returned albeit with most elements
 * empty.
 *
 * No events are generated by this API call.
 *
 * @return Zero-terminated string representing a JSON object of type ConfigurationObjects::LicenseDescriptor
 * <P>
 * Example: @include[doc] examples/LicenseDescriptor.json
 * </P>
 * @see ConfigurationObjects::LicenseDescriptor, engageUpdateLicense(), engageUpdateLicense()
 */
ENGAGE_API const char * _Nonnull engageGetActiveLicenseDescriptor();


/**
 * @brief [SYNC] Returns a license descriptor for the parameters passed in
 *
 * Call this function to obtain a JSON descriptor of the license that would be used based on the supplied parameters.
 *
 * Typically applications use this function to determine whether license information is valid in order to
 * provide feedback to a user.
 *
 * No events are generated by this API call.
 *
 * @param entitlement The application's entitlement key
 * @param key The license key
 * @param activationCode The activation code for the license on the current device (if any)
 * @param manufacturerId OEM-specific manufacturer ID or null foir default
 * @return Zero-terminated string representing a JSON object of type ConfigurationObjects::LicenseDescriptor
 * <P>
 * Example: @include[doc] examples/LicenseDescriptor.json
 * </P>
 * @see ConfigurationObjects::LicenseDescriptor, engageGetActiveLicenseDescriptor(), engageUpdateLicense()
 */
ENGAGE_API const char * _Nonnull engageGetLicenseDescriptor(const char * _Nonnull entitlement, const char * _Nonnull key, const char * _Nullable activationCode, const char * _Nullable manufacturerId);


/**
 * @brief [ASYNC] Update the active license with new parameters
 *
 * Call this function to install a new license and/or activation code in the Engine.  Assuming the parameters are valid,
 * the Engine will put the new license into effect right away - regardless of whether the license is active, expired, or
 * expiring in the near future.
 *
 * - ENGAGE_LICENSE_CHANGED
 * - ENGAGE_LICENSE_EXPIRED if the license has expired
 * - ENGAGE_LICENSE_EXPIRING if the license will expire soon
 *
 * @param entitlement The application's entitlement key
 * @param key The license key
 * @param activationCode The activation code for the license on the current device (if any)
 * @param manufacturerId OEM-specific manufacturer ID or null foir default
 * @return ENGAGE_RESULT_OK if the submission request was successful
 */
ENGAGE_API int engageUpdateLicense(const char * _Nonnull entitlement, const char * _Nonnull key, const char * _Nullable activationCode, const char * _Nullable manufacturerId);


/**
 * @brief [ASYNC] Send an application-structured byte array to a group as a specialized RTP payload
 *
 * Call this function to have the Engine transmit an application-defined byte array on a group that encapsulates data in
 * RTP packets - i.e. groups of type "Audio".
 *
 * In order to carry out this operation, the Engine wraps the transmitted data in an RTP packet - i.e. with an RTP
 * header and, on the receiving end, processes that RTP packet.  As a result, the application must specify a valid
 * RTP payload type for the RTP header defined in the *jsonParams* parameters - all other fields in that object are
 * optional.  Once the RTP packet is created, it follows the same path through the Engine as other RTP packets - that
 * being through the COMSEC encryption layer if necessary if the group is encrypted and, then, onward to the network.  If
 * the network connection is via multicast, the packet is queued and sent to the multicast otherwise, in a unicast
 * environment, the packet is forwarded through the Rallpoint link via TLS to a Rallypoint node/
 *
 * This function is only valid for groups of type "Audio" - as specified in the call to engageCreateGroup().
 *
 * In order to receive payloads sent via RTP, receiving Engine instances must be passed the payload ID in the call to
 * engageRegisterGroupRtpHandler().  This will cause the receiving Engine to fire the ENGAGE_GROUP_RTP_RECEIVED event,
 * passing up the payload along with a JSON object of type RtpHeader.
 *
 * Be aware that Engage-specific RTP header extensions for the group will NOT be sent for custom RTP payloads.  Applications
 * that wish to convey identifying information (such as source and target(s)) in the transmission need to include that
 * information in the payload and be responsible for handling those elements.
 *
 * Note: The application must ensure that the size of the data to be transmitted will fit inside a restricted-size UDP
 * MTU which may be encrypted.  As a best practice, applications should limit the number of bytes to be sent to 512 bytes.

 * - ENGAGE_GROUP_RTP_SENT when the data is sent
 * - ENGAGE_GROUP_RTP_SEND_FAILED if an error occurs
 *
 * @param id The ID of the group
 * @param payload Pointer to a byte array to send
 * @param size Number of bytes to send from the byte array
 * @param jsonParams RTP header parameters in a JSON object of type RtpHeader
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see jsonRtpHeader, ENGAGE_GROUP_RTP_RECEIVED, engageSendGroupBlob(), engageCreateGroup()
 */
ENGAGE_API int engageSendGroupRtp(const char * _Nonnull id, const uint8_t * _Nonnull payload, size_t size, const char * _Nonnull jsonParams);


/**
 * @brief [ASYNC] Registers the application as the handler for an RTP payload
 *
 * Instructs the Engine that RTP packets received with the indicated payload ID are to
 * be handled by the application via a callback event rather than processed by the
 * Engine.  The payload ID may be any valid RTP payload ID - including RTP payloads
 * that would normally be handled by the Engine.  In fact, having the application
 * register as the handler for RTP payloads that would normally be handled by the
 * Engine has the effect of "muting" the Engine's processing of those payloads.
 *
 * Note that registration of payload IDs are cumulative - meaning that calling
 * this API does not remove registration of a previously-registered payload ID.  Rather
 * the newly-registered payload ID is added to a list for the group that the Engine
 * will pass up to the application for processing.
 *
 * No events are generated by this API call.
 *
 * @param id The ID of the group for which to register
 * @param payloadId The RTP payload ID
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see engageUnregisterGroupRtpHandler(), engageSendGroupRtp()
 */
ENGAGE_API int engageRegisterGroupRtpHandler(const char * _Nonnull id, uint16_t payloadId);


/**
 * @brief [ASYNC] Unregisters the application as the handler for an RTP payload
 *
 * The inverse of engageRegisterGroupRtpHandler(), removing the payload ID as
 * an application-handled payload for the particular group.  If the payload ID
 * is one that the Engine would normally process, that functionality will be
 * reinstated.
 *
 * No events are generated by this API call.
 *
 * @param id The ID of the group for which to unregister
 * @param payloadId The RTP payload ID
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see engageRegisterGroupRtpHandler()
 */
ENGAGE_API int engageUnregisterGroupRtpHandler(const char * _Nonnull id, uint16_t payloadId);


/**
 * @brief [ASYNC] Send an application-structured byte array to a group as an Engine-managed blob
 *
 * Call this function to have the Engine transmit an application-defined byte array on a group that is configured
 * for RTP or Presence.
 *
 * While largely the same as engageSendGroupRtp(), this function differs in that the Engine will include a BLOB
 * header with the transmission identifying the node ID of the transmitter - the source - as well as a node ID of
 * the target (or a null node ID if the message is intended for everyone).
 *
 * Be aware that these node IDs are each 16 bytes in size and therefore result in overhead of at least 32 bytes
 * on a per-messages basis.  This use of 32 bytes therefore results in 32 bytes less data that should be included in
 * the data section of the blob in order to fit within the recommended 512-byte limit of restricted UDP MTUs.
 *
 * Blobs may be sent on *Presence* or *Audio* groups but not *Raw* groups.  When sending a blob on an audio group,
 * the Engine requires that the *jsonParams* parameter contains an *RtpHeader* JSON object that includes, at minimum,
 * the RTP payload ID to use.  This much like the requirement as specified for engageSendGroupRtp(). The Engine follows
 * much the same procedure as when using engageSendGroupRtp().  However, whereas engageSendGroupRtp() requires receiving
 * Engine instances to register for events for the payload ID in question, blob transmission does not.  Only the transmitting
 * Engine need set the payload and, as per the discussion for engageSendGroupRtp(), that payload ID must be valid.
 *
 * Blobs sent on *Presence* groups do not have a requirement for the *RtpHeader* JSON object.  If it is provided, it will
 * be ignored by the Engine.
 *
 * Engine instances receiving the transmitted blob will fire the ENGAGE_GROUP_BLOB_RECEIVED event, passing up the blob
 * payload data along with a JSON object describing it.  Note that if the blob is being transmitted to a particular node,
 * only that node's application will receive the ENGAGE_GROUP_BLOB_RECEIVED event.
 *
 * Note: The application must ensure that the size of the data to be transmitted will fit inside a restricted-size UDP
 * MTU which may be encrypted.  As a best practice, applications should limit the number of bytes to be sent to 512 bytes.

 * - ENGAGE_GROUP_BLOB_SENT when the data is sent
 * - ENGAGE_GROUP_BLOB_SEND_FAILED if an error occurs
 *
 * @param id The ID of the group
 * @param blob Pointer to a byte array to send
 * @param size Number of bytes to send from the byte array
 * @param jsonParams Blob transmission JSON object of type @ref ConfigurationObjects::BlobInfo
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see ConfigurationObjects::BlobInfo, ENGAGE_GROUP_BLOB_RECEIVED, engageSendGroupBlob(), engageCreateGroup()
 */
ENGAGE_API int engageSendGroupBlob(const char * _Nonnull id, const uint8_t * _Nonnull blob, size_t size, const char * _Nonnull jsonParams);


/**
 * @brief [ASYNC] Send an application-structured byte array to a group with minimal Engine involvement
 *
 * Call this function to have the Engine transmit raw data to the group.  In contrast to engageSendGroupBlob()
 * and engageSendGroupRtp(), the Engine does not perform any processing on the data other than to encrypt
 * it for transmission - assuming the group is encrypted.  The application is wholly responsible for meta data
 * associated with the data such as headers.  Other Engine instances receiving the data will be notified via the
 * ENGAGE_GROUP_RAW_RECEIVED event.
 *
 * This function is only valid for groups of type "Raw" - as specified in the call to engageCreateGroup().
 *
 * The Raw group type along with this API call and the ENGAGE_GROUP_RAW_RECEIVED are particularly useful in environments
 * where an application needs to interact with a legacy or third-party system using data structured in a very particular
 * format.  A Raw group may, for example, be used to interact with IoT devices, video streamers, or other entities which
 * the Engage Engine does not natively support.
 *
 * Alternatively, Raw groups may be used as a means for Engage-based applications to implement secured, proprietary
 * data exchange with each other.
 *
 * Note: The application must ensure that the size of the data to be transmitted will fit inside a restricted-size UDP
 * MTU which may be encrypted.  As a best practice, applications should limit the number of bytes to be sent to 512 bytes.
 *
 * - ENGAGE_GROUP_RAW_SENT when the data is sent
 * - ENGAGE_GROUP_RAW_SEND_FAILED if an error occurs
 *
 * @param id The ID of the group
 * @param raw Pointer to a byte array to send
 * @param size Number of bytes to send from the byte array
 * @param jsonParams Optional parameters in JSON format TODO: Shaun, where is this defined?
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see ENGAGE_GROUP_RAW_RECEIVED, engageSendGroupBlob(), engageSendGroupRtp(), engageCreateGroup()
 */
ENGAGE_API int engageSendGroupRaw(const char * _Nonnull id, const uint8_t * _Nonnull raw, size_t size, const char * _Nonnull jsonParams);


/**
 * @brief [ASYNC] Informs the Engine that the application has discovered a service
 *
 * Call this API to indicate to the Engine that the application has discovered a service on the
 * network that may be usable by the Engine.  There is no guarantee that the Engine will use the
 * new service.  However, if it does, the Engine will fire ENGAGE_GROUP_ASSET_DISCOVERED
 *
 * No events are fired as a direct result of this API call.  However, indirect events such as
 * ENGAGE_GROUP_ASSET_DISCOVERED and ENGAGE_GROUP_ASSET_REDISCOVERED may be fired if the Engine
 * decides to utilize the service and converts the capabilities offered by the service into usable
 * assets.
 *
 * @param id The ID assigned to the service by the application
 * @param jsonParams Details of the discovered service in a JSON object of type PlatformDiscoveredService
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see engagePlatformServiceRediscovered(), engagePlatformServiceUndiscovered(), ENGAGE_GROUP_ASSET_DISCOVERED
 */
ENGAGE_API int engagePlatformServiceDiscovered(const char * _Nonnull id, const char * _Nonnull jsonParams);


/**
 * @brief [ASYNC] Informs the Engine that the application has rediscovered a service
 *
 * This API call is much like engagePlatformServiceDiscovered() but typically called when
 * the application has rediscovered the service - under conditions such as certain service parameters
 * having changed like address or port number.
 *
 * No events are fired as a direct result of this API call.  However, indirect events such as
 * ENGAGE_GROUP_ASSET_DISCOVERED and ENGAGE_GROUP_ASSET_REDISCOVERED may be fired if the Engine
 * decides to utilize the service and converts the capabilities offered by the service into usable
 * assets.

 * @param id The ID assigned to the service by the application
 * @param jsonParams Details of the rediscovered service in a JSON object of type PlatformDiscoveredService
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see engagePlatformServiceDiscovered(), engagePlatformServiceUndiscovered(), ENGAGE_GROUP_ASSET_DISCOVERED
 */
ENGAGE_API int engagePlatformServiceRediscovered(const char * _Nonnull id, const char * _Nonnull jsonParams);


/**
 * @brief [ASYNC] Informs that Engine that a previously-discovered service has disappeared
 *
 * Call this API to inform the Engine that a service which was previously reported by the calls to
 * engagePlatformServiceDiscovered() and/or engagePlatformServiceRediscovered() has no longer available.
 *
 * No events are fired as a direct result of this API call.  However, ENGAGE_GROUP_ASSET_UNDISCOVERED may
 * be fired if the Engine had decided to previously utilize the service and report available assets.
 *
 * @param id The ID assigned to the service by the application
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see engagePlatformServiceDiscovered(), engagePlatformServiceRediscovered(), ENGAGE_GROUP_ASSET_UNDISCOVERED
 */
ENGAGE_API int engagePlatformServiceUndiscovered(const char * _Nonnull id);


/**
 * @brief [ASYNC] Requests that the Engine deliver a timeline report for the group
 *
 * @param id The ID of the group
 * @param jsonParams Parameters for the query operation. TODO: Shaun, where do I find this definition?
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see PFN_ENGAGE_GROUP_TIMELINE_REPORT
 */
ENGAGE_API int engageQueryGroupTimeline(const char * _Nonnull id, const char * _Nonnull jsonParams);


/**
 * @brief [ASYNC] Requests that the Engine deliver a health report for the group
 *
 * @param id The ID of the group
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see PFN_ENGAGE_GROUP_HEALTH_REPORT
 */
ENGAGE_API int engageQueryGroupHealth(const char * _Nonnull id);


/**
 * @brief [ASYNC] Requests that the Engine deliver a statistics report for the group
 *
 * @param id The ID of the group
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see PFN_ENGAGE_GROUP_STATS_REPORT
 */
ENGAGE_API int engageQueryGroupStats(const char * _Nonnull id);


/**
 * @brief [SYNC] Log a message via the Engine's logger
 *
 * @param level Logging level
 * @param tag Tag
 * @param msg Message to be logged
 * @return ENGAGE_RESULT_OK if the request was successful
 * @see loggingLevels
 */
ENGAGE_API int engageLogMsg(int level, const char * _Nonnull tag, const char * _Nonnull msg);


/**
 * @brief [SYNC] Returns an array of network interface devices in JSON format
 *
 * No events are generated by this API call.
 *
 * @return Zero-terminated JSON string of array of @ref Configuration::NetworkInterfaceDevice objects
 */
ENGAGE_API const char * _Nonnull engageGetNetworkInterfaceDevices();


/**
 * @brief [SYNC] Returns an array of audio device descriptor interface devices in JSON format
 *
 * No events are generated by this API call.
 *
 * @return Zero-terminated JSON string of array of @ref Configuration::AudioDeviceDescriptor objects
 */
ENGAGE_API const char * _Nonnull engageGetAudioDevices();


/**
 * @brief [SYNC] Generates a mission based on a key phrase
 *
 * Call this function to obtain a JSON descriptor for a mission generated using the supplied key phrase.  The mission
 * will contain 1 presence/mission control group and *audioGroupCount* audio groups.
 *
 * No events are generated by this API call.
 *
 * @param keyPhrase A string used as a baseline hash for generating algorithm
 * @param audioGroupCount The number of audio groups to include the mission
 * @param rallypointHost The host address or FQDN of the Rallypoint to use (if any)
 * @param missionName The mission name to use (if any)
 * @return Zero-terminated string representing a JSON object describing a mission
 */
ENGAGE_API const char * _Nonnull engageGenerateMission(const char * _Nonnull keyPhrase, int audioGroupCount, const char * _Nullable rallypointHost, const char * _Nullable missionName);


/**
 * @brief [SYNC] Generates a mission based on a key phrase and using a specified certificate store
 *
 * Call this function to obtain a JSON descriptor for a mission generated using the supplied key phrase.  The mission
 * will contain 1 presence/mission control group and *audioGroupCount* audio groups.
 *
 * No events are generated by this API call.
 *
 * @param keyPhrase A string used as a baseline hash for generating algorithm
 * @param audioGroupCount The number of audio groups to include the mission
 * @param rallypointHost The host address or FQDN of the Rallypoint to use (if any)
 * @param missionName The mission name to use (if any)
 * @param certStoreFn The full path to the certificate store file
 * @param certStorePasswordHexByteString Certificate store password as a hex byte string (if the file is password protected with an additional application password)
 * @param certStoreElement The name of the certificate store element to use.  If not specified, the element tagged as "-enginedefault" will be used if available.
 * @return Zero-terminated string representing a JSON object describing a mission
 */
ENGAGE_API const char * _Nonnull engageGenerateMissionUsingCertStore(const char * _Nonnull keyPhrase, 
                                                                     int audioGroupCount, 
                                                                     const char * _Nullable rallypointHost, 
                                                                     const char * _Nullable missionName,
                                                                     const char * _Nonnull certStoreFn,
                                                                     const char * _Nullable certStorePasswordHexByteString,
                                                                     const char * _Nullable certStoreElement);


/**
 * @brief [SYNC] Sets the (optional) mission ID for the Engine
 *
 * Call this function to set an optional mission ID to be used for various operations in the Engine - particularly
 * for Rallypoint registrations.
 *
 * No events are generated by this API call.
 *
 * @param missionId The mission ID (null or empty string to revert to default)
 * @return ENGAGE_RESULT_OK if the call was successful
 */
ENGAGE_API int engageSetMissionId(const char * _Nullable missionId);


/**
 * @brief [SYNC] Opens/creates a certificate store
 *
 * This function opens (or creates) a certificate store and makes it the active certificate store used for the Engine.  The function fails
 * if the file already exists and it cannot be opened due to file corruption, password mismatch, or invalid content.
 *
 * No events are generated by this API call.
 *
 * @param fileName The full path to the file
 * @param passwordHexByteString Password as a hex byte string (if the file is password protected with an additional application password)
 * @return ENGAGE_RESULT_OK if the store was opened
 */
ENGAGE_API int engageOpenCertStore(const char * _Nonnull fileName, const char * _Nullable passwordHexByteString);


/**
 * @brief [SYNC] Returns a JSON descriptor of the currently opened certificate store.
 *
 * No events are generated by this API call.
 *
 * @return JSON @see ConfigurationObjects::CertStoreDescriptor
 */
ENGAGE_API const char * _Nonnull engageGetCertStoreDescriptor(void);


/**
 * @brief [SYNC] Closes the current certificate store (if any)
 *
 * No events are generated by this API call.
 *
 * @return ENGAGE_RESULT_OK
 */
ENGAGE_API int engageCloseCertStore(void);


/**
 * @brief [SYNC] Adds/updates a certificate and (optionally) private key using PEM strings
 *
 * This function adds or updates a certificate using PEM as the source content.  PEM for the certificate must be provided but
 * the PEM for the private key may be an emoty string (or null) if no prvate key is to be associated.
 *
 * No events are generated by this API call.
 *
 * @param id The identifier by which the certificate and private key will be referenced
 * @param certificatePem Certificate content in PEM format
 * @param privateKeyPem Private key content in PEM format (optional)
 * @param tags A comma-seperated list of tags
 * @return ENGAGE_RESULT_OK if operation was successful.
 */
ENGAGE_API int engageSetCertStoreCertificatePem(const char * _Nonnull id, const char * _Nonnull certificatePem, const char * _Nullable privateKeyPem, const char * _Nullable tags);


/**
 * @brief [SYNC] Adds/updates a certificate and (optionally) private key using P12 content
 *
 * This function adds or updates a certificate using P12 as the source container.  The container must contain the certificate but
 * does not need to not contain a private key if no private key is to be associated.
 *
 * No events are generated by this API call.
 *
 * @param id The identifier by which the certificate and (optional) private key will be referenced
 * @param data A binary buffer containing the P12 data
 * @param size Size of the buffer
 * @param password Password to the P12 if it is password-protected
 * @param tags A comma-seperated list of tags
 * @return ENGAGE_RESULT_OK if operation was successful.
 */
ENGAGE_API int engageSetCertStoreCertificateP12(const char * _Nonnull id, uint8_t * _Nonnull data, size_t size, const char * _Nullable password, const char * _Nullable tags);


/**
 * @brief [SYNC] Deletes a certificate and its private key (if any)
 *
 * This function deletes a certificate from the store.
 *
 * No events are generated by this API call.
 *
 * @param id The identifier to delete
 * @return ENGAGE_RESULT_OK if operation was successful.
 */
ENGAGE_API int engageDeleteCertStoreCertificate(const char * _Nonnull id);


/**
 * @brief [SYNC] Returns a certificate in PEM format.  The private key (if any) is NOT returned.
 *
 * No events are generated by this API call.
 *
 * @return Certificate PEM
 */
ENGAGE_API const char * _Nonnull engageGetCertStoreCertificatePem(const char * _Nonnull id);


/**
 * @brief [SYNC] Returns a JSON object describing the certificate PEM.
 *
 * No events are generated by this API call.
 *
 * @param pem The PEM for which the descriptor is desired.
 * @return JSON
 */
ENGAGE_API const char * _Nonnull engageGetCertificateDescriptorFromPem(const char * _Nonnull pem);


/**
 * @brief [SYNC] Adds/updates an element (certificate and optionally private key) from another certificate store.
 *
 * This function adds or updates an element using elements(s) from another certificate store.
 *
 * No events are generated by this API call.
 *
 * @param id The identifier by which the certificate and private key will be referenced
 * @param srcId The identifier in the source certificate store
 * @param srcFileName The full path to the source certificate store file
 * @param srcPasswordHexByteString Password as a hex byte string (if the source file is password protected with an additional application password)
 * @param tags A comma-seperated list of tags
 * @return ENGAGE_RESULT_OK if operation was successful.
 */
ENGAGE_API int engageImportCertStoreElementFromCertStore(const char * _Nonnull id, const char * _Nonnull srcId, const char * _Nonnull srcFileName, const char * _Nonnull srcPasswordHexByteString, const char * _Nullable tags);


/**
 * @brief [SYNC] Returns an extended descriptor of named certificate store
 *
 * This function opens a certificate store and, if successful, returns a JSON descriptor describing all publicly-available information and contents.
 *
 * No events are generated by this API call.
 *
 * @param fileName The full path to the file
 * @param passwordHexByteString Password as a hex byte string (if the file is password protected with an additional application password)
* @return JSON
 */
ENGAGE_API const char * _Nonnull engageQueryCertStoreContents(const char * _Nonnull fileName, const char * _Nullable passwordHexByteString);


/**
 * @brief [SYNC] Updates a certificate's tags
 *
 * This function a certificate's tags.  The certificate must already be present in the store.
 *
 * No events are generated by this API call.
 *
 * @param id The identifier by which the certificate and (optional) private key will be referenced
 * @param tags A comma-seperated list of tags
 * @return ENGAGE_RESULT_OK if operation was successful.
 */
ENGAGE_API int engageSetCertStoreCertificateTags(const char * _Nonnull id, const char * _Nullable tags);


/**
 * @brief [ASYNC] Creates a bridge object
 *
 * Requests creation of a bridge object
 *
 * - ENGAGE_BRIDGE_CREATED is fired when the bridge is created.
 * - ENGAGE_BRIDGE_CREATE_FAILED is fired if the request fails due to invalid configuration or
 *   due to the fact that the bridge object already exists.
 *
 * @param jsonConfiguration A JSON object of type @ref ConfigurationObjects::Bridge
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see engageDeleteBridge(), ConfigurationObjects::Bridge
 */
ENGAGE_API int engageCreateBridge(const char * _Nonnull jsonConfiguration);


/**
 * @brief [ASYNC] Deletes a bridge object
 *
 * Requests deletiom of a bridge object
 *
 * - ENGAGE_BRIDGE_DELETED is fired when the bridge is deleted.
 *
 * @param id The ID of the bridge
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see engageCreateBridge(), ConfigurationObjects::Bridge
 */
ENGAGE_API int engageDeleteBridge(const char * _Nonnull id);


/**
 * @brief [SYNC] Returns a hardware report
 *
 * No events are generated by this API call.
 *
 * @return Zero-terminated JSON string
 */
ENGAGE_API const char * _Nonnull engageGetHardwareReport();


/**
 * @brief [ASYNC] Requests that the Engine add a timeline event
 *
 * @param id The ID of the group
 * @param jsonParams Parameters for the add operation.
 * @param blobData Optional pointer to an array of bytes for the event.
 * @param blobSize Size of the array of bytes.
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see PFN_ENGAGE_GROUP_TIMELINE_REPORT
 */
ENGAGE_API int engageAddGroupTimelineEvent(const char * _Nonnull id, const char * _Nonnull jsonParams, const uint8_t * _Nullable blobData, size_t blobSize);


/**
 * @brief [ASYNC] Requests that the Engine update a timeline event
 *
 * @param id The ID of the group
 * @param jsonParams Parameters for the add operation.
 * @param blobData Optional pointer to an array of bytes for the event.
 * @param blobSize Size of the array of bytes.
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see PFN_ENGAGE_GROUP_TIMELINE_REPORT
 */
ENGAGE_API int engageUpdateGroupTimelineEvent(const char * _Nonnull id, const char * _Nonnull jsonParams, const uint8_t * _Nullable blobData, size_t blobSize);


/**
 * @brief [ASYNC] Requests that the Engine delete timeline event
 *
 * @param id The ID of the group
 * @param jsonParams Parameters for the delete operation.
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see PFN_ENGAGE_GROUP_TIMELINE_REPORT
 */
ENGAGE_API int engageDeleteGroupTimelineEvent(const char * _Nonnull id, const char * _Nonnull jsonParams);


/**
 * @brief [SYNC] Returns the device identifier
 *
 * No events are generated by this API call.
 *
 * @return Zero-terminated string
 */
ENGAGE_API const char * _Nonnull engageGetDeviceId();


/**
 * @brief [SYNC] Enables or disables the Engine watchdog.
 *
 * This output to syslog has no effect on platforms which do not support syslog
 * such as Windows.
 *
 * No events are generated by this API call.
 *
 * @see sysLogEnableDisable, engageSetLogLevel()
 *
 * @param enable ENGAGE_WATCHDOG_ENABLE or ENGAGE_WATCHDOG_DISABLE
 * @return ENGAGE_RESULT_OK if successful
 */
ENGAGE_API int engageEnableWatchdog(int enable);


/**
 * @brief [SYNC] Notifies Engage of changes reported by the application or platform
 *
 * No events are generated by this API call.
 * *
 * @param jsonChangesArray An array of pre-defined strings detailing changes
 * 
 * @return ENGAGE_RESULT_OK if successful
 */
ENGAGE_API int engagePlatformNotifyChanges(const char * _Nonnull jsonChangesArray);


/**
 * @brief [SYNC] Notifies Engage of app's willingness to receive notifications
 *
 * No events are generated by this API call.
 * *
 * @param enable ENGAGE_NOTIFICATIONS_ENABLE or ENGAGE_NOTIFICATIONS_DISABLE
 * 
 * @return ENGAGE_RESULT_OK if successful
 */
ENGAGE_API int engageEnableNotifications(int enable);


/**
 * @brief [ASYNC] Requests that the Engine update its audio device registry
 *
 * @return ENGAGE_RESULT_OK if the submission request was successful
 * @see PFN_ENGAGE_ENGINE_AUDIO_DEVICES_REFRESHED
 */
ENGAGE_API int engageRefreshAudioDevices(void);


/**
 * @brief [ASYNC] Requests that the Engine reconfigure a group
 *
 * @param id The ID of the group
 * @param jsonConfiguration A JSON object of type @ref ConfigurationObjects::Group
 * @return ENGAGE_RESULT_OK if the submission request was successful
 */
ENGAGE_API int engageReconfigureGroup(const char * _Nonnull id, const char * _Nonnull jsonConfiguration);


/**
 * @brief [ASYNC] Requests that the Engine begin recording audio to a file
 *
 * @param jsonParams A JSON object of type @ref ConfigurationObjects::FileRecordingRequest
 * @return ENGAGE_RESULT_OK if the submission request was successful
 */
ENGAGE_API int engageBeginFileRecording(const char * _Nonnull jsonParams);


/**
 * @brief [ASYNC] Requests that the Engine end recording audio to a file
 *
 * @param id The identifier of the recording provided in the call to engageBeginFileRecording
 * @return ENGAGE_RESULT_OK if the submission request was successful
 */
ENGAGE_API int engageEndFileRecording(const char * _Nonnull id);

/**
 * @brief [SYNC] Returns JSON object containing the currently active featureset
 *
 * No events are generated by this API call.
 *
 * @return Zero-terminated JSON string of @ref Configuration::Featureset object
 */
ENGAGE_API const char * _Nonnull engageGetActiveFeatureset();

/**
 * @brief [SYNC] Enable FIPS crypto
 *
 * @param jsonParams A JSON object of type @ref ConfigurationObjects::FipsCryptoSettings
 * @return ENGAGE_RESULT_OK on success, other values on failure
 */
ENGAGE_API int engageSetFipsCrypto(const char * _Nonnull jsonParams);

/**
 * @brief [SYNC] Returns 1 if crypto is FIPS140-2 validated, otherwise 0
 *
 * No events are generated by this API call.
 * 
 * @return 1 if crypo is FIPS validated, otherwise 0
 */
ENGAGE_API int engageIsCryptoFipsValidated(void);

/**
 * @brief [SYNC] Set the certstore content via buffer
 *
 * An alternative to engageOpenCertStore whereby a buffer is passed to the Engine containing
 * a certificate store's binary data rather having it loaded from file.
 *
 * No events are generated by this API call.
 *
 * @param buffer Pointer to the certificate store byte array
 * @param size Number of bytes in the buffer
 * @param passwordHexByteString Hexadecimal representation of the password to be used to decrypt the buffer
 * @return ENGAGE_RESULT_OK on success, other values on failure
 * @see engageOpenCertStore()
 */
ENGAGE_API int engageSetCertStore(const uint8_t * _Nonnull buffer, size_t size, const char * _Nullable passwordHexByteString);

// TODO: Engage compression/decompression functions not available at this time
#if 0
/**
 * @brief [SYNC] Compress data using the Engine's compression functions
 *
 * Applications can use this function call to compress data using the Engine's compression logic.  The
 * algorithm used by compression is Brotli (https://en.wikipedia.org/wiki/Brotli) and is well suited
 * for compressing a variety of formats.  Of particular note is compression of JSON data which Brotli
 * compresses better than any other algorithm present in the industry.
 *
 * It is important that the calling application allocate sufficient space in the destination to
 * accommodate the compressed output.  If insufficient space exists, the API call returns 
 * ENGAGE_RESULT_INSUFFICIENT_DESTINATION_SPACE.
 *
 * No events are generated by this API call.
 *
 * @param src Pointer to the source byte array to be compressed
 * @param srcSize Number of bytes in the source byte array
 * @param dst Pointer to the destination where compressed data is to be output
 * @param maxDstSize Maximum number of bytes available in the buffer pointed to by dst
 * @return Number of bytes output to the destination (<= 0 if an error occurred)
 * @see engageDecompress()
 */
ENGAGE_API int engageCompress(const uint8_t * _Nonnull src, size_t srcSize, uint8_t * _Nonnull dst, size_t maxDstSize);

/**
 * @brief [SYNC] Decompress data using the Engine's compression functions
 *
 * The inverse of engageCompress().  Consult the discussion for engageCompress() for more information.
 *
 * No events are generated by this API call.
 *
 * @param src Pointer to the source byte array to be decompressed
 * @param size Number of bytes in the source byte array
 * @param dst Pointer to the destination where decompressed data is to be output
 * @param maxDstSize Maximum number of bytes available in the buffer pointed to by dst
 * @return Number of bytes output to the destination (<= 0 if an error occurred)
 * @see engageCompress()
 */
ENGAGE_API int engageDecompress(const uint8_t * _Nonnull src, size_t srcSize, uint8_t * _Nonnull dst, size_t maxDstSize);
#endif


/**
 * @brief [SYNC] Registers for a high resolution (millisecond-precision) timer
 *
 * Creates an ongoing timer at millisecond resolution.  Only a single high resolution
 * timer can exist at any one time.
 *
 * No events are generated by this API call.
 *
 * @param durationMs Milliseconds between intervals
 * @return ENGAGE_RESULT_OK on success, other values on failure
 * @see engageUnregisterFromHighResolutionTimer()
 */
ENGAGE_API int engageRegisterForHighResolutionTimer(uint32_t durationMs);


/**
 * @brief [SYNC] Unregisters the previously-registered high-resolution timer if any.
 *
 * Unregisters the previously registered timer.  If none exists no error occurs.
 *
 * No events are generated by this API call.
 *
 * @return ENGAGE_RESULT_OK on success, other values on failure
 * @see engageRegisterForHighResolutionTimer()
 */
ENGAGE_API int engageUnregisterFromHighResolutionTimer(void);

#endif

#ifdef __cplusplus
}
#endif
#endif // EngageInterface_h
