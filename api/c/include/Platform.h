//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

#ifndef Platform_h
#define Platform_h

#if defined(WIN32)
    #include <WinSock2.h>
	#include <Ws2tcpip.h>
	#include <mswsock.h>
#else
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <netinet/in.h>
    #include <signal.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/un.h>

    #if defined( __ANDROID__ )
        #include <sys/prctl.h>
    #endif
#endif

#ifdef WIN32
    #define strncasecmp                             _strnicmp
    #define strcasecmp                              _stricmp
    #define PATH_MAX                                MAX_PATH
    #define pid_t                                   int
    #define SHUT_RDWR                               SD_BOTH
	#ifdef _WIN64
		#define ssize_t		                        __int64
	#else
		#define ssize_t		                        __int32
	#endif

#else
    static const int INVALID_SOCKET = -1;

    #define strtok_s                                strtok_r
    #define strcpy_s(__dst, __sz, __src)            strcpy(__dst, __src)
    #define strncpy_s(__dst, __sz, __src, __mx)     strncpy(__dst, __src, __mx)
    #define strcat_s(__dst, __sz, __src)            strcat(__dst, __src)
    #if defined(RTS_HAVE_SNPRINTF)
        #define sprintf_s(__dst, __sz, __fmt, ...)      snprintf(__dst, __sz, __fmt, __VA_ARGS__)
    #else
        #define sprintf_s(__dst, __sz, __fmt, ...)      sprintf(__dst, __fmt, __VA_ARGS__)
    #endif
    #define SOCKET                                  int
    #define sscanf_s                                sscanf
    #define closesocket                             close
#endif

#if !defined(htonll)
    #define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#endif

#if !defined(ntohll)
    #define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(WIN32)
    #ifdef PLATFORM_EXPORTS
        // Windows needs dllexport to produce an import lib without a .DEF file
        #define PLATFORM_API  __declspec(dllexport) extern
    #else
        #define PLATFORM_API  extern
    #endif
#else
    #define PLATFORM_API
#endif

PLATFORM_API bool PlatformInit();
PLATFORM_API void PlatformDeinit();
PLATFORM_API void PlatformOnProcessStarted();
PLATFORM_API void PlatformOnProcessEnded();
PLATFORM_API void PlatformOnThreadStarted();
PLATFORM_API void PlatformOnThreadEnded();
PLATFORM_API void PlatformSetThreadName(const char *nm);
PLATFORM_API void PlatformGetThreadName(char *buff, size_t buffSize);
PLATFORM_API void PlatformOnPlatformChangeNotification(const char *notification);
PLATFORM_API bool PlatformStoreSecret(const char *key, const char *val);
PLATFORM_API bool PlatformDeleteSecret(const char *key);
PLATFORM_API bool PlatformGetSecret(const char *key, char **retval);
PLATFORM_API void PlatformEnableAec(intptr_t p1, intptr_t p2, intptr_t p3, intptr_t p4);
PLATFORM_API void PlatformDisableAec(intptr_t p1, intptr_t p2, intptr_t p3, intptr_t p4);
PLATFORM_API void PlatformAbort();

#ifdef __cplusplus
}
#endif

#endif /* Platform_h */
