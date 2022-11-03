//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

#ifndef EngageConstants_h
#define EngageConstants_h

#include <stdint.h>

/** @addtogroup resultCodes Engage Engine Result Codes
 *
 * Result codes are returned by calls to the API functions and most often are related to
 * the submission of a request to Engage rather than the outcome of that submission.
 *
 * For example: a call to engageStart resulting in a return value of ENGAGE_RESULT_OK simply
 * means that the request to asychronously start Engage has been successfully submitted.  That
 * asynchronous operation will report back later on the outcome by means of an ON_ENGINE_STARTED
 * event.
 *  @{
 */
/** @brief The request was succesful */
static const int ENGAGE_RESULT_OK = 0;
/** @brief One or more parameters are invalid */
static const int ENGAGE_RESULT_INVALID_PARAMETERS = -1;
/** @brief The library has not yet been initialized - engageInitialize() should be called */
static const int ENGAGE_RESULT_NOT_INITIALIZED = -2;
/** @brief The library has already been initialized */
static const int ENGAGE_RESULT_ALREADY_INITIALIZED = -3;
/** @brief An unspecified error has occurred */
static const int ENGAGE_RESULT_GENERAL_FAILURE = -4;
/** @brief The Engine has not yet been started - engageStart() should be called */
static const int ENGAGE_RESULT_NOT_STARTED = -5;
/** @brief The Engine has already been started */
static const int ENGAGE_RESULT_ALREADY_STARTED = -6;
/** @brief Insufficient space in destination */
static const int ENGAGE_RESULT_INSUFFICIENT_DESTINATION_SPACE = -7;
/** @brief Initialization of the crypto module failed */
static const int ENGAGE_RESULT_CRYPTO_MODULE_INITIALIZATION_FAILED = -8;
/** @brief An application high resolution timer is already defined */
static const int ENGAGE_RESULT_HIGH_RES_TIMER_ALREADY_EXISTS = -9;
/** @} */


/** @addtogroup loggingLevels Logging levels
 *
 * Logging levels dictate the amount of output generated by Engage during operation. In
 * general you should aim to keep your logging at ENGAGE_LOG_LEVEL_INFORMATIONAL or lower
 * as logging at ENGAGE_LOG_LEVEL_DEBUG produces a great deal of output and can overwhelm
 * a system, resulting in degraded performance.
 *
 * On systems that support syslog, Engage will output to the syslog subsystem so that
 * monitoring tools can make appropriate use of messages - particularly those of a
 * warning, error, or fatal nature.
 *
 * @see engageSetLogLevel
 *  @{
 */
/** @brief A fatal error has occurred - Engine operation cannot continue */
static const int ENGAGE_LOG_LEVEL_FATAL = 0;
/** @brief A error has occurred but operation can continue */
static const int ENGAGE_LOG_LEVEL_ERROR = 1;
/** @brief Indicates that a condition exists that should be addressed soon */
static const int ENGAGE_LOG_LEVEL_WARNING = 2;
/** @brief The log message is of an informational nature - no action is required */
static const int ENGAGE_LOG_LEVEL_INFORMATIONAL = 3;
/** @brief Debugging, primarily geared toward development and support personnel */
static const int ENGAGE_LOG_LEVEL_DEBUG = 4;
/** @} */


/** @addtogroup sysLogEnableDisable Syslog Enable/Disable
 *
 * These are simply inputs to engageEnableSyslog() representing
 * TRUE and FALSE values.
 *
 * @see engageEnableSyslog
 *  @{
 */
/** @brief Enables syslog output on supported systems */
static const int ENGAGE_SYSLOG_ENABLE = 1;
/** @brief Disables syslog output */
static const int ENGAGE_SYSLOG_DISABLE = 0;
/** @} */


/** @addtogroup watchdogEnableDisable Watchdog Enable/Disable
 *
 * These are simply inputs to engageEnableWatchdog() representing
 * TRUE and FALSE values.
 *
 * @see engageEnableWatchdog
 *  @{
 */
/** @brief Enables the watchdog - has no effect if the watchdog is disabled in the policy configuration */
static const int ENGAGE_WATCHDOG_ENABLE = 1;
/** @brief Disables the watchdog - has no effect if the watchdog is disabled in the policy configuration */
static const int ENGAGE_WATCHDOG_DISABLE = 0;
/** @} */


/** @addtogroup notificationEnableDisable Notifications Enable/Disable
 *
 * These are simply inputs to engageEnableNotifications() representing
 * TRUE and FALSE values.
 *
 * @see engageEnableNotifications
 *  @{
 */
/** @brief Enables notifications */
static const int ENGAGE_NOTIFICATIONS_ENABLE = 1;
/** @brief Disables notifications */
static const int ENGAGE_NOTIFICATIONS_DISABLE = 0;
/** @} */


/** @addtogroup txFlags Application-configurable TX flags
 *
 * These flags are added to RTP packets that contain header extensions.  If the
 * group does not have header extensions enabled, no flags will accompany the
 * transmission.
 *
 * @see engageBeginGroupTx(), engageBeginGroupTxAdvanced()
 *  @{
 */

/** @brief Sets the EMERGENCY flag in the transmitted stream */
static const uint8_t ENGAGE_TXFLAG_EMERGENCY                = 0x0001;

/** @brief Indicates that the transmission has concluded */
static const uint8_t ENGAGE_TXFLAG_TX_END                   = 0x0002;                    // READ-ONLY!!

/** @brief Indicates that the transmission is coming from an automated system */
static const uint8_t ENGAGE_TXFLAG_AUTOMATED_SYSTEM         = 0x0004;

/** @brief Indicates a "sticky" transmission ID (if non-zero) */
static const uint8_t ENGAGE_TXFLAG_STICKY_TID               = 0x0008;

/** @brief Indicates that the transmission ID is no longer valid */
static const uint8_t ENGAGE_TXFLAG_TID_INVALIDATED          = 0x0010;                    // READ-ONLY!!

/** @brief Indicates that the receiver should auto-mute using the alias specializer if receiver has that specializer present */
static const uint8_t ENGAGE_TXFLAG_MUTE_RX_FOR_SPECIALIZER  = 0x0020;                    // READ-ONLY!!
/** @} */

#endif // EngageConstants_h
