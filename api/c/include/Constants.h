//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

#ifndef Constants_h
#define Constants_h

#include <cstddef>
#include <cstdint>

#ifdef WIN32
    #include <WinSock2.h>
	#include <Windows.h>
#else
    #include <netinet/in.h>
#endif

static const int INVALID_IP_ADDRESS_FAMILY = -1;

#ifndef WIN32
    static const int INVALID_SOCKET = -1;
    #define closesocket     close
#endif

#ifdef WIN32
    #define SHUT_RDWR   SD_BOTH
	#ifdef _WIN64
		#define ssize_t		__int64
	#else
		#define ssize_t		__int32
	#endif
#endif

static const size_t MAX_RTP_OUTPUT_QUEUE_PACKETS = 100;

static const size_t MAX_IP_ADDR_SIZE = sizeof(struct sockaddr_in6);
static const size_t MAX_ALIAS_SIZE = 16;
static const size_t MAX_NODE_ID_SIZE = 16;

static const size_t RTP_PACKET_BUFFER_ALLOCATION_EXTRA_BYTES = 512;
static const size_t BLOB_PACKET_BUFFER_ALLOCATION_EXTRA_BYTES = 512;

static const int16_t PCM_MIN_VALUE = -32768;
static const int16_t PCM_MAX_VALUE = 32767;

static const uint16_t SELECT_FUNCTION_TIMEOUT_SECS = 1;
static const uint16_t MULTICAST_REJOIN_SECS = 8;

static const size_t MAX_DATAGRAM_SIZE = 4096;
static const long MAX_RECONNECT_PAUSE_MS = 30000;
static const long RECONNECT_FAILURE_PAUSE_INCREMENT_MS = 1500;

static const size_t BASE_RTP_HEADER_SIZE = 12;
static const uint16_t ACCEPTED_RTP_VERSION = 2;
static const size_t MAX_RTP_SAMPLES_THAT_CAN_BE_DECODED = (8000 * 5);

static const size_t PCM_CHANNELS = 1;
static const size_t PCM_SAMPLE_COUNT_PER_MS = 8;
static const size_t PCM_10_MS_SAMPLE_COUNT = (PCM_SAMPLE_COUNT_PER_MS * 10);

#if defined(__APPLE__)
    static const size_t PCM_MIN_PLATFORM_SAMPLE_COUNT = (PCM_SAMPLE_COUNT_PER_MS * 40);
    static const size_t NUM_SPEAKER_BUFFERS = 3;
    static const size_t NUM_MIC_BUFFERS = 3;
#elif defined(__ANDROID__)
    static const size_t PCM_MIN_PLATFORM_SAMPLE_COUNT = (PCM_SAMPLE_COUNT_PER_MS * 250);
    static const size_t NUM_SPEAKER_BUFFERS = 5;
    static const size_t NUM_MIC_BUFFERS = 3;
#elif defined(__linux__)
    static const size_t PCM_MIN_PLATFORM_SAMPLE_COUNT = (PCM_SAMPLE_COUNT_PER_MS * 250);
    static const size_t NUM_SPEAKER_BUFFERS = 5;
    static const size_t NUM_MIC_BUFFERS = 3;
#elif defined(WIN32)
	static const size_t PCM_MIN_PLATFORM_SAMPLE_COUNT = (PCM_SAMPLE_COUNT_PER_MS * 250);
	static const size_t NUM_SPEAKER_BUFFERS = 5;
    static const size_t NUM_MIC_BUFFERS = 3;
#endif

static const long TLS_CONNECTION_KEY_MATERIAL_SIZE = 32;
static const uint64_t RTP_RESET_AFTER_IDLE_MS = (1000 * 30);

#if defined(RTS_DEBUG_BUILD)
static const uint64_t GROUP_HEALTH_ERROR_ERROR_NOTIFICATION_INTERVAL_MS = (1000 * 10);
#else
static const uint64_t GROUP_HEALTH_ERROR_ERROR_NOTIFICATION_INTERVAL_MS = (1000 * 30);
#endif

static const size_t MAX_GROUPS_PER_BRIDGE = 128;

#endif // Constants_h
