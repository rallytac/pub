//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//
/** @file EngageAudioDevice.h
 *  @brief Custom Audio Devices
 *
 * TODO: Shaun, can you please review this whole file?
 */

#ifndef EngageAudioDevice_h
#define EngageAudioDevice_h


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

/** @brief Audio Device Control Operation enum
 *
 * More detailed enum description.
 */
typedef enum {
    /** @brief Create an audio device instance */
    eadCreateInstance = 1,

    /** @brief Destroy an audio device instance */
    eadDestroyInstance,

    /** @brief Start playback */
    eadStart,

    /** @brief Stop playback */
    eadStop,

    /** @brief Pause playback  */
    eadPause,

    /** @brief Resume playback */
    eadResume,

    /** @brief Reset an audio device - TODO:Shaun */
    eadReset,

    /** @brief Restart an audio device - TODO:Shaun */
    eadRestart
} EngageAudioDeviceCtlOp_t;

/**
 * @brief Everything is fine.
 *
 * The operation executed without any errors.
 */
static const int ENGAGE_AUDIO_DEVICE_RESULT_OK = 0;

/**
 * @brief A general error occurred.
 *
 * This could be due to a number of reasons. See debug output logs for more information.
 */
static const int ENGAGE_AUDIO_DEVICE_GENERAL_ERROR = -1;

/**
 * @brief Unable to register device due to invalid configuration.
 *
 * This could be due to a number of reasons. See debug output logs for more information.
 */
static const int ENGAGE_AUDIO_DEVICE_INVALID_CONFIGURATION = -2;

/**
 * @brief Invalid deviceId.
 *
 * The deviceId used is no longer valid.  See debug output logs for more information.
 */
static const int ENGAGE_AUDIO_DEVICE_INVALID_DEVICE_ID = -3;

/**
 * @brief Invalid instanceId.
 *
 * The instanceId used is no longer valid.  See debug output logs for more information.
 */
static const int ENGAGE_AUDIO_DEVICE_INVALID_INSTANCE_ID = -4;

/**
 * @brief Invalid deviceId and instanceId.
 *
 * Both deviceId and instanceId used are no longer valid.  See debug output logs for more information.
 */
static const int ENGAGE_AUDIO_DEVICE_INVALID_COMBINED_DEVICE_ID_AND_INSTANCE_ID = -5;

/**
 * @brief An invalid operation has been attempted.
 *
 * An invalid operation was attempted, please make sure you use a valid operation from @ref EngageAudioDeviceCtlOp_t enum and that the device is in the correct state.  See debug output logs for more information.
 */
static const int ENGAGE_AUDIO_DEVICE_INVALID_OPERATION = -6;

/**
 * @brief Audio Device Control instance pointer
 *
 * TODO: Shaun, want to add something?
 */
typedef int (*PFN_ENGAGE_AUDIO_DEVICE_CTL)(int16_t deviceId, int16_t instanceId, EngageAudioDeviceCtlOp_t op, uintptr_t p1);

/**
 * @brief [SYNC] Registers an Audio Device with the Engine
 *
 * @details The jsonConfiguration JSON configuration object contains the configuration for the virtual audio device
 * and the pfnCtl is a pointer to a control structure used to manage the audio device instance.
 *
 * @param jsonConfiguration pointer to JSON Audio Device configuration object @ref ConfigurationObjects::AudioDeviceDescriptor
 * @param pfnCtl pointer to an Audio Device Control instance @see PFN_ENGAGE_AUDIO_DEVICE_CTL
 * @return ENGAGE_AUDIO_DEVICE_RESULT_OK if successful TODO:Shaun is this correct>
 */
ENGAGE_API int16_t engageAudioDeviceRegister(const char *jsonConfiguration, PFN_ENGAGE_AUDIO_DEVICE_CTL pfnCtl);

/**
 * @brief [SYNC] Unregisters a virtual audio device
 *
 * @details Unregisters the virtual audio device created by @ref engageAudioDeviceRegister
 *
 * @param deviceId device identifier returned engageAudioDeviceRegister
 *
 * @return ENGAGE_AUDIO_DEVICE_RESULT_OK TODO:Shaun
 */
ENGAGE_API int16_t engageAudioDeviceUnregister(int16_t deviceId);

/**
 * @brief [SYNC] Writes a buffer of audio to the audio device
 *
 * @details TODO:Shaun, do we need any more details on the size of the buffer and samples calculation?
 *
 * @param deviceId device identifier returned by @ref engageAudioDeviceRegister
 * @param instanceId instance identifier
 * @param buffer Pointer to a buffer containing the audio
 * @param samples The number of samples contained in the buffer
 * @return ENGAGE_AUDIO_DEVICE_RESULT_OK if successful
 */
ENGAGE_API int16_t engageAudioDeviceWriteBuffer(int16_t deviceId, int16_t instanceId, const int16_t *buffer, size_t samples);

/**
 * @brief [SYNC] Reads a buffer of audio from the audio device
 *
 * @details TODO:Shaun, do we need any more details on the size of the buffer and samples calculation?
 *
 * @param deviceId device identifier returned by @ref engageAudioDeviceRegister
 * @param instanceId instance identifier
 * @param buffer Pointer to a buffer containing the audio
 * @param samples The number of samples contained in the buffer
 * @return ENGAGE_AUDIO_DEVICE_RESULT_OK if successful
 */
ENGAGE_API int16_t engageAudioDeviceReadBuffer(int16_t deviceId, int16_t instanceId, int16_t *buffer, size_t samples);

#ifdef __cplusplus
}
#endif
#endif // EngageAudioDevice_h
