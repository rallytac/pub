
/*! \file EngageLm.h
      \brief Specification for Engage Loadable Modules

*/

//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

#ifndef EngageLm_h
#define EngageLm_h

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined(ENGAGE_LM_API)
    #if defined(WIN32)
        #ifdef ENGAGE_LM_EXPORTS
            #define ENGAGE_LM_API  __declspec(dllexport) extern
        #else
            #define ENGAGE_LM_API  extern
        #endif
    #else
        #define ENGAGE_LM_API  __attribute__ ((visibility ("default")))
    #endif
#endif

typedef uintptr_t ENGAGE_HANDLE;
#define NULL_ENGAGE_HANDLE ((uintptr_t)nullptr)

#pragma pack(push, 1)
    // The V-table passed from the Engine to the loadable module
    typedef struct _EngineVT_t
    {
        // The size of the vtable is like a "version"
        unsigned int vtSize;

        #ifdef WIN32
            void (*pfnLogger)(int level, const char *tag, _Printf_format_string_ const char *fmt, ...);
        #else
            void (*pfnLogger)(int level, const char *tag, const char *fmt, ...)
                __attribute__((__format__(__printf__, 3, 4)));
        #endif
    } EngineVT_t;

    // The base of loadable module V-tables
    typedef struct _LmVTBase_t
    {
        // The size of the vtable is like a "version"
        unsigned int vtSize;

        // Module-level
        int (*initializeModule)(const char *moduleConfiguration);
        int (*deinitializeModule)();
        const char *(*getModuleDescriptor)();
        
        // Instance level
        ENGAGE_HANDLE (*createInstance)(const char *instanceConfiguration);
        void (*deleteInstance)(ENGAGE_HANDLE h);
        const char *(*getInstanceDescriptor)(ENGAGE_HANDLE h);

        void (*startInstance)(ENGAGE_HANDLE h);
        void (*stopInstance)(ENGAGE_HANDLE h);
        void (*pauseInstance)(ENGAGE_HANDLE h);
        void (*resumeInstance)(ENGAGE_HANDLE h);
        void (*resetInstance)(ENGAGE_HANDLE h);
    } LmVTBase_t;

    // A V-table for a codec instance
    typedef struct _LmVTCodec_t
    {
        LmVTBase_t      base;

        // Encoding
        void (*setEncoderFramingMs)(ENGAGE_HANDLE h, int ms);
        void (*feedEncoder)(ENGAGE_HANDLE h, const int16_t *pInput, size_t inputSize, int vadResult);
        size_t (*extractFromEncoder)(ENGAGE_HANDLE h, uint8_t *pOutput, size_t *pSamplesEncoded);

        // Decoding
        int (*decodeUsingDecoder)(ENGAGE_HANDLE h, const uint8_t *pInput, size_t inputSize, int16_t *pOutput, size_t maxOutputSize);
    } LmVTCodec_t;
#pragma pack(pop)

// The vtable-exchange function that the module needs to export
ENGAGE_LM_API int EngageLM_XVT(const EngineVT_t *evt, void *mvt, unsigned int maxMvtSize);

#ifdef __cplusplus
}
#endif
#endif // EngageLm_h
