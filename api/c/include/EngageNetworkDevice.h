//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//
/** @file EngageNetworkDevice.h
 *  @brief Custom Network Devices
 *
 */

#ifndef EngageNetworkDevice_h
#define EngageNetworkDevice_h


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

/** @brief Network Device Control Operation enum
 *
 * More detailed enum description.
 */
typedef enum {
    /** @brief Start operation */
    enetStart,

    /** @brief Stop operation */
    enetStop,

    /** @brief Send a buffer */
    enetSend,

    /** @brief Read a buffer */
    enetRecv
} EngageNetworkDeviceCtlOp_t;

/**
 * @brief Everything is fine.
 *
 * The operation executed without any errors.
 */
static const int ENGAGE_NETWORK_DEVICE_RESULT_OK = 0;

/**
 * @brief A general error occurred.
 *
 * This could be due to a number of reasons. See debug output logs for more information.
 */
static const int ENGAGE_NETWORK_DEVICE_GENERAL_ERROR = -1;

/**
 * @brief Unable to register device due to invalid configuration.
 *
 * This could be due to a number of reasons. See debug output logs for more information.
 */
static const int ENGAGE_NETWORK_DEVICE_INVALID_CONFIGURATION = -2;

/**
 * @brief Invalid deviceId.
 *
 * The deviceId used is no longer valid.  See debug output logs for more information.
 */
static const int ENGAGE_NETWORK_DEVICE_INVALID_DEVICE_ID = -3;

/**
 * @brief Invalid instanceId.
 *
 * The instanceId used is no longer valid.  See debug output logs for more information.
 */
static const int ENGAGE_NETWORK_DEVICE_INVALID_INSTANCE_ID = -4;

/**
 * @brief Invalid deviceId and instanceId.
 *
 * Both deviceId and instanceId used are no longer valid.  See debug output logs for more information.
 */
static const int ENGAGE_NETWORK_DEVICE_INVALID_COMBINED_DEVICE_ID_AND_INSTANCE_ID = -5;

/**
 * @brief An invalid operation has been attempted.
 *
 * An invalid operation was attempted, please make sure you use a valid operation from @ref EngageNetworkDeviceCtlOp_t enum and that the device is in the correct state.  See debug output logs for more information.
 */
static const int ENGAGE_NETWORK_DEVICE_INVALID_OPERATION = -6;

/**
 * @brief Network Device Control instance control pointer
 *
 * @param deviceId The device identifier
 * @param op The operation to carry out
 * @param jsonMetaData JSON metadata associated with the operation
 * @param p1 Variable type based on op
 * @param p2 Variable type based on op
 * @param p3 Variable type based on op
 * @param p4 Variable type based on op
 * @param p5 Variable type based on op
 * 
 * When:
 * 
 * ----------- -------------- ---------------- ---------------- ---------------- ---------------- ----------------
 * op         | jsonMetadata |      p1        |      p2        |      p3        |      p4        |      p5        |
 * ----------- -------------- ---------------- ---------------- ---------------- ---------------- ----------------
 * enetStart  |     ?        |                |                |                |                |                |
 * enetStop   |     ?        |                |                |                |                |                |
 * enetSend   |     ?        |     buff       |     buffSz     |    timeoutMs   |                |                |
 * enetRecv   |     ?        |     buff       |   maxBuffSz    |    timeoutMs   |                |                |
 * ----------- -------------- ---------------- ---------------- ---------------- ---------------- ----------------
 * 
 */
typedef int (*PFN_ENGAGE_NETWORK_DEVICE_CTL)(int16_t deviceId, 
                                             EngageNetworkDeviceCtlOp_t op, 
                                             const char *jsonMetaData, 
                                             uintptr_t p1,
                                             uintptr_t p2,
                                             uintptr_t p3,
                                             uintptr_t p4,
                                             uintptr_t p5);

/**
 * @brief [SYNC] Registers a network device with the Engine
 *
 * @details The jsonConfiguration JSON configuration object contains the configuration for the network device
 * and the pfnCtl is a pointer to a control structure used to manage the device's instance.
 *
 * @param jsonConfiguration pointer to JSON Network Device configuration object @ref ConfigurationObjects::NetworkDeviceDescriptor
 * @param pfnCtl pointer to an Network Device Control instance @see PFN_ENGAGE_NETWORK_DEVICE_CTL
 * @return ENGAGE_NETWORK_DEVICE_RESULT_OK if successful
 */
ENGAGE_API int16_t engageNetworkDeviceRegister(const char *jsonConfiguration, PFN_ENGAGE_NETWORK_DEVICE_CTL pfnCtl);

/**
 * @brief [SYNC] Unregisters a network device
 *
 * @details Unregisters the network device created by @ref engageNetworkDeviceRegister
 *
 * @param deviceId device identifier returned engageNetworkDeviceRegister
 *
 * @return ENGAGE_NETWORK_DEVICE_RESULT_OK
 */
ENGAGE_API int16_t engageNetworkDeviceUnregister(int16_t deviceId);

#ifdef __cplusplus
}
#endif
#endif // EngageNetworkDevice_h
