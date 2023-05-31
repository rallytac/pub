//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

/** \file ConfigurationObjects.h
    \brief Place to look for configuration helper
*/

/**
 * Detailed description of the ConfigurationObjects.h file
 *
 * This file contains all the configuration objects
 *
 *  Copyright (c) 2018 Rally Tactical Systems, Inc.
 *  All rights reserved.
 *
 */

#ifndef ConfigurationObjects_h
#define ConfigurationObjects_h

#include "Platform.h"
#include "EngageConstants.h"

#include <iostream>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <vector>
#include <string>

#include <nlohmann/json.hpp>

#ifndef WIN32
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-function"
#endif

#if !defined(ENGAGE_IGNORE_COMPILER_UNUSED_WARNING)
    #if defined(__GNUC__)
        #define ENGAGE_IGNORE_COMPILER_UNUSED_WARNING __attribute__((unused))
    #else
        #define ENGAGE_IGNORE_COMPILER_UNUSED_WARNING
    #endif
#endif  // ENGAGE_IGNORE_COMPILER_UNUSED_WARNING

// We'll use a different namespace depending on whether we're building the RTS core code
// or if this is being included in an app-land project.
#if defined(RTS_CORE_BUILD)
namespace ConfigurationObjects
#else
namespace AppConfigurationObjects
#endif
{
    static const char *ENGAGE_CONFIGURATION_OBJECT_ATTACHED_OBJECT = "_attached";

    //-----------------------------------------------------------
    #pragma pack(push, 1)
        typedef struct _DataSeriesHeader_t
        {
            /** @brief DataSeries Type.
             * Currently supported types
             *
             * Type | Value | Range | JSON Object Name | Detail
             * --- | ---  | ---  | --- | ---
             * Heart Rate | 1 | 0-255 | heartRate | Beats per minute
             * Skin Temperature | 2 | 0-255 | skinTemp | Celsius
             * Core Temperature | 3 | 0-255 | coreTemp | Celsius
             * Hydration Percentage | 4 | 0-100 | hydration | Percentage
             * Blood Oxygenation Percentage | 5 | 0-100 | bloodOxygenation | Percentage
             * Fatigue Level | 6 | 0-10 | fatigueLevel | 0 = low fatigue, 10 = maximum fatigue
             * Task Effectiveness Level | 7 | 0-10 | taskEffectiveness | 0 = minimal effectiveness, 10 = maximum effectiveness
             *
             */
            uint8_t     t;

            /**
             * @brief Timestamp representing the number of seconds elapsed since January 1, 1970 -  based on traditional Unix time
             */
            uint32_t    ts;

            /** @brief Increment type.
             *  Valid Types:
             *
             * Type  | Value
             * ------------- | -------------
             * 0 | Seconds
             * 1 | Milliseconds
             * 2 | Minutes
             * 3 | Hours
             * 4 | Days
             *
             */
            uint8_t     it;

            /**
             * @brief Increment multiplier.
             * The increment multiplier is an additional field that allows you apply a multiplier of 1-255 for time offset increments.
             * For example: let's assume that our data is being gathered at 20-millisecond intervals. We could certainly represent the
             * data as [0,37,60,38,20,42,140,36] where the time offset "3" becomes "60", "1" becomes "20", and "7" becomes "140".
             *
             * Valid range is 1 to 255
             */
            uint8_t     im;

            /**
             * @brief Value type
             */
            uint8_t     vt;

            /**
             * @brief Series size (element count)
             */
            uint8_t     ss;
        } DataSeriesHeader_t;

        typedef struct _DataElementUint8_t
        {
            uint8_t     ofs;
            uint8_t     val;
        } DataElementUint8_t;

        typedef struct _DataElementUnint16_t
        {
            uint8_t     ofs;
            uint16_t    val;
        } DataElementUnint16_t;

        typedef struct _DataElementUnint32_t
        {
            uint8_t     ofs;
            uint32_t    val;
        } DataElementUnint32_t;

        typedef struct _DataElementUnint64_t
        {
            uint8_t     ofs;
            uint64_t    val;
        } DataElementUnint64_t;
    #pragma pack(pop)

    typedef enum
    {
        invalid = 0,
        uint8 = 1,
        uint16 = 2,
        uint32 = 3,
        uint64 = 4
    } DataSeriesValueType_t;

    /**
     * @brief Human Biometric Types.
     *
     * TODO: More detailed HumanBiometricsTypes_t description.
     */
    typedef enum
    {
        unknown = 0,
        heartRate = 1,
        skinTemp = 2,
        coreTemp = 3,
        hydration = 4,
        bloodOxygenation = 5,
        fatigueLevel = 6,
        taskEffectiveness = 7
    } HumanBiometricsTypes_t;

    //-----------------------------------------------------------

    static FILE *_internalFileOpener(const char *fn, const char *mode)
    {
        FILE *fp = nullptr;

        #ifndef WIN32
            fp = fopen(fn, mode);
        #else
            if(fopen_s(&fp, fn, mode) != 0)
            {
                fp = nullptr;
            }
        #endif

        return fp;
    }

    #define JSON_SERIALIZED_CLASS(_cn) \
        class _cn; \
        static void to_json(nlohmann::json& j, const _cn& p); \
        static void from_json(const nlohmann::json& j, _cn& p);

    #define IMPLEMENT_JSON_DOCUMENTATION(_cn) \
        public: \
        static void document(const char *path = nullptr) \
        { \
            _cn   example; \
            example.initForDocumenting(); \
            std::string theJson = example.serialize(3); \
            std::cout << "------------------------------------------------" << std::endl \
                      << #_cn << std::endl \
                      << theJson << std::endl \
                      << "------------------------------------------------" << std::endl; \
            \
            if(path != nullptr && path[0] != 0) \
            { \
                std::string fn = path; \
                fn.append("/"); \
                fn.append(#_cn); \
                fn.append(".json");  \
                \
                FILE *fp = _internalFileOpener(fn.c_str(), "wt");\
                \
                if(fp != nullptr) \
                { \
                    fputs(theJson.c_str(), fp); \
                    fclose(fp); \
                } \
                else \
                { \
                    std::cout << "ERROR: Cannot write to " << fn << std::endl; \
                } \
            } \
        } \
        static const char *className() \
        { \
            return #_cn; \
        }

    #define IMPLEMENT_JSON_SERIALIZATION() \
        public: \
        bool deserialize(const char *s) \
        { \
            try \
            { \
                if(s != nullptr && s[0] != 0) \
                { \
                    from_json(nlohmann::json::parse(s), *this); \
                } \
                else \
                { \
                    return false; \
                } \
            } \
            catch(...) \
            { \
                return false; \
            } \
            return true; \
        } \
        \
        std::string serialize(const int indent = -1) \
        { \
            try \
            { \
                nlohmann::json j; \
                to_json(j, *this); \
                return j.dump(indent); \
            } \
            catch(...) \
            { \
                return std::string("{}"); \
            } \
        }

    #define IMPLEMENT_WRAPPED_JSON_SERIALIZATION(_cn) \
        public: \
        std::string serializeWrapped(const int indent = -1) \
        { \
            try \
            { \
                nlohmann::json j; \
                to_json(j, *this); \
                \
                std::string rc; \
                char firstChar[2]; \
                firstChar[0] = #_cn[0]; \
                firstChar[1] = 0; \
                firstChar[0] = tolower(firstChar[0]); \
                rc.assign("{\""); \
                rc.append(firstChar); \
                rc.append((#_cn) + 1); \
                rc.append("\":"); \
                rc.append(j.dump(indent)); \
                rc.append("}"); \
                \
                return rc; \
            } \
            catch(...) \
            { \
                return std::string("{}"); \
            } \
        }

    #define TOJSON_IMPL(__var) \
        {#__var, p.__var}

    #define FROMJSON_IMPL_SIMPLE(__var) \
        getOptional(#__var, p.__var, j)

    #define FROMJSON_IMPL(__var, __type, __default) \
        getOptional<__type>(#__var, p.__var, j, __default)

    #define TOJSON_BASE_IMPL() \
        to_json(j, (ConfigurationObjectBase&)p)

    #define FROMJSON_BASE_IMPL() \
        from_json(j, (ConfigurationObjectBase&)p);


    //-----------------------------------------------------------
    static std::string EMPTY_STRING;

    template<class T>
    static void getOptional(const char *name, T& v, const nlohmann::json& j, T def)
    {
        try
        {
            if(j.contains(name))
            {
                j.at(name).get_to(v);
            }
            else
            {
                v = def;    
            }
        }
        catch(...)
        {
            v = def;
        }
    }

    template<class T>
    static void getOptional(const char *name, T& v, const nlohmann::json& j)
    {
        try
        {
            if(j.contains(name))
            {
                j.at(name).get_to(v);
            }
        }
        catch(...)
        {
        }
    }

    template<class T>
    static void getOptionalWithIndicator(const char *name, T& v, const nlohmann::json& j, T def, bool *wasFound)
    {
        try
        {
            if(j.contains(name))
            {
                j.at(name).get_to(v);
                *wasFound = true;
            }
            else
            {
                v = def;
                *wasFound = false;
            }
        }
        catch(...)
        {
            v = def;
            *wasFound = false;
        }
    }

    template<class T>
    static void getOptionalWithIndicator(const char *name, T& v, const nlohmann::json& j, bool *wasFound)
    {
        try
        {
            if(j.contains(name))
            {
                j.at(name).get_to(v);
                *wasFound = true;
            }
            else
            {
                *wasFound = false;    
            }
        }
        catch(...)
        {
            *wasFound = false;
        }
    }

    class ConfigurationObjectBase
    {
    public:
        ConfigurationObjectBase()
        {
            _documenting = false;
        }

        virtual ~ConfigurationObjectBase()
        {
        }

        virtual void initForDocumenting()
        {
            _documenting = true;
        }

        virtual std::string toString()
        {
            return std::string("");
        }

        inline virtual bool isDocumenting() const
        {
            return _documenting;
        }

        nlohmann::json      _attached;

    protected:
        bool                _documenting;
    };

    static void to_json(nlohmann::json& j, const ConfigurationObjectBase& p)
    {
        try
        {
            if(p._attached != nullptr)
            {
                j[ENGAGE_CONFIGURATION_OBJECT_ATTACHED_OBJECT] = p._attached;
            }
        }
        catch(...)
        {
        }
    }
    static void from_json(const nlohmann::json& j, ConfigurationObjectBase& p)
    {
        try
        {
            if(j.contains(ENGAGE_CONFIGURATION_OBJECT_ATTACHED_OBJECT))
            {
                p._attached = j.at(ENGAGE_CONFIGURATION_OBJECT_ATTACHED_OBJECT);
            }
        }
        catch(...)
        {
        }
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(FipsCryptoSettings)
    class FipsCryptoSettings : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(FipsCryptoSettings)

    public:
        /** @brief [Optional, Default false] If true, requires FIPS140-2 crypto operation. */
        bool                        enabled;

        /** @brief Path where the crypto engine module is located  */
        std::string                 path;

        /** @brief [Optional, Default false] If true, requests the crypto engine module to run in debugging mode. */
        bool                        debug;

        FipsCryptoSettings()
        {
            clear();
        }

        void clear()
        {
            enabled = false;
            path.clear();
            debug = false;
        }

        virtual void initForDocumenting()
        {
            clear();
        }
    };

    static void to_json(nlohmann::json& j, const FipsCryptoSettings& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(path),
            TOJSON_IMPL(debug)
        };
    }
    static void from_json(const nlohmann::json& j, FipsCryptoSettings& p)
    {
        p.clear();
        FROMJSON_IMPL_SIMPLE(enabled);
        FROMJSON_IMPL_SIMPLE(path);
        FROMJSON_IMPL_SIMPLE(debug);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(WatchdogSettings)
    class WatchdogSettings : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(WatchdogSettings)

    public:
        /** @brief [Optional, Default: true] Enables/disables a watchdog. */
        bool                        enabled;

        /** @brief [Optional, Default: 5000] Interval at which checks are made. */
        int                         intervalMs;

        /** @brief [Optional, Default: 2000] Number of milliseconds that must pass before a hang is assumed. */
        int                         hangDetectionMs;

        /** @brief [Optional, Default: true] If true, aborts the process if a hang is detected. */
        bool                        abortOnHang;

        /** @brief [Optional, Default: 100] Maximum number of milliseconds that a task may take before being considered slow. */
        int                         slowExecutionThresholdMs;

        WatchdogSettings()
        {
            clear();
        }

        void clear()
        {
            enabled = true;
            intervalMs = 5000;
            hangDetectionMs = 2000;
            abortOnHang = true;
            slowExecutionThresholdMs = 100;
        }

        virtual void initForDocumenting()
        {
            clear();
        }
    };

    static void to_json(nlohmann::json& j, const WatchdogSettings& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(intervalMs),
            TOJSON_IMPL(hangDetectionMs),
            TOJSON_IMPL(abortOnHang),
            TOJSON_IMPL(slowExecutionThresholdMs)
        };
    }
    static void from_json(const nlohmann::json& j, WatchdogSettings& p)
    {
        p.clear();
        getOptional<bool>("enabled", p.enabled, j, true);
        getOptional<int>("intervalMs", p.intervalMs, j, 5000);
        getOptional<int>("hangDetectionMs", p.hangDetectionMs, j, 2000);
        getOptional<bool>("abortOnHang", p.abortOnHang, j, true);
        getOptional<int>("slowExecutionThresholdMs", p.slowExecutionThresholdMs, j, 100);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(FileRecordingRequest)
    class FileRecordingRequest : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(FileRecordingRequest)

    public:
        std::string                 id;
        std::string                 fileName;
        uint32_t                    maxMs;

        FileRecordingRequest()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            fileName.clear();
            maxMs = 60000;
        }

        virtual void initForDocumenting()
        {
            clear();
            id = "1-2-3-4-5-6-7-8-9";
            fileName = "/tmp/test.wav";
            maxMs = 10000;
        }
    };

    static void to_json(nlohmann::json& j, const FileRecordingRequest& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(fileName),
            TOJSON_IMPL(maxMs)
        };
    }
    static void from_json(const nlohmann::json& j, FileRecordingRequest& p)
    {
        p.clear();
        j.at("id").get_to(p.id);
        j.at("fileName").get_to(p.fileName);
        getOptional<uint32_t>("maxMs", p.maxMs, j, 60000);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(Feature)
    class Feature : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(Feature)

    public:
        std::string                 id;
        std::string                 name;
        std::string                 description;
        std::string                 comments;
        int                         count;
        int                         used;       // NOTE: Ignored during deserialization!

        Feature()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            name.clear();
            description.clear();
            comments.clear();
            count = 0;
            used = 0;
        }

        virtual void initForDocumenting()
        {
            clear();
            id = "{af9540d1-3e86-4fa6-8b80-e26daecb61ab}";
            name = "A sample feature";
            description = "This is an example of a feature";
            comments = "These are comments for this feature";
            count = 42;
            used = 16;
        }
    };

    static void to_json(nlohmann::json& j, const Feature& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(name),
            TOJSON_IMPL(description),
            TOJSON_IMPL(comments),
            TOJSON_IMPL(count),
            TOJSON_IMPL(used)
        };
    }
    static void from_json(const nlohmann::json& j, Feature& p)
    {
        p.clear();
        j.at("id").get_to(p.id);
        getOptional("name", p.name, j);
        getOptional("description", p.description, j);
        getOptional("comments", p.comments, j);
        getOptional("count", p.count, j, 0);

        // NOTE: Not deserialized!
        //getOptional("used", p.used, j, 0);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(Featureset)
    class Featureset : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(Featureset)

    public:
        std::string                 signature;
        bool                        lockToDeviceId;
        std::vector<Feature>        features;

        Featureset()
        {
            clear();
        }

        void clear()
        {
            signature.clear();
            lockToDeviceId = false;
            features.clear();
        }

        virtual void initForDocumenting()
        {
            clear();
            signature = "c39df3f36c6444e686e47e70fc45cf91e6ed2d8de62d4a1e89f507d567ff48aaabb1a70e54b44377b46fc4a1a2e319e5b77e4abffc444db98f8eb55d709aad5f";
            lockToDeviceId = false;
        }
    };

    static void to_json(nlohmann::json& j, const Featureset& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(signature),
            TOJSON_IMPL(lockToDeviceId),
            TOJSON_IMPL(features)
        };
    }
    static void from_json(const nlohmann::json& j, Featureset& p)
    {
        p.clear();
        getOptional("signature", p.signature, j);
        getOptional<bool>("lockToDeviceId", p.lockToDeviceId, j, false);
        getOptional<std::vector<Feature>>("features", p.features, j);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(Agc)
    /**
     * @brief Agc
     *
     * Helper C++ class to serialize and de-serialize Agc JSON
     *     *
     * Example: @include[doc] examples/Agc.json
     *
     */
    class Agc : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(Agc)

    public:
        /** @brief [Optional, Default: false] Enables automatic gain control. */
        bool enabled;

        /** @brief [Optional, Default: 0] Minimum level. */
        int minLevel;

        /** @brief [Optional, Default: 255] Maximum level. */
        int maxLevel;

        /** @brief [Optional, Default: 25, Minimum = 0, Maximum = 125] Gain in db. */        
        int compressionGainDb;

        /** @brief [Optional, Default: false] Enables limiter to prevent overdrive. */
        bool enableLimiter;

        /** @brief [Optional, Default: 9] Target gain level if there is no compression gain. */
        int targetLevelDb;

        Agc()
        {
            clear();
        }

        void clear()
        {
            enabled = false;
            minLevel = 0;
            maxLevel = 255;
            compressionGainDb = 25;
            enableLimiter = false;
            targetLevelDb = 3;
        }
    };

    static void to_json(nlohmann::json& j, const Agc& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(minLevel),
            TOJSON_IMPL(maxLevel),
            TOJSON_IMPL(compressionGainDb),
            TOJSON_IMPL(enableLimiter),
            TOJSON_IMPL(targetLevelDb)
        };
    }
    static void from_json(const nlohmann::json& j, Agc& p)
    {
        p.clear();
        getOptional<bool>("enabled", p.enabled, j, false);
        getOptional<int>("minLevel", p.minLevel, j, 0);
        getOptional<int>("maxLevel", p.maxLevel, j, 255);
        getOptional<int>("compressionGainDb", p.compressionGainDb, j, 25);
        getOptional<bool>("enableLimiter", p.enableLimiter, j, false);
        getOptional<int>("targetLevelDb", p.targetLevelDb, j, 3);
    }    


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(RtpPayloadTypeTranslation)
    /**
     * @brief RtpPayloadTypeTranslation
     *
     * Helper C++ class to serialize and de-serialize RtpPayloadTypeTranslation JSON
     *     *
     * Example: @include[doc] examples/RtpPayloadTypeTranslation.json
     *
     */
    class RtpPayloadTypeTranslation : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(RtpPayloadTypeTranslation)

    public:
        /** @brief The payload type used by the external entity */
        uint16_t        external;

        /** @brief The payload type used by Engage */
        uint16_t        engage; 

        RtpPayloadTypeTranslation()
        {
            clear();
        }

        void clear()
        {
            external = 0;
            engage = 0;
        }

        bool matches(const RtpPayloadTypeTranslation& other)
        {
            return ( (external == other.external) && (engage == other.engage) );
        }
    };

    static void to_json(nlohmann::json& j, const RtpPayloadTypeTranslation& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(external),
            TOJSON_IMPL(engage)
        };
    }
    static void from_json(const nlohmann::json& j, RtpPayloadTypeTranslation& p)
    {
        p.clear();
        getOptional<uint16_t>("external", p.external, j);
        getOptional<uint16_t>("engage", p.engage, j);
    }    

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(NetworkInterfaceDevice)
    class NetworkInterfaceDevice : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(NetworkInterfaceDevice)

    public:
        std::string                 name;
        std::string                 friendlyName;
        std::string                 description;
        int                         family;
        std::string                 address;
        bool                        available;
        bool                        isLoopback;
        bool                        supportsMulticast;
        std::string                 hardwareAddress;

        NetworkInterfaceDevice()
        {
            clear();
        }

        void clear()
        {
            name.clear();
            friendlyName.clear();
            description.clear();
            family = -1;
            address.clear();
            available = false;
            isLoopback = false;
            supportsMulticast = false;
            hardwareAddress.clear();
        }

        virtual void initForDocumenting()
        {
            clear();
            name = "en0";
            friendlyName = "Wi-Fi";
            description = "A wi-fi adapter";
            family = 1;
            address = "127.0.0.1";
            available = true;
            isLoopback = true;
            supportsMulticast = false;
            hardwareAddress = "DE:AD:BE:EF:01:02:03";
        }
    };

    static void to_json(nlohmann::json& j, const NetworkInterfaceDevice& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(name),
            TOJSON_IMPL(friendlyName),
            TOJSON_IMPL(description),
            TOJSON_IMPL(family),
            TOJSON_IMPL(address),
            TOJSON_IMPL(available),
            TOJSON_IMPL(isLoopback),
            TOJSON_IMPL(supportsMulticast),
            TOJSON_IMPL(hardwareAddress)
        };
    }
    static void from_json(const nlohmann::json& j, NetworkInterfaceDevice& p)
    {
        p.clear();
        getOptional("name", p.name, j);
        getOptional("friendlyName", p.friendlyName, j);
        getOptional("description", p.description, j);
        getOptional("family", p.family, j, -1);
        getOptional("address", p.address, j);
        getOptional("available", p.available, j, false);
        getOptional("isLoopback", p.isLoopback, j, false);
        getOptional("supportsMulticast", p.supportsMulticast, j, false);
        getOptional("hardwareAddress", p.hardwareAddress, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(ListOfNetworkInterfaceDevice)
    class ListOfNetworkInterfaceDevice : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(ListOfNetworkInterfaceDevice)

    public:
        std::vector<NetworkInterfaceDevice> list;

        ListOfNetworkInterfaceDevice()
        {
            clear();
        }

        void clear()
        {
            list.clear();
        }
    };

    static void to_json(nlohmann::json& j, const ListOfNetworkInterfaceDevice& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(list)
        };
    }
    static void from_json(const nlohmann::json& j, ListOfNetworkInterfaceDevice& p)
    {
        p.clear();
        getOptional<std::vector<NetworkInterfaceDevice>>("list", p.list, j);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(RtpHeader)
    /**
     * @brief RTP header information as per <a href="https://tools.ietf.org/html/rfc3550" target="_blank">RFC 3550</a>.
     *
     * @see BlobInfo
     *
     * Example JSON: @include[lineno] examples/RtpHeader.json
     *
     */
    class RtpHeader : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(RtpHeader)

    public:

        /** @brief A valid RTP payload between 0 and 127 <a href="https://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml" target="_blank">See IANA Real-Time Transport Protocol (RTP) Parameters</a>  */
        int         pt;

        /** @brief Indicates whether this is the start of the media stream burst. */
        bool        marker;

        /** @brief Packet sequence number. */
        uint16_t    seq;

        /** @brief Psuedo-random synchronization source. */
        uint32_t    ssrc;

        /** @brief Media sample timestamp. */
        uint32_t    ts;

        RtpHeader()
        {
            clear();
        }

        void clear()
        {
            pt = -1;
            marker = false;
            seq = 0;
            ssrc = 0;
            ts = 0;
        }

        virtual void initForDocumenting()
        {
            clear();
            pt = 0;
            marker = false;
            seq = 123;
            ssrc = 12345678;
            ts = 87654321;
        }
    };

    static void to_json(nlohmann::json& j, const RtpHeader& p)
    {
        if(p.pt != -1)
        {
            j = nlohmann::json{
                TOJSON_IMPL(pt),
                TOJSON_IMPL(marker),
                TOJSON_IMPL(seq),
                TOJSON_IMPL(ssrc),
                TOJSON_IMPL(ts)
            };
        }
    }
    static void from_json(const nlohmann::json& j, RtpHeader& p)
    {
        p.clear();
        getOptional<int>("pt", p.pt, j, -1);
        getOptional<bool>("marker", p.marker, j, false);
        getOptional<uint16_t>("seq", p.seq, j, 0);
        getOptional<uint32_t>("ssrc", p.ssrc, j, 0);
        getOptional<uint32_t>("ts", p.ts, j, 0);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(BlobInfo)
    /** @brief Describes the Blob data being sent used in the @ref engageSendGroupBlob API
     *
     *  Helper C++ class to serialize and de-serialize BlobInfo JSON
     *
     *  Example JSON: @include[lineno] examples/BlobInfo.json
     *
     *  @see engageSendGroupBlob
     */
    class BlobInfo : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(BlobInfo)

    public:
        /** @brief Payload type.
         * BlobInfo RTP supported Payload types.
         */
        typedef enum
        {
            /** @brief Unknown type */
            bptUndefined                    = 0,

            /** @brief Plain UTF 8 text. This is typically used to send plain text messages */
            bptAppTextUtf8                  = 1,

            /** @brief JSON UFT 8 text. This is used to send JSON formatted messages */
            bptJsonTextUtf8                 = 2,

            /** @brief Binary payload. Used to send binary data */
            bptAppBinary                    = 3,

            /** @brief Biometrics payload. Used to send biometric series data. <a href="https://github.com/rallytac/pub/wiki/Engage-Human-Biometrics" target="_blank">See WIKI post for more info</a> */
            bptEngageBinaryHumanBiometrics  = 4
        } PayloadType_t;

        /** @brief [Optional, Default : 0] Size of the payload */
        size_t          size;

        /** @brief [Optional, Default: empty string] The nodeId of Engage Engine that sent the message. If this is empty the Blob info will be anonymous. */
        std::string     source;

        /** @brief [Optional, Default: empty string] The nodeId to which this message is targeted. If this is empty, the Blob will targeted at all endpoints. */
        std::string     target;

        /** @brief [Optional, Default: @ref bptUndefined] The payload type to send in the blob @see PayloadType_t */
        PayloadType_t   payloadType;

        /** @brief Custom RTP header */
        RtpHeader       rtpHeader;

        BlobInfo()
        {
            clear();
        }

        void clear()
        {
            size = 0;
            source.clear();
            target.clear();
            rtpHeader.clear();
            payloadType = PayloadType_t::bptUndefined;
        }

        virtual void initForDocumenting()
        {
            clear();
            rtpHeader.initForDocumenting();
        }
    };

    static void to_json(nlohmann::json& j, const BlobInfo& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(size),
            TOJSON_IMPL(source),
            TOJSON_IMPL(target),
            TOJSON_IMPL(rtpHeader),
            TOJSON_IMPL(payloadType)
        };
    }
    static void from_json(const nlohmann::json& j, BlobInfo& p)
    {
        p.clear();
        getOptional<size_t>("size", p.size, j, 0);
        getOptional<std::string>("source", p.source, j, EMPTY_STRING);
        getOptional<std::string>("target", p.target, j, EMPTY_STRING);
        getOptional<RtpHeader>("rtpHeader", p.rtpHeader, j);
        getOptional<BlobInfo::PayloadType_t>("payloadType", p.payloadType, j, BlobInfo::PayloadType_t::bptUndefined);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(TxAudioUri)
    /**
     * @brief Optional audio streaming from a URI for @ref engageBeginGroupTxAdvanced
     *
     * Helper C++ class to serialize and de-serialize TxAudioUri JSON
     *
     * Example JSON: @include[lineno] examples/TxAudioUri.json
     *
     * TODO: Completed class properties
     *
     * @see engageBeginGroupTxAdvanced, txFlags
     */
    class TxAudioUri : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(TxAudioUri)

    public:
        /** @brief URI for the file */
        std::string     uri;

        /** @brief [Optional, Default: 0] Number of times to repeat */
        int             repeatCount;

        TxAudioUri()
        {
            clear();
        }

        void clear()
        {
            uri.clear();
            repeatCount = 0;
        }

        virtual void initForDocumenting()
        {
        }
    };

    static void to_json(nlohmann::json& j, const TxAudioUri& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(uri),
            TOJSON_IMPL(repeatCount)
        };
    }
    static void from_json(const nlohmann::json& j, TxAudioUri& p)
    {
        p.clear();
        getOptional<std::string>("uri", p.uri, j, EMPTY_STRING);
        getOptional<int>("repeatCount", p.repeatCount, j, 0);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(AdvancedTxParams)
    /**
     * @brief Configuration when using the @ref engageBeginGroupTxAdvanced API
     *
     * Helper C++ class to serialize and de-serialize AdvancedTxParams JSON
     *
     * Example JSON: @include[lineno] examples/AdvancedTxParams.json
     *
     * TODO: Completed class properties
     *
     * @see engageBeginGroupTxAdvanced, txFlags
     */
    class AdvancedTxParams : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(AdvancedTxParams)

    public:

        /** @brief [Optional, Default: 0] Combination of the ENGAGE_TXFLAG_xxx flags */
        uint16_t        flags;

        /** @brief [Optional, Default: 0] Transmit priority between 0 (lowest) and 255 (highest). */
        uint8_t         priority;

        /** @brief [Optional, Default: 0] Defines a sub channel within a group. Audio will be opaque to all other clients on the same group not on the same sub channel. */
        uint16_t        subchannelTag;

        /** @brief [Optional, Default: false] The Engage Engine should transmit the NodeId as part of the header extension in the RTP packet. */
        bool            includeNodeId;

        /** @brief [Optional, Default: empty string] The Engage Engine should transmit the user's alias as part of the header extension in the RTP packet. */
        std::string     alias;

        /** @brief [Optional, Default: false] While the microphone should be opened, captured audio should be ignored until unmuted. */
        bool            muted;

        /** @brief [Optional, Default: 0] Transmission ID */
        uint32_t        txId;

        /** @brief [Optional] A URI to stream from instead of the audio input device */
        TxAudioUri      audioUri;        

        /** @brief [Optional, Default: 0] Defines a numeric affinity value to be included in the transmission.  If defined, 3 bytes of the transmitted alias will be truncated. */
        uint16_t        aliasSpecializer;

        /** @brief [Optional, Default: false] Indicates that the aliasSpecializer must cause receivers to mute this transmission if they have an affinity for the specializer.. */
        bool            receiverRxMuteForAliasSpecializer;

        AdvancedTxParams()
        {
            clear();
        }

        void clear()
        {
            flags = 0;
            priority = 0;
            subchannelTag = 0;
            includeNodeId = false;
            alias.clear();
            muted = false;
            txId = 0;
            audioUri.clear();
            aliasSpecializer = 0;
            receiverRxMuteForAliasSpecializer = false;
        }

        virtual void initForDocumenting()
        {
        }
    };

    static void to_json(nlohmann::json& j, const AdvancedTxParams& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(flags),
            TOJSON_IMPL(priority),
            TOJSON_IMPL(subchannelTag),
            TOJSON_IMPL(includeNodeId),
            TOJSON_IMPL(alias),
            TOJSON_IMPL(muted),
            TOJSON_IMPL(txId),
            TOJSON_IMPL(audioUri),
            TOJSON_IMPL(aliasSpecializer),
            TOJSON_IMPL(receiverRxMuteForAliasSpecializer)
        };
    }
    static void from_json(const nlohmann::json& j, AdvancedTxParams& p)
    {
        p.clear();
        getOptional<uint16_t>("flags", p.flags, j, 0);
        getOptional<uint8_t>("priority", p.priority, j, 0);
        getOptional<uint16_t>("subchannelTag", p.subchannelTag, j, 0);
        getOptional<bool>("includeNodeId", p.includeNodeId, j, false);
        getOptional<std::string>("alias", p.alias, j, EMPTY_STRING);
        getOptional<bool>("muted", p.muted, j, false);
        getOptional<uint32_t>("txId", p.txId, j, 0);
        getOptional<TxAudioUri>("audioUri", p.audioUri, j);
        getOptional<uint16_t>("aliasSpecializer", p.aliasSpecializer, j, 0);
        getOptional<bool>("receiverRxMuteForAliasSpecializer", p.receiverRxMuteForAliasSpecializer, j, false);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(Identity)
    /**
     * @brief Users Identity
     *
     * Helper C++ class to serialize and de-serialize Identity JSON
     *
     * This configuration will be used by the Engine when transmitting presence data on behalf of the application.
     *
     * Example JSON: @include[lineno] examples/Identity.json
     *
     * @see engageInitialize, PresenceDescriptor
     */
    class Identity : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(Identity)

    public:
        /**
         * @brief [Optional, Default: Auto Generated] This is the Node ID to use to represent instance on the network.
         *
         * The application can generate and persist this Id or if an Id is not provided, the Engage Engine will generate
            a random Id and will be advertised in the PresenceDescriptor JSON with the <b>self</b> attribute
            set to <b>true</b> in the PFN_ENGAGE_GROUP_NODE_DISCOVERED event @see PresenceDescriptor.
         */
        std::string     nodeId;

        /** @brief [Optional, Default: empty string] The user ID to be used to represent the user. */
        std::string     userId;

        /** @brief [Optional, Default: empty string] The display name to be used for the user. */
        std::string     displayName;

        /** @brief [Optional, Default: empty string] This is a application defined field used to indicate a users avatar. The Engine doesn't not process this data and is a pass through. */
        std::string     avatar;

        Identity()
        {
            clear();
        }

        void clear()
        {
            nodeId.clear();
            userId.clear();
            displayName.clear();
            avatar.clear();
        }

        virtual void initForDocumenting()
        {
        }
    };

    static void to_json(nlohmann::json& j, const Identity& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(nodeId),
            TOJSON_IMPL(userId),
            TOJSON_IMPL(displayName),
            TOJSON_IMPL(avatar)
        };
    }
    static void from_json(const nlohmann::json& j, Identity& p)
    {
        p.clear();
        getOptional<std::string>("nodeId", p.nodeId, j);
        getOptional<std::string>("userId", p.userId, j);
        getOptional<std::string>("displayName", p.displayName, j);
        getOptional<std::string>("avatar", p.avatar, j);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(Location)
    /**
     * @brief Location information used as part of the @ref PresenceDescriptor
     *
     * Helper C++ class to serialize and de-serialize Location JSON
     *
     * Location is presented by using the <a href="https://gisgeography.com/wgs84-world-geodetic-system/" target="_blank">World Geodetic System (WGS84)</a>
     *
     * Example JSON: @include[lineno] examples/Location.json
     *
     *  @see PresenceDescriptor
     */
    class Location : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(Location)

    public:
        constexpr static double INVALID_LOCATION_VALUE = -999.999;

        /** @brief [Read Only: Unix timestamp - Zulu/UTC] Indicates the timestamp that the location was recorded. */
        uint32_t                    ts;

        /** @brief Its the latitude position using the using the <b>Signed degrees format</b> (DDD.dddd). Valid range is -90 to 90. */
        double                      latitude;

        /** @brief Its the longitudinal position using the <b>Signed degrees format</b> (DDD.dddd) format. Valid range is -180 to 180. */
        double                      longitude;

        /** @brief [Optional, Default: INVALID_LOCATION_VALUE] The altitude above sea level in meters. */
        double                      altitude;

        /** @brief [Optional, Default: INVALID_LOCATION_VALUE] Direction the endpoint is traveling in degrees. Valid range is 0 to 360 (NOTE: 0 and 360 are both the same heading). */
        double                      direction;

        /** @brief [Optional, Default: INVALID_LOCATION_VALUE] The speed the endpoint is traveling at in meters per second. */
        double                      speed;

        Location()
        {
            clear();
        }

        void clear()
        {
            ts = 0;
            latitude = INVALID_LOCATION_VALUE;
            longitude = INVALID_LOCATION_VALUE;
            altitude = INVALID_LOCATION_VALUE;
            direction = INVALID_LOCATION_VALUE;
            speed = INVALID_LOCATION_VALUE;
        }

        virtual void initForDocumenting()
        {
            clear();

            ts = 123456;
            latitude = 123.456;
            longitude = 456.789;
            altitude = 123;
            direction = 1;
            speed = 1234;
        }
    };

    static void to_json(nlohmann::json& j, const Location& p)
    {
        if(p.latitude != Location::INVALID_LOCATION_VALUE && p.longitude != Location::INVALID_LOCATION_VALUE)
        {
            j = nlohmann::json{
                TOJSON_IMPL(latitude),
                TOJSON_IMPL(longitude),
            };

            if(p.ts != 0) j["ts"] = p.ts;
            if(p.altitude != Location::INVALID_LOCATION_VALUE) j["altitude"] = p.altitude;
            if(p.speed != Location::INVALID_LOCATION_VALUE) j["speed"] = p.speed;
            if(p.direction != Location::INVALID_LOCATION_VALUE) j["direction"] = p.direction;
        }
    }
    static void from_json(const nlohmann::json& j, Location& p)
    {
        p.clear();
        getOptional<uint32_t>("ts", p.ts, j, 0);
        j.at("latitude").get_to(p.latitude);
        j.at("longitude").get_to(p.longitude);
        getOptional<double>("altitude", p.altitude, j, Location::INVALID_LOCATION_VALUE);
        getOptional<double>("direction", p.direction, j, Location::INVALID_LOCATION_VALUE);
        getOptional<double>("speed", p.speed, j, Location::INVALID_LOCATION_VALUE);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(Power)
    /**
     * @brief Device Power Information used as part of the @ref PresenceDescriptor
     *
     *  Helper C++ class to serialize and de-serialize Power JSON
     *
     * Example JSON: @include[lineno] examples/Power.json
     *
     *  @see PresenceDescriptor
     */
    class Power : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(Power)

    public:

        /** @brief [Optional, Default: 0] Is the source the power is being delivered from
         *
         * Valid values are
         *
         * Value | Description
         * --- | ---
         *  0 | Undefined
         *  1 | Battery
         *  2 | Wired
         *
         */
        int     source;

        /** @brief [Optional, Default: 0] Is the current state that the power system is in.
         *
         * Valid values are
         *
         * Value | Description
         * --- | ---
         *  0 | Undefined
         *  1 | Charging
         *  2 | Discharging
         *  3 | Not Charging
         *  4 | Full
         */
        int     state;

        /** @brief [Optional, Default: 0] Is the current level of the battery or power system as a percentage. Valid range is 0 to 100. */
        int     level;

        Power()
        {
            clear();
        }

        void clear()
        {
            source = 0;
            state = 0;
            level = 0;
        }

        virtual void initForDocumenting()
        {
        }
    };

    static void to_json(nlohmann::json& j, const Power& p)
    {
        if(p.source != 0 && p.state != 0 && p.level != 0)
        {
            j = nlohmann::json{
                TOJSON_IMPL(source),
                TOJSON_IMPL(state),
                TOJSON_IMPL(level)
            };
        }
    }
    static void from_json(const nlohmann::json& j, Power& p)
    {
        p.clear();
        getOptional<int>("source", p.source, j, 0);
        getOptional<int>("state", p.state, j, 0);
        getOptional<int>("level", p.level, j, 0);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(Connectivity)
    /**
     * @brief Connectivity Information used as part of the @ref PresenceDescriptor
     *
     *  Helper C++ class to serialize and de-serialize Connectivity JSON
     *
     * Example JSON: @include[lineno] examples/Connectivity.json
     *
     *  @see PresenceDescriptor
     */
    class Connectivity : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(Connectivity)

    public:
        /**
         * @brief Is the type of connectivity the device has to the network
         *
         * Valid values are
         *
         * Value | Description
         * --- | ---
         *  0 | Undefined
         *  1 | Wired
         *  2 | Wireless WiFi
         *  3 | Wireless Cellular
         *
         */
        int     type;

        /** @brief Is the strength of the connection connection as reported by the OS - usually in dbm. */
        int     strength;

        /** @brief Is the quality of the network connection as reported by the OS - OS dependent. */
        int     rating;

        Connectivity()
        {
            clear();
        }

        void clear()
        {
            type = 0;
            strength = 0;
            rating = 0;
        }

        virtual void initForDocumenting()
        {
            clear();

            type = 1;
            strength = 2;
            rating = 3;
        }
    };

    static void to_json(nlohmann::json& j, const Connectivity& p)
    {
        if(p.type != 0)
        {
            j = nlohmann::json{
                TOJSON_IMPL(type),
                TOJSON_IMPL(strength),
                TOJSON_IMPL(rating)
            };
        }
    }
    static void from_json(const nlohmann::json& j, Connectivity& p)
    {
        p.clear();
        getOptional<int>("type", p.type, j, 0);
        getOptional<int>("strength", p.strength, j, 0);
        getOptional<int>("rating", p.rating, j, 0);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(PresenceDescriptorGroupItem)
    /**
     * @brief Group Alias used as part of the @ref PresenceDescriptor
     *
     *  Helper C++ class to serialize and de-serialize PresenceDescriptorGroupItem JSON
     *
     * Example JSON: @include[lineno] examples/PresenceDescriptorGroupItem.json
     *
     *  @see PresenceDescriptor
     */
    class PresenceDescriptorGroupItem : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(PresenceDescriptorGroupItem)

    public:
        /** @brief Group Id the alias is associated with. */
        std::string     groupId;

        /** @brief User's alias for the group. */
        std::string     alias;

        /** @brief Status flags for the user's participation on the group */
        uint16_t        status;

        PresenceDescriptorGroupItem()
        {
            clear();
        }

        void clear()
        {
            groupId.clear();
            alias.clear();
            status = 0;
        }

        virtual void initForDocumenting()
        {
            groupId = "{123-456}";
            alias = "MYALIAS";
            status = 0;
        }
    };

    static void to_json(nlohmann::json& j, const PresenceDescriptorGroupItem& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(groupId),
            TOJSON_IMPL(alias),
            TOJSON_IMPL(status)
        };
    }
    static void from_json(const nlohmann::json& j, PresenceDescriptorGroupItem& p)
    {
        p.clear();
        getOptional<std::string>("groupId", p.groupId, j);
        getOptional<std::string>("alias", p.alias, j);
        getOptional<uint16_t>("status", p.status, j);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(PresenceDescriptor)
    /**
     * @brief Represents an endpoints presence properties. Used in @ref engageUpdatePresenceDescriptor API and PFN_ENGAGE_GROUP_NODE events.
     *
     *  Helper C++ class to serialize and de-serialize PresenceDescriptor JSON
     *
     *  Example: @include[doc] examples/PresenceDescriptor.json
     *
     *  @see engageUpdatePresenceDescriptor, PFN_ENGAGE_GROUP_NODE_DISCOVERED, PFN_ENGAGE_GROUP_NODE_REDISCOVERED
     */
    class PresenceDescriptor : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(PresenceDescriptor)

    public:

        /**
         * @brief [Read Only] Indicates that this presence declaration was generated by the Engage Engine the application currently connected to.
         *
         * NOTE: This is useful in identifying the nodeId that is automatically generated by the Engage Engine when the application does not provide the nodeId.
         */
        bool                        self;

        /**
         * @brief [Read Only, Unix timestamp - Zulu/UTC] Indicates the timestamp that the message was originally sent.
         *
         * Use this property and the @ref nextUpdate attribute to determine if the endpoint is still on the network.
         */
        uint32_t                    ts;

        /**
         * @brief [Read Only, Unix timestamp - Zulu/UTC] Indicates the next time the presence descriptor will be sent.
         *
         *   This attributed together with the @ref ts attribute is used by the Engage Engage Engine to "timeout" the endpoint. (Unix timestamp - Zulu/UTC)
         */
        uint32_t                    nextUpdate;

        /** @brief [Optional, Default see @ref Identity] Endpoint's identity information. */
        Identity                    identity;

        /** @brief [Optional] No defined limit on size but the total size of the serialized JSON object must fit inside the current network transport's MTU. */
        std::string                 comment;

        /**
         * @brief [Optional] Indicates the users disposition
         *
         * This may be any value set by the application.  Such as:
         *
         * Value | Description
         * --- | ---
         *  0x00000000 | Undefined
         *  0x00000001 | Emergency
         *  0x00000002 | Available
         *  0x00000004 | Busy
         *  etc | etc
         */
        uint32_t                    disposition;

        /** @brief [Read Only] List of group items associated with this presence descriptor. */
        std::vector<PresenceDescriptorGroupItem>        groupAliases;

        /** @brief [Optional, Default: see @ref Location] Location information @see Location for more info. */
        Location                    location;

        /** @brief [Optional, Default: empty string] Custom string application can use of presence descriptor. TODO: Whats the max length. */
        std::string                 custom;

        /** @brief [Read Only] Indicates that the Engine will announce its PresenceDescriptor in response to this message. */
        bool                        announceOnReceive;

        /** @brief [Optional, Default: see @ref Connectivity] Device connectivity information like wifi/cellular, strength etc. */
        Connectivity                connectivity;

        /** @brief [Optional, Default: see @ref Power] Device power information like charging state, battery level, etc. */
        Power                       power;

        PresenceDescriptor()
        {
            clear();
        }

        void clear()
        {
            self = false;
            ts = 0;
            nextUpdate = 0;
            identity.clear();
            comment.clear();
            disposition = 0;
            groupAliases.clear();
            location.clear();
            custom.clear();
            announceOnReceive = false;
            connectivity.clear();
            power.clear();
        }

        virtual void initForDocumenting()
        {
            clear();

            self = true;
            ts = 123;
            nextUpdate = 0;
            identity.initForDocumenting();
            comment = "This is a comment";
            disposition = 123;

            PresenceDescriptorGroupItem gi;
            gi.initForDocumenting();
            groupAliases.push_back(gi);

            location.initForDocumenting();
            custom = "{}";
            announceOnReceive = true;
            connectivity.initForDocumenting();
            power.initForDocumenting();
        }
    };

    static void to_json(nlohmann::json& j, const PresenceDescriptor& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(ts),
            TOJSON_IMPL(nextUpdate),
            TOJSON_IMPL(identity),
            TOJSON_IMPL(comment),
            TOJSON_IMPL(disposition),
            TOJSON_IMPL(groupAliases),
            TOJSON_IMPL(location),
            TOJSON_IMPL(custom),
            TOJSON_IMPL(announceOnReceive),
            TOJSON_IMPL(connectivity),
            TOJSON_IMPL(power)
        };

        if(!p.comment.empty()) j["comment"] = p.comment;
        if(!p.custom.empty()) j["custom"] = p.custom;

        if(p.self)
        {
            j["self"] = true;
        }
    }
    static void from_json(const nlohmann::json& j, PresenceDescriptor& p)
    {
        p.clear();
        getOptional<bool>("self", p.self, j);
        getOptional<uint32_t>("ts", p.ts, j);
        getOptional<uint32_t>("nextUpdate", p.nextUpdate, j);
        getOptional<Identity>("identity", p.identity, j);
        getOptional<std::string>("comment", p.comment, j);
        getOptional<uint32_t>("disposition", p.disposition, j);
        getOptional<std::vector<PresenceDescriptorGroupItem>>("groupAliases", p.groupAliases, j);
        getOptional<Location>("location", p.location, j);
        getOptional<std::string>("custom", p.custom, j);
        getOptional<bool>("announceOnReceive", p.announceOnReceive, j);
        getOptional<Connectivity>("connectivity", p.connectivity, j);
        getOptional<Power>("power", p.power, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(NetworkTxOptions)
    /**
     * @brief Network Transmit Options
     *
     *  Helper C++ class to serialize and de-serialize NetworkTxOptions JSON
     *
     *  TODO: Complete this Class
     *
     *  Example: @include[doc] examples/NetworkTxOptions.json
     *
     *  @see Group
     */
    class NetworkTxOptions : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(NetworkTxOptions)

    public:

        /**
         * @brief Network Transmission Priority.
         *
         *   Priority to set on the packets in order for the network to optimize routing of the packets
         */
        typedef enum
        {
            /** @brief best effort */
            priBestEffort   = 0,

            /** @brief signaling */
            priSignaling    = 1,

            /** @brief video */
            priVideo        = 2,

            /** @brief voice */
            priVoice        = 3
        } TxPriority_t;

        /** @brief [Optional, Default: @ref priVoice] Transmission priority. This has meaning on some operating systems based on how their IP stack operates.  It may or may not affect final packet marking. */
        TxPriority_t    priority;

        /**
         * @brief [Optional, Default: 1] Time to live or hop limit is a mechanism that limits the lifespan or lifetime of data in a network. TTL prevents a data packet from circulating indefinitely.
         *
         *   E.g If you don't want multicast data to leave your local network, set the TTL to 1.
         */
        int             ttl;

        NetworkTxOptions()
        {
            clear();
        }

        void clear()
        {
            priority = priVoice;
            ttl = 1;
        }

        virtual void initForDocumenting()
        {
        }
    };

    static void to_json(nlohmann::json& j, const NetworkTxOptions& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(priority),
            TOJSON_IMPL(ttl)
        };
    }
    static void from_json(const nlohmann::json& j, NetworkTxOptions& p)
    {
        p.clear();
        getOptional<NetworkTxOptions::TxPriority_t>("priority", p.priority, j, NetworkTxOptions::priVoice);
        getOptional<int>("ttl", p.ttl, j, 1);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(TcpNetworkTxOptions)
    /**
     * @brief Network Transmit Options for TCP
     *
     *  Helper C++ class to serialize and de-serialize TcpNetworkTxOptions JSON
     *
     *  Example: @include[doc] examples/TcpNetworkTxOptions.json
     */
    class TcpNetworkTxOptions : public NetworkTxOptions
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(TcpNetworkTxOptions)

    public:
        TcpNetworkTxOptions()
        {
            clear();
        }

        void clear()
        {
            priority = priVoice;
            ttl = -1;
        }

        virtual void initForDocumenting()
        {
        }
    };

    static void to_json(nlohmann::json& j, const TcpNetworkTxOptions& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(priority),
            TOJSON_IMPL(ttl)
        };
    }
    static void from_json(const nlohmann::json& j, TcpNetworkTxOptions& p)
    {
        p.clear();
        getOptional<NetworkTxOptions::TxPriority_t>("priority", p.priority, j, NetworkTxOptions::priVoice);
        getOptional<int>("ttl", p.ttl, j, -1);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(NetworkAddress)
    /**
     * @brief NetworkAddress
     *
     * Helper C++ class to serialize and de-serialize NetworkAddress JSON
     *
     * TODO: Complete this Class
     *
     * Example: @include[doc] examples/NetworkAddress.json
     *
     */
    class NetworkAddress : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(NetworkAddress)

    public:
        /** @brief IP address. */
        std::string     address;

        /** @brief Port. */
        int             port;

        NetworkAddress()
        {
            clear();
        }

        void clear()
        {
            address.clear();
            port = 0;
        }

        bool matches(const NetworkAddress& other)
        {
            if(address.compare(other.address) != 0)
            {
                return false;
            }

            if(port != other.port)
            {
                return false;
            }

            return true;
        }
    };

    static void to_json(nlohmann::json& j, const NetworkAddress& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(address),
            TOJSON_IMPL(port)
        };
    }
    static void from_json(const nlohmann::json& j, NetworkAddress& p)
    {
        p.clear();
        getOptional<std::string>("address", p.address, j);
        getOptional<int>("port", p.port, j);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(NetworkAddressRxTx)
    /**
     * @brief NetworkAddressRxTx
     *
     * Helper C++ class to serialize and de-serialize NetworkAddressRxTx JSON
     *
     * TODO: Complete this Class
     *
     * Example: @include[doc] examples/NetworkAddressRxTx.json
     *
     */
    class NetworkAddressRxTx : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(NetworkAddressRxTx)

    public:
        /** @brief RX. */
        NetworkAddress          rx;

        /** @brief TX. */
        NetworkAddress          tx;

        NetworkAddressRxTx()
        {
            clear();
        }

        void clear()
        {
            rx.clear();
            tx.clear();
        }
    };

    static void to_json(nlohmann::json& j, const NetworkAddressRxTx& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(rx),
            TOJSON_IMPL(tx)
        };
    }
    static void from_json(const nlohmann::json& j, NetworkAddressRxTx& p)
    {
        p.clear();
        getOptional<NetworkAddress>("rx", p.rx, j);
        getOptional<NetworkAddress>("tx", p.tx, j);
    }

    /** @brief Enum describing restriction types. */
    typedef enum
    {
        /** @brief Undefined */
        rtUndefined = 0,

        /** @brief Elements are whitelisted */
        rtWhitelist = 1,

        /** @brief Elements are blacklisted */
        rtBlacklist = 2
    } RestrictionType_t;

    static bool isValidRestrictionType(RestrictionType_t t)
    {
        return (t == RestrictionType_t::rtUndefined ||
                t == RestrictionType_t::rtWhitelist ||
                t == RestrictionType_t::rtBlacklist );
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(NetworkAddressRestrictionList)
    /**
     * @brief NetworkAddressRestrictionList
     *
     * Helper C++ class to serialize and de-serialize NetworkAddressRestrictionList JSON
     *
     * TODO: Complete this Class
     *
     * Example: @include[doc] examples/NetworkAddressRestrictionList.json
     *
     */
    class NetworkAddressRestrictionList : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(NetworkAddressRestrictionList)

    public:
        /** @brief Type indicating how the elements are to be treated **/
        RestrictionType_t                   type;

        /** @brief List of elements */
        std::vector<NetworkAddressRxTx>     elements;

        NetworkAddressRestrictionList()
        {
            clear();
        }

        void clear()
        {
            type = RestrictionType_t::rtUndefined;
            elements.clear();
        }
    };

    static void to_json(nlohmann::json& j, const NetworkAddressRestrictionList& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(type),
            TOJSON_IMPL(elements)
        };
    }
    static void from_json(const nlohmann::json& j, NetworkAddressRestrictionList& p)
    {
        p.clear();
        getOptional<RestrictionType_t>("type", p.type, j, RestrictionType_t::rtUndefined);
        getOptional<std::vector<NetworkAddressRxTx>>("elements", p.elements, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(StringRestrictionList)
    /**
     * @brief StringRestrictionList
     *
     * Helper C++ class to serialize and de-serialize StringRestrictionList JSON
     *
     * TODO: Complete this Class
     *
     * Example: @include[doc] examples/StringRestrictionList.json
     *
     */
    class StringRestrictionList : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(StringRestrictionList)

    public:
        /** @brief Type indicating how the elements are to be treated **/
        RestrictionType_t                   type;

        /** @brief List of elements */
        std::vector<std::string>     elements;

        StringRestrictionList()
        {
            type = RestrictionType_t::rtUndefined;
            clear();
        }

        void clear()
        {
            elements.clear();
        }
    };

    static void to_json(nlohmann::json& j, const StringRestrictionList& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(type),
            TOJSON_IMPL(elements)
        };
    }
    static void from_json(const nlohmann::json& j, StringRestrictionList& p)
    {
        p.clear();
        getOptional<RestrictionType_t>("type", p.type, j, RestrictionType_t::rtUndefined);
        getOptional<std::vector<std::string>>("elements", p.elements, j);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(PacketCapturer)
    /**
     * @brief Description of a packet capturer
     *
     * Helper C++ class to serialize and de-serialize PacketCapturer JSON.
     *
     * Example: @include[doc] examples/PacketCapturer.json
     *
     */
    class PacketCapturer : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(PacketCapturer)

    public:
        bool            enabled;
        uint32_t        maxMb;
        std::string     filePrefix;

        PacketCapturer()
        {
            clear();
        }

        void clear()
        {
            enabled = false;
            maxMb = 10;
            filePrefix.clear();
        }
    };

    static void to_json(nlohmann::json& j, const PacketCapturer& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(maxMb),
            TOJSON_IMPL(filePrefix)
        };
    }
    static void from_json(const nlohmann::json& j, PacketCapturer& p)
    {
        p.clear();
        getOptional<bool>("enabled", p.enabled, j, false);
        getOptional<uint32_t>("maxMb", p.maxMb, j, 10);
        getOptional<std::string>("filePrefix", p.filePrefix, j, EMPTY_STRING);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(TransportImpairment)
    /**
     * @brief Description of a transport impairment
     *
     * Helper C++ class to serialize and de-serialize TransportImpairment JSON.
     *
     * Example: @include[doc] examples/TransportImpairment.json
     *
     */
    class TransportImpairment : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(TransportImpairment)

    public:
        int     applicationPercentage;
        int     jitterMs;
        int     lossPercentage;

        TransportImpairment()
        {
            clear();
        }

        void clear()
        {
            applicationPercentage = 0;
            jitterMs = 0;
            lossPercentage = 0;
        }
    };

    static void to_json(nlohmann::json& j, const TransportImpairment& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(applicationPercentage),
            TOJSON_IMPL(jitterMs),
            TOJSON_IMPL(lossPercentage)
        };
    }
    static void from_json(const nlohmann::json& j, TransportImpairment& p)
    {
        p.clear();
        getOptional<int>("applicationPercentage", p.applicationPercentage, j, 0);
        getOptional<int>("jitterMs", p.jitterMs, j, 0);
        getOptional<int>("lossPercentage", p.lossPercentage, j, 0);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(NsmNetworking)
    /**
     * @brief NsmNetworking
     *
     * Helper C++ class to serialize and de-serialize NsmNetworking JSON
     * 
     * Example: @include[doc] examples/NsmNetworking.json
     *
     */
    class NsmNetworking : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(NsmNetworking)

    public:        
        std::string                         interfaceName;
        NetworkAddress                      address;
        int                                 ttl;
        int                                 tos;
        int                                 txOversend;
        TransportImpairment                 rxImpairment;
        TransportImpairment                 txImpairment;
        std::string                         cryptoPassword;

        NsmNetworking()
        {
            clear();
        }

        void clear()
        {
            interfaceName.clear();
            address.clear();
            ttl = 1;
            tos = 56;
            txOversend = 0;
            rxImpairment.clear();
            txImpairment.clear();
            cryptoPassword.clear();
        }
    };

    static void to_json(nlohmann::json& j, const NsmNetworking& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(interfaceName),
            TOJSON_IMPL(address),
            TOJSON_IMPL(ttl),
            TOJSON_IMPL(tos),
            TOJSON_IMPL(txOversend),
            TOJSON_IMPL(rxImpairment),
            TOJSON_IMPL(txImpairment),
            TOJSON_IMPL(cryptoPassword)
        };
    }
    static void from_json(const nlohmann::json& j, NsmNetworking& p)
    {
        p.clear();
        getOptional("interfaceName", p.interfaceName, j, EMPTY_STRING);
        getOptional<NetworkAddress>("address", p.address, j);
        getOptional<int>("ttl", p.ttl, j, 1);
        getOptional<int>("tos", p.tos, j, 56);
        getOptional<int>("txOversend", p.txOversend, j, 0);
        getOptional<TransportImpairment>("rxImpairment", p.rxImpairment, j);
        getOptional<TransportImpairment>("txImpairment", p.txImpairment, j);
        getOptional("cryptoPassword", p.cryptoPassword, j, EMPTY_STRING);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(NsmConfiguration)
    /**
     * @brief NsmConfiguration
     *
     * Helper C++ class to serialize and de-serialize NsmConfiguration JSON
     * 
     * Example: @include[doc] examples/NsmConfiguration.json
     *
     */
    class NsmConfiguration : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(NsmConfiguration)

    public:

        std::string                         id;
        bool                                favorUptime;
        NsmNetworking                       networking;
        std::vector<std::string>            resources;
        int                                 tokenStart;
        int                                 tokenEnd;
        int                                 intervalSecs;
        int                                 transitionSecsFactor;

        NsmConfiguration()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            favorUptime = false;
            networking.clear();
            resources.clear();
            tokenStart = 1000000;
            tokenEnd = 2000000;
            intervalSecs = 1;
            transitionSecsFactor = 3;
        }
    };

    static void to_json(nlohmann::json& j, const NsmConfiguration& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(favorUptime),
            TOJSON_IMPL(networking),
            TOJSON_IMPL(resources),
            TOJSON_IMPL(tokenStart),
            TOJSON_IMPL(tokenEnd),
            TOJSON_IMPL(intervalSecs),
            TOJSON_IMPL(transitionSecsFactor)
        };
    }
    static void from_json(const nlohmann::json& j, NsmConfiguration& p)
    {
        p.clear();
        getOptional("id", p.id, j);
        getOptional<bool>("favorUptime", p.favorUptime, j, false);
        getOptional<NsmNetworking>("networking", p.networking, j);
        getOptional<std::vector<std::string>>("resources", p.resources, j);
        getOptional<int>("tokenStart", p.tokenStart, j, 1000000);
        getOptional<int>("tokenEnd", p.tokenEnd, j, 2000000);
        getOptional<int>("intervalSecs", p.intervalSecs, j, 1);
        getOptional<int>("transitionSecsFactor", p.transitionSecsFactor, j, 3);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(Rallypoint)
    /**
     * @brief Rallypoint
     *
     * Helper C++ class to serialize and de-serialize Rallypoint JSON
     *
     * Example: @include[doc] examples/Rallypoint.json
     */
    class Rallypoint : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(Rallypoint)

    public:

        /**
         * @brief This is the host address for the Engine to connect to the RallyPoint service.
         *
         */
        NetworkAddress              host;

        /**
         * @brief This is the X509 certificate to use for mutual authentication.
         *
         * The full contents of the cert can be specified or the file path to where the cert is located can be specified by using an @ replacement string e.g
         *
         *  \code{.json}
             certificate="@/privatestore/certs/rp_cert.pem"
         *  \endcode
         *
         *
         */
        std::string                 certificate;

        /**
         * @brief This is the private key used to generate the X509 certificate.
         *
         * The full contents of the key can be specified or the file path to where the key is located can be specified by using an @ replacement string e.g
         *
         *  \code{.json}
             certificate="@/privatestore/certs/rp_private_key.key"
         *  \endcode
         *
         *
         */
        std::string                 certificateKey;

        /**
         * @brief [Optional, Default true] Indicates whether the connection peer is to be verified by checking the validaity of its X.509 certificate.
         */
        bool                        verifyPeer;

        /**
         * @brief [Optional, Default false] Allows the Rallypoint to accept self-signed certificates from the far-end
         */
        bool                        allowSelfSignedCertificate;

        /**
         * @brief [Optional] A vector of certificates (raw content, file names, or certificate store elements) used to verify far-end X.509 certificates.
         */
        std::vector<std::string>    caCertificates;

        /**
         * @brief [Optional, Default 5000] Number of milliseconds that a transaction may take before the link is considered broken.
         */
        int                         transactionTimeoutMs;

        /**
         * @brief [Optional, Default false] Indicates whether to forego ECSDA signing of control-plane messages.
         */
        bool                        disableMessageSigning;

        /** @brief [Optional, Default: 5] Connection timeout in seconds to the RP */
        int                         connectionTimeoutSecs;

        /** @brief [Optional] Tx options for the TCP link */
        TcpNetworkTxOptions         tcpTxOptions;

        Rallypoint()
        {
            clear();
        }

        void clear()
        {
            host.clear();
            certificate.clear();
            certificateKey.clear();
            caCertificates.clear();
            verifyPeer = false;
            transactionTimeoutMs = 5000;
            disableMessageSigning = false;
            connectionTimeoutSecs = 5;
            tcpTxOptions.clear();
        }

        bool matches(const Rallypoint& other)
        {
            if(!host.matches(other.host))
            {
                return false;
            }

            if(certificate.compare(other.certificate) != 0)
            {
                return false;
            }

            if(certificateKey.compare(other.certificateKey) != 0)
            {
                return false;
            }

            if(verifyPeer != other.verifyPeer)
            {
                return false;
            }

            if(allowSelfSignedCertificate != other.allowSelfSignedCertificate)
            {
                return false;
            }

            if(caCertificates.size() != other.caCertificates.size())
            {
                return false;
            }

            for(size_t x = 0; x < caCertificates.size(); x++)
            {
                bool found = false;

                for(size_t y = 0; y < other.caCertificates.size(); y++)
                {
                    if(caCertificates[x].compare(other.caCertificates[y]) == 0)
                    {
                        found = true;
                        break;
                    }
                }

                if(!found)
                {
                    return false;
                }
            }

            if(transactionTimeoutMs != other.transactionTimeoutMs)
            {
                return false;
            }

            if(disableMessageSigning != other.disableMessageSigning)
            {
                return false;
            }

            return true;
        }
    };

    static void to_json(nlohmann::json& j, const Rallypoint& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(host),
            TOJSON_IMPL(certificate),
            TOJSON_IMPL(certificateKey),
            TOJSON_IMPL(verifyPeer),
            TOJSON_IMPL(allowSelfSignedCertificate),
            TOJSON_IMPL(caCertificates),
            TOJSON_IMPL(transactionTimeoutMs),
            TOJSON_IMPL(disableMessageSigning),
            TOJSON_IMPL(connectionTimeoutSecs),
            TOJSON_IMPL(tcpTxOptions)
        };
    }

    static void from_json(const nlohmann::json& j, Rallypoint& p)
    {
        p.clear();
        j.at("host").get_to(p.host);
        getOptional("certificate", p.certificate, j);
        getOptional("certificateKey", p.certificateKey, j);
        getOptional<bool>("verifyPeer", p.verifyPeer, j, true);
        getOptional<bool>("allowSelfSignedCertificate", p.allowSelfSignedCertificate, j, false);
        getOptional<std::vector<std::string>>("caCertificates", p.caCertificates, j);
        getOptional<int>("transactionTimeoutMs", p.transactionTimeoutMs, j, 5000);
        getOptional<bool>("disableMessageSigning", p.disableMessageSigning, j, false);
        getOptional<int>("connectionTimeoutSecs", p.connectionTimeoutSecs, j, 5);
        getOptional<TcpNetworkTxOptions>("tcpTxOptions", p.tcpTxOptions, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(RallypointCluster)
    /**
     * @brief RallypointCluster
     *
     * Helper C++ class to serialize and de-serialize RallypointCluster JSON
     *
     * TODO: Complete this Class
     *
     * Example: @include[doc] examples/RallypointCluster.json
     *
     */
    class RallypointCluster : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(RallypointCluster)

    public:
        /**
         * @brief Connection strategy enum.
         *
         * More detailed ConnectionStrategy_t description.
         */
        typedef enum
        {
            /** @brief Connect in round-robin fashion */
            csRoundRobin    = 0,

            /** @brief Fail back to high-order RP when available */
            csFailback      = 1
        } ConnectionStrategy_t;

        /** @brief [Optional, Default: @ref csRoundRobin] Specifies the connection strategy to be followed. See @ref ConnectionStrategy_t for all strategy types */
        ConnectionStrategy_t                connectionStrategy;

        /** @brief List of Rallypoints */
        std::vector<Rallypoint>             rallypoints;

        /** @brief Seconds between switching to a new target */
        int                                 rolloverSecs;

        /** @brief [Optional, Default: 0] Default connection timeout in seconds to any RP in the cluster */
        int                                 connectionTimeoutSecs;

        RallypointCluster()
        {
            clear();
        }

        void clear()
        {
            connectionStrategy = csRoundRobin;
            rallypoints.clear();
            rolloverSecs = 10;
            connectionTimeoutSecs = 5;
        }
    };

    static void to_json(nlohmann::json& j, const RallypointCluster& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(connectionStrategy),
            TOJSON_IMPL(rallypoints),
            TOJSON_IMPL(rolloverSecs),
            TOJSON_IMPL(connectionTimeoutSecs)
        };
    }
    static void from_json(const nlohmann::json& j, RallypointCluster& p)
    {
        p.clear();
        getOptional<RallypointCluster::ConnectionStrategy_t>("connectionStrategy", p.connectionStrategy, RallypointCluster::ConnectionStrategy_t::csRoundRobin);
        getOptional<std::vector<Rallypoint>>("rallypoints", p.rallypoints, j);
        getOptional<int>("rolloverSecs", p.rolloverSecs, j, 10);
        getOptional<int>("connectionTimeoutSecs", p.connectionTimeoutSecs, j, 5);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(NetworkDeviceDescriptor)
    /**
     * @brief Custom Network Device Configuration
     *
     * Helper C++ class to serialize and de-serialize NetworkDeviceDescriptor JSON used in @ref engageNetworkDeviceRegister API.
     *
     * Example: @include[doc] examples/NetworkDeviceDescriptor.json
     *
     * @see engageNetworkDeviceRegister
     */
    class NetworkDeviceDescriptor : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(NetworkDeviceDescriptor)

    public:
        /**
         * @brief [Read Only] Unique device identifier assigned by Engage Engine at time of device creation.
         *
         */
        int             deviceId;

        /** @brief Name of the device assigned by the platform */
        std::string     name;

        /** @brief Device manufacturer (if any) */
        std::string     manufacturer;

        /** @brief Device mode (if any) */
        std::string     model;

        /** @brief Device hardware ID (if any) */
        std::string     hardwareId;

        /** @brief Device serial number (if any) */
        std::string     serialNumber;

        /** @brief Device type (if any) */
        std::string     type;

        /** @brief Extra data provided by the platform (if any) */
        std::string     extra;

        NetworkDeviceDescriptor()
        {
            clear();
        }

        void clear()
        {
            deviceId = 0;

            name.clear();
            manufacturer.clear();
            model.clear();
            hardwareId.clear();
            serialNumber.clear();
            type.clear();
            extra.clear();
        }

        virtual std::string toString()
        {
            char buff[2048];

            snprintf(buff, sizeof(buff), "deviceId=%d, name=%s, manufacturer=%s, model=%s, hardwareId=%s, serialNumber=%s, type=%s, extra=%s",
                        deviceId,
                        name.c_str(),
                        manufacturer.c_str(),
                        model.c_str(),
                        hardwareId.c_str(),
                        serialNumber.c_str(),
                        type.c_str(),
                        extra.c_str());

            return std::string(buff);
        }
    };

    static void to_json(nlohmann::json& j, const NetworkDeviceDescriptor& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(deviceId),
            TOJSON_IMPL(name),
            TOJSON_IMPL(manufacturer),
            TOJSON_IMPL(model),
            TOJSON_IMPL(hardwareId),
            TOJSON_IMPL(serialNumber),
            TOJSON_IMPL(type),
            TOJSON_IMPL(extra)
        };
    }
    static void from_json(const nlohmann::json& j, NetworkDeviceDescriptor& p)
    {
        p.clear();
        getOptional<int>("deviceId", p.deviceId, j, 0);
        getOptional("name", p.name, j);
        getOptional("manufacturer", p.manufacturer, j);
        getOptional("model", p.model, j);
        getOptional("hardwareId", p.hardwareId, j);
        getOptional("serialNumber", p.serialNumber, j);
        getOptional("type", p.type, j);
        getOptional("extra", p.extra, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(TxAudio)
    /**
     * @brief Configuration for the audio transmit properties for a group
     *
     *  Helper C++ class to serialize and de-serialize TxAudio JSON  @see Group
     *
     *  This JSON is used as an object in the Group definition JSON.
     *
     *  <P>
     *  Example: @include[doc] examples/TxAudio.json
     *  </P>
     *
     */
    class TxAudio : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(TxAudio)

    public:
        /**
         * @brief Codec Types enum.
         *
         * More detailed TxCodec_t description.
         */
        typedef enum
        {
            /** @brief Externally implemented */
            ctExternal      = -1,
            
            /** @brief Unknown Codec type */
            ctUnknown       = 0,

            /* G.711 */
            /** @brief G711 U-Law 64 (kbit/s) <a href="https://en.wikipedia.org/wiki/G.711" target="_blank">See for more info</a> */
            ctG711ulaw      = 1,

            /** @brief G711 A-Law 64 (kbit/s) <a href="https://en.wikipedia.org/wiki/G.711" target="_blank">See for more info</a> */
            ctG711alaw      = 2,


            /* GSM */
            /** @brief GSM Full Rate 13.2 (kbit/s) <a href="https://en.wikipedia.org/wiki/Full_Rate" target="_blank">See for more info</a> */
            ctGsm610        = 3,


            /* G.729 */
            /** @brief G.729a 8 (kbit/s) <a href="https://en.wikipedia.org/wiki/G.729" target="_blank">See for more info</a> */
            ctG729a         = 4,


            /* PCM */
            /** @brief PCM */
            ctPcm           = 5,

            // AMR Narrowband */
            /** @brief AMR Narrowband 4.75 (kbit/s) <a href="https://en.wikipedia.org/wiki/Adaptive_Multi-Rate_audio_codec" target="_blank">See for more info</a> */
            ctAmrNb4750     = 10,

            /** @brief AMR Narrowband 5.15 (kbit/s) <a href="https://en.wikipedia.org/wiki/Adaptive_Multi-Rate_audio_codec" target="_blank">See for more info</a> */
            ctAmrNb5150     = 11,

            /** @brief AMR Narrowband 5.9 (kbit/s) <a href="https://en.wikipedia.org/wiki/Adaptive_Multi-Rate_audio_codec" target="_blank">See for more info</a> */
            ctAmrNb5900     = 12,

            /** @brief AMR Narrowband 6.7 (kbit/s) <a href="https://en.wikipedia.org/wiki/Adaptive_Multi-Rate_audio_codec" target="_blank">See for more info</a> */
            ctAmrNb6700     = 13,

            /** @brief AMR Narrowband 7.4 (kbit/s) <a href="https://en.wikipedia.org/wiki/Adaptive_Multi-Rate_audio_codec" target="_blank">See for more info</a> */
            ctAmrNb7400     = 14,

            /** @brief AMR Narrowband 7.95 (kbit/s) <a href="https://en.wikipedia.org/wiki/Adaptive_Multi-Rate_audio_codec" target="_blank">See for more info</a> */
            ctAmrNb7950     = 15,

            /** @brief AMR Narrowband 10.2 (kbit/s) <a href="https://en.wikipedia.org/wiki/Adaptive_Multi-Rate_audio_codec" target="_blank">See for more info</a> */
            ctAmrNb10200    = 16,

            /** @brief AMR Narrowband 12.2 (kbit/s) <a href="https://en.wikipedia.org/wiki/Adaptive_Multi-Rate_audio_codec" target="_blank">See for more info</a> */
            ctAmrNb12200    = 17,


            /* Opus */
            /** @brief Opus 6 (kbit/s) <a href="http://opus-codec.org" target="_blank">See for more info</a> */
            ctOpus6000      = 20,

            /** @brief Opus 8 (kbit/s) <a href="http://opus-codec.org" target="_blank">See for more info</a> */
            ctOpus8000      = 21,

            /** @brief Opus 10 (kbit/s) <a href="http://opus-codec.org" target="_blank">See for more info</a> */
            ctOpus10000     = 22,

            /** @brief Opus 12 (kbit/s) <a href="http://opus-codec.org" target="_blank">See for more info</a> */
            ctOpus12000     = 23,

            /** @brief Opus 14 (kbit/s) <a href="http://opus-codec.org" target="_blank">See for more info</a> */
            ctOpus14000     = 24,

            /** @brief Opus 16 (kbit/s) <a href="http://opus-codec.org" target="_blank">See for more info</a> */
            ctOpus16000     = 25,

            /** @brief Opus 18 (kbit/s) <a href="http://opus-codec.org" target="_blank">See for more info</a> */
            ctOpus18000     = 26,

            /** @brief Opus 20 (kbit/s) <a href="http://opus-codec.org" target="_blank">See for more info</a> */
            ctOpus20000     = 27,

            /** @brief Opus 22 (kbit/s) <a href="http://opus-codec.org" target="_blank">See for more info</a> */
            ctOpus22000     = 28,

            /** @brief Opus 24 (kbit/s) <a href="http://opus-codec.org" target="_blank">See for more info</a> */
            ctOpus24000     = 29,


            /* Speex */
            /** @brief Speex 2.15 (kbit/s) <a href="http://https://www.speex.org/" target="_blank">See for more info</a> */
            ctSpxNb2150     = 30,

            /** @brief Speex 3.95 (kbit/s) <a href="http://https://www.speex.org/" target="_blank">See for more info</a> */
            ctSpxNb3950     = 31,

            /** @brief Speex 5.95 (kbit/s) <a href="http://https://www.speex.org/" target="_blank">See for more info</a> */
            ctSpxNb5950     = 32,

            /** @brief Speex 8 (kbit/s) <a href="http://https://www.speex.org/" target="_blank">See for more info</a> */
            ctSpxNb8000     = 33,

            /** @brief Speex 11 (kbit/s) <a href="http://https://www.speex.org/" target="_blank">See for more info</a> */
            ctSpxNb11000    = 34,

            /** @brief Speex 15 (kbit/s) <a href="http://https://www.speex.org/" target="_blank">See for more info</a> */
            ctSpxNb15000    = 35,

            /** @brief Speex 18.2 (kbit/s) <a href="http://https://www.speex.org/" target="_blank">See for more info</a> */
            ctSpxNb18200    = 36,

            /** @brief Speex 24.6 (kbit/s) <a href="http://https://www.speex.org/" target="_blank">See for more info</a> */
            ctSpxNb24600    = 37,


            /* Codec2 */
            /** @brief Codec2 0.45 (kbit/s) <a href="https://www.rowetel.com/?page_id=452" target="_blank">See for more info</a> */
            ctC2450         = 40,

            /** @brief Codec2 0.70 (kbit/s) <a href="https://www.rowetel.com/?page_id=452" target="_blank">See for more info</a> */
            ctC2700         = 41,

            /** @brief Codec2 1.2 (kbit/s) <a href="https://www.rowetel.com/?page_id=452" target="_blank">See for more info</a> */
            ctC21200        = 42,

            /** @brief Codec2 1.3 (kbit/s) <a href="https://www.rowetel.com/?page_id=452" target="_blank">See for more info</a> */
            ctC21300        = 43,

            /** @brief Codec2 1.4 (kbit/s) <a href="https://www.rowetel.com/?page_id=452" target="_blank">See for more info</a> */
            ctC21400        = 44,

            /** @brief Codec2 1.6 (kbit/s) <a href="https://www.rowetel.com/?page_id=452" target="_blank">See for more info</a> */
            ctC21600        = 45,

            /** @brief Codec2 2.4 (kbit/s) <a href="https://www.rowetel.com/?page_id=452" target="_blank">See for more info</a> */
            ctC22400        = 46,

            /** @brief Codec2 3.2 (kbit/s) <a href="https://www.rowetel.com/?page_id=452" target="_blank">See for more info</a> */
            ctC23200        = 47,


            /* MELPe */
            /** @brief MELPe 0.60 (kbit/s) <a href="https://en.wikipedia.org/wiki/Mixed-excitation_linear_prediction" target="_blank">See for more info</a> */
            ctMelpe600      = 50,

            /** @brief MELPe 1.2 (kbit/s) <a href="https://en.wikipedia.org/wiki/Mixed-excitation_linear_prediction" target="_blank">See for more info</a> */
            ctMelpe1200     = 51,

            /** @brief MELPe 2.4 (kbit/s) <a href="https://en.wikipedia.org/wiki/Mixed-excitation_linear_prediction" target="_blank">See for more info</a> */
            ctMelpe2400     = 52
        } TxCodec_t;

        /** @brief [Optional, Default: @ref ctOpus8000] Specifies the Codec Type to use for the transmission. See @ref TxCodec_t for all codec types */
        TxCodec_t       encoder;

        /** @brief [Optional] The name of the external codec - overrides encoder */
        std::string     encoderName;

        /** @brief [Optional, Default: 60] Audio sample framing size in milliseconds. */
        int             framingMs;

        /** @brief [Optional, Default: 0] If >0, derives framingMs based on the encoder's internal operation */
        int             blockCount;
        
        /** @brief [Optional, Default: false] Indicates if full duplex audio is supported. */
        bool            fdx;

        /**
         * @brief [Optional, Default: false] Set to <b>true</b> whether to disable header extensions.
         *
         * Indicates whether the Engine should not add a header extension on the RTP packet. If the header extension is omitted then any of the information such as Alias, NodeID and Tx
         * Priority will <B>not</B> be transmitted along with the audio.
         */
        bool            noHdrExt;

        /**
         * @brief [Optional, Default: 0] Maximum number of seconds the Engine will transmit for.
         *
         * When the time limit is exceeded, the Engine will fire a PFN_ENGAGE_GROUP_MAX_TX_TIME_EXCEEDED event.
         */
        int             maxTxSecs;

        /**
         * @brief [Optional, Default: 10] The number of packets when to periodically send the header extension.
         *
         * Eg, if its 3, then every third packed will contain the header extension.
         */
        int             extensionSendInterval;

        /** @brief [Optional, Default: 5] Number of headers to send at the beginning of a talk burst. */
        int             initialHeaderBurst;

        /** @brief [Optional, Default: 5] Number of headers to send at the conclusion of a talk burst. */
        int             trailingHeaderBurst;

        /** @brief [Optional, Default: -1] The custom RTP payload type to use for transmission. A value of -1 causes the default
         * Engage-defined RTP payload type to be used for the selected CODEC.
        */
        int             customRtpPayloadType;

        /** @brief [INTERNAL] The Engine-assigned key for the codec */
        uint32_t        internalKey;

        /** @brief [Optional, Default: true] Resets RTP counters on each new transmission. */
        bool            resetRtpOnTx;

        /** @brief [Optional, Default: true] Smooth input audio */
        bool                enableSmoothing;

        /** @brief [Optional, Default: false] Support discontinuous transmission on those CODECs that allow it */
        bool                dtx;

        /** @brief [Optional, Default: 0] Hang timer for ongoing TX - only applicable if enableSmoothing is true */
        int                 smoothedHangTimeMs;

        TxAudio()
        {
            clear();
        }

        void clear()
        {
            encoder = TxAudio::TxCodec_t::ctUnknown;
            encoderName.clear();
            framingMs = 60;
            blockCount = 0;
            fdx = false;
            noHdrExt = false;
            maxTxSecs = 0;
            extensionSendInterval = 10;
            initialHeaderBurst = 5;
            trailingHeaderBurst = 5;
            customRtpPayloadType = -1;
            internalKey = 0;
            resetRtpOnTx = true;
            enableSmoothing = true;
            dtx = false;
            smoothedHangTimeMs = 0;            
        }
    };

    static void to_json(nlohmann::json& j, const TxAudio& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(encoder),
            TOJSON_IMPL(encoderName),
            TOJSON_IMPL(framingMs),
            TOJSON_IMPL(blockCount),
            TOJSON_IMPL(fdx),
            TOJSON_IMPL(noHdrExt),
            TOJSON_IMPL(maxTxSecs),
            TOJSON_IMPL(extensionSendInterval),
            TOJSON_IMPL(initialHeaderBurst),
            TOJSON_IMPL(trailingHeaderBurst),
            TOJSON_IMPL(customRtpPayloadType),
            TOJSON_IMPL(resetRtpOnTx),
            TOJSON_IMPL(enableSmoothing),
            TOJSON_IMPL(dtx),
            TOJSON_IMPL(smoothedHangTimeMs)            
        };

        // internalKey is not serialized
    }
    static void from_json(const nlohmann::json& j, TxAudio& p)
    {
        p.clear();
        getOptional<TxAudio::TxCodec_t>("encoder", p.encoder, j, TxAudio::TxCodec_t::ctOpus8000);
        getOptional<std::string>("encoderName", p.encoderName, j, EMPTY_STRING);
        getOptional("framingMs", p.framingMs, j, 60);
        getOptional("blockCount", p.blockCount, j, 0);
        getOptional("fdx", p.fdx, j, false);
        getOptional("noHdrExt", p.noHdrExt, j, false);
        getOptional("maxTxSecs", p.maxTxSecs, j, 0);
        getOptional("extensionSendInterval", p.extensionSendInterval, j, 10);
        getOptional("initialHeaderBurst", p.initialHeaderBurst, j, 5);
        getOptional("trailingHeaderBurst", p.trailingHeaderBurst, j, 5);
        getOptional("customRtpPayloadType", p.customRtpPayloadType, j, -1);
        getOptional("resetRtpOnTx", p.resetRtpOnTx, j, true);
        getOptional("enableSmoothing", p.enableSmoothing, j, true);
        getOptional("dtx", p.dtx, j, false);
        getOptional("smoothedHangTimeMs", p.smoothedHangTimeMs, j, 0);

        // internalKey is not serialized   
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(AudioDeviceDescriptor)
    /**
     * @brief Custom Audio Device Configuration
     *
     * Helper C++ class to serialize and de-serialize AudioDeviceDescriptor JSON used in @ref engageAudioDeviceRegister API.
     *
     * Example: @include[doc] examples/AudioDeviceDescriptor.json
     *
     * @see engageAudioDeviceRegister
     */
    class AudioDeviceDescriptor : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(AudioDeviceDescriptor)

    public:

        /** @brief Audio Device Direction Enum. */
        typedef enum
        {
            /** @brief Direction unknown */
            dirUnknown = 0,

            /** @brief This is an input only device */
            dirInput,

            /** @brief This is an output only device */
            dirOutput,

            /** @brief This device supports both input and output */
            dirBoth
        } Direction_t;

        /**
         * @brief [Read Only] Unique device identifier assigned by Engage Engine at time of device creation.
         *
         */
        int             deviceId;

        /**
         * @brief This is the rate that the device will process the PCM audio data at.
         * 
         * Valid values are 8000 and 16000.
         *
         */
        int             samplingRate;

        /**
         * @brief Indicates the number of audio channels to process.
         * 
         * Valid values are 1 (mono) and 2 (stereo).
         * 
         */
        int             channels;

        /** @brief Audio direction the device supports @see Direction_t */
        Direction_t     direction;

        /**
         * @brief A percentage at which to gain/attenuate the audio.
         * 
         * Values above 100% will gain the level, values below will attenuate.
         * 
         */
        int             boostPercentage;

        /** @brief True if the device is an Application-Defined Audio Device */
        bool            isAdad;

        /** @brief Name of the device assigned by the platform */
        std::string     name;

        /** @brief Device manufacturer (if any) */
        std::string     manufacturer;

        /** @brief Device mode (if any) */
        std::string     model;

        /** @brief Device hardware ID (if any) */
        std::string     hardwareId;

        /** @brief Device serial number (if any) */
        std::string     serialNumber;

        /** @brief True if this is the default device for the direction above */
        bool            isDefault;

        /** @brief Device type (if any) */
        std::string     type;

        /** @brief Extra data provided by the platform (if any) */
        std::string     extra;

        /** @brief True if the device is currently present on the system */
        bool            isPresent;

        AudioDeviceDescriptor()
        {
            clear();
        }

        void clear()
        {
            deviceId = 0;
            samplingRate = 0;
            channels = 0;
            direction = dirUnknown;
            boostPercentage = 0;
            isAdad = false;
            isDefault = false;

            name.clear();
            manufacturer.clear();
            model.clear();
            hardwareId.clear();
            serialNumber.clear();
            type.clear();
            extra.clear();
            isPresent = false;
        }

        virtual std::string toString()
        {
            char buff[2048];

            snprintf(buff, sizeof(buff), "deviceId=%d, samplingRate=%d, channels=%d, direction=%d, boostPercentage=%d, isAdad=%d, name=%s, manufacturer=%s, model=%s, hardwareId=%s, serialNumber=%s, isDefault=%d, type=%s, present=%d, extra=%s",
                        deviceId,
                        samplingRate,
                        channels,
                        (int)direction,
                        boostPercentage,
                        (int)isAdad,
                        name.c_str(),
                        manufacturer.c_str(),
                        model.c_str(),
                        hardwareId.c_str(),
                        serialNumber.c_str(),
                        (int)isDefault,
                        type.c_str(),
                        (int)isPresent,
                        extra.c_str());

            return std::string(buff);
        }
    };

    static void to_json(nlohmann::json& j, const AudioDeviceDescriptor& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(deviceId),
            TOJSON_IMPL(samplingRate),
            TOJSON_IMPL(channels),
            TOJSON_IMPL(direction),
            TOJSON_IMPL(boostPercentage),
            TOJSON_IMPL(isAdad),
            TOJSON_IMPL(name),
            TOJSON_IMPL(manufacturer),
            TOJSON_IMPL(model),
            TOJSON_IMPL(hardwareId),
            TOJSON_IMPL(serialNumber),
            TOJSON_IMPL(isDefault),
            TOJSON_IMPL(type),
            TOJSON_IMPL(extra),
            TOJSON_IMPL(isPresent)
        };
    }
    static void from_json(const nlohmann::json& j, AudioDeviceDescriptor& p)
    {
        p.clear();
        getOptional<int>("deviceId", p.deviceId, j, 0);
        getOptional<int>("samplingRate", p.samplingRate, j, 0);
        getOptional<int>("channels", p.channels, j, 0);
        getOptional<AudioDeviceDescriptor::Direction_t>("direction", p.direction, j,
                        AudioDeviceDescriptor::Direction_t::dirUnknown);
        getOptional<int>("boostPercentage", p.boostPercentage, j, 0);

        getOptional<bool>("isAdad", p.isAdad, j, false);
        getOptional("name", p.name, j);
        getOptional("manufacturer", p.manufacturer, j);
        getOptional("model", p.model, j);
        getOptional("hardwareId", p.hardwareId, j);
        getOptional("serialNumber", p.serialNumber, j);
        getOptional("isDefault", p.isDefault, j);
        getOptional("type", p.type, j);
        getOptional("extra", p.extra, j);
        getOptional<bool>("isPresent", p.isPresent, j, false);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(ListOfAudioDeviceDescriptor)
    class ListOfAudioDeviceDescriptor : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(ListOfAudioDeviceDescriptor)

    public:
        std::vector<AudioDeviceDescriptor> list;

        ListOfAudioDeviceDescriptor()
        {
            clear();
        }

        void clear()
        {
            list.clear();
        }
    };

    static void to_json(nlohmann::json& j, const ListOfAudioDeviceDescriptor& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(list)
        };
    }
    static void from_json(const nlohmann::json& j, ListOfAudioDeviceDescriptor& p)
    {
        p.clear();
        getOptional<std::vector<AudioDeviceDescriptor>>("list", p.list, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(Audio)
    /**
     * @brief Used to configure the Audio properties for a group @see Group
     *
     * Helper C++ class to serialize and de-serialize Audio JSON
     *
     * Example: @include[doc] examples/Audio.json
     */
    class Audio : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(Audio)

    public:
        /** @brief [Optional, Default: true] Audio is enabled */
        bool    enabled;

        /** @brief [Optional, Default: first audio device] Id for the input audio device to use for this group. */
        int     inputId;

        /** @brief [Optional, Default: 0] The percentage at which to gain the input audio. */
        int     inputGain;

        /** @brief [Optional, Default: first audio device] Id for the output audio device to use for this group. */
        int     outputId;

        /** @brief [Optional, Default: 0] The percentage at which to gain the output audio. */
        int     outputGain;

        /** @brief [Optional, Default: 100] The percentage at which to set the <b>left</b> audio at.  */
        int     outputLevelLeft;

        /** @brief [Optional, Default: 100] The percentage at which to set the <b>right</b> audio at.  */
        int     outputLevelRight;

        /** @brief [Optional, Default: false] Mutes output audio.  */
        bool    outputMuted;

        Audio()
        {
            clear();
        }

        void clear()
        {
            enabled = true;
            inputId = 0;
            inputGain = 0;
            outputId = 0;
            outputGain = 0;
            outputLevelLeft = 100;
            outputLevelRight = 100;
            outputMuted = false;
        }
    };

    static void to_json(nlohmann::json& j, const Audio& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(inputId),
            TOJSON_IMPL(inputGain),
            TOJSON_IMPL(outputId),
            TOJSON_IMPL(outputLevelLeft),
            TOJSON_IMPL(outputLevelRight),
            TOJSON_IMPL(outputMuted)
        };
    }
    static void from_json(const nlohmann::json& j, Audio& p)
    {
        p.clear();
        getOptional<bool>("enabled", p.enabled, j, true);
        getOptional<int>("inputId", p.inputId, j, 0);
        getOptional<int>("inputGain", p.inputGain, j, 0);
        getOptional<int>("outputId", p.outputId, j, 0);
        getOptional<int>("outputGain", p.outputGain, j, 0);
        getOptional<int>("outputLevelLeft", p.outputLevelLeft, j, 100);
        getOptional<int>("outputLevelRight", p.outputLevelRight, j, 100);
        getOptional<bool>("outputMuted", p.outputMuted, j, false);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(TalkerInformation)
    /**
     * @brief Contains talker information used in providing a list in GroupTalkers
     *
     * Helper C++ class to serialize and de-serialize TalkerInformation JSON
     *
     * Example: @include[doc] examples/TalkerInformation.json
     *
     * @see GroupTalkers
     */
    class TalkerInformation : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(TalkerInformation)

    public:

        /** @brief The user alias to represent as a "talker". */
        std::string alias;

        /** @brief The nodeId the talker is originating from. */
        std::string nodeId;

        /** @brief Flags associated with a talker's transmission. */
        uint16_t    rxFlags;

        /** @brief Priority associated with a talker's transmission. */
        int         txPriority;

        /** @brief Transmission ID associated with a talker's transmission. */
        uint32_t    txId;

        /** @brief Number of duplicates detected. */
        int         duplicateCount;

        /** @brief The numeric specializer (if any) associated with the alias. */
        uint16_t    aliasSpecializer;

        /** @brief Indicates if RX is muted for this talker. */
        bool        rxMuted;

        TalkerInformation()
        {
            clear();
        }

        void clear()
        {
            alias.clear();
            nodeId.clear();
            rxFlags = 0;
            txPriority = 0;
            txId = 0;
            duplicateCount = 0;
            aliasSpecializer = 0;
            rxMuted = false;
        }
    };

    static void to_json(nlohmann::json& j, const TalkerInformation& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(alias),
            TOJSON_IMPL(nodeId),
            TOJSON_IMPL(rxFlags),
            TOJSON_IMPL(txPriority),
            TOJSON_IMPL(txId),
            TOJSON_IMPL(duplicateCount),
            TOJSON_IMPL(aliasSpecializer),
            TOJSON_IMPL(rxMuted)
        };
    }
    static void from_json(const nlohmann::json& j, TalkerInformation& p)
    {
        p.clear();
        getOptional<std::string>("alias", p.alias, j, EMPTY_STRING);
        getOptional<std::string>("nodeId", p.nodeId, j, EMPTY_STRING);
        getOptional<uint16_t>("rxFlags", p.rxFlags, j, 0);
        getOptional<int>("txPriority", p.txPriority, j, 0);
        getOptional<uint32_t>("txId", p.txId, j, 0);
        getOptional<int>("duplicateCount", p.duplicateCount, j, 0);
        getOptional<uint16_t>("aliasSpecializer", p.aliasSpecializer, j, 0);
        getOptional<bool>("rxMuted", p.rxMuted, j, false);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(GroupTalkers)
    /**
     * @brief List of @ref TalkerInformation objects.
     *
     * Helper C++ class to serialize and de-serialize GroupTalkers JSON
     *
     * This is used as the groupTalkerJson parameter in the PFN_ENGAGE_GROUP_RX_SPEAKERS_CHANGED event to represent a list of active talkers on the group.
     *
     * Example: @include[doc] examples/GroupTalkers.json
     *
     * @see PFN_ENGAGE_GROUP_RX_SPEAKERS_CHANGED
     */
    class GroupTalkers : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(GroupTalkers)

    public:
        /** @brief List of @ref TalkerInformation objects.*/
        std::vector<TalkerInformation> list;

        GroupTalkers()
        {
            clear();
        }

        void clear()
        {
            list.clear();
        }
    };

    static void to_json(nlohmann::json& j, const GroupTalkers& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(list)
        };
    }
    static void from_json(const nlohmann::json& j, GroupTalkers& p)
    {
        p.clear();
        getOptional<std::vector<TalkerInformation>>("list", p.list, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(Presence)
    /**
     * @brief Describes how the Presence is configured for a group of type @ref Group::gtPresence in @ref Group::Type_t
     *
     * Helper C++ class to serialize and de-serialize Presence JSON
     *
     * Example: @include[doc] examples/Presence.json
     *
     * @see Group
     */
    class Presence : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(Presence)

    public:
        /**
         * @brief Presence format types enum.
         */
        typedef enum
        {
            /** @brief Unknown */
            pfUnknown       = 0,

            /** @brief Engage propriety format */
            pfEngage        = 1,

            /**
             * @brief Cursor On Target format.
             *
             * This is a simple XML based exchange standard that is used to share information about targets.
             * Cursor on Target was originally developed by MITRE in 2002 in support of the U.S. Air Force Electronic Systems Center (ESC)
             */
            pfCot           = 2
        } Format_t;

        /** @brief Format to be used to represent presence information. */
        Format_t        format;

        /** @brief [Optional, Default: 30] The interval in seconds at which to send the presence descriptor on the presence group */
        int             intervalSecs;

        /** @brief Instructs the Engage Engine to not transmit presence descriptor */
        bool            listenOnly;

        /** @brief [Optional, Default: 5] The minimum interval to send at to prevent network flooding */
        int             minIntervalSecs;

        Presence()
        {
            clear();
        }

        void clear()
        {
            format = pfUnknown;
            intervalSecs = 30;
            listenOnly = false;
            minIntervalSecs = 5;
        }
    };

    static void to_json(nlohmann::json& j, const Presence& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(format),
            TOJSON_IMPL(intervalSecs),
            TOJSON_IMPL(listenOnly),
            TOJSON_IMPL(minIntervalSecs)
        };
    }
    static void from_json(const nlohmann::json& j, Presence& p)
    {
        p.clear();
        getOptional<Presence::Format_t>("format", p.format, j, Presence::Format_t::pfEngage);
        getOptional<int>("intervalSecs", p.intervalSecs, j, 30);
        getOptional<bool>("listenOnly", p.listenOnly, j, false);
        getOptional<int>("minIntervalSecs", p.minIntervalSecs, j, 5);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(Advertising)
    /**
     * @brief Defines parameters for advertising of an entity such as a known, public, group.
     *
     * Helper C++ class to serialize and de-serialize Advertising JSON
     *
     * Example: @include[doc] examples/Advertising.json
     *
     * @see DiscoverySsdp, DiscoverySap
     */
    class Advertising : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(Advertising)

    public:
        /** @brief [Optional, Default: false] Enabled advertising */
        bool        enabled;

        /** @brief [Optional, Default: 20000] Interval at which the advertisement should be sent in milliseconds. */
        int         intervalMs;

        /** @brief [Optional, Default: false] If true, the node will advertise the item even if it detects other nodes making the same advertisement */
        bool        alwaysAdvertise;

        Advertising()
        {
            clear();
        }

        void clear()
        {
            enabled = false;
            intervalMs = 20000;
            alwaysAdvertise = false;
        }
    };

    static void to_json(nlohmann::json& j, const Advertising& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(intervalMs),
            TOJSON_IMPL(alwaysAdvertise)
        };
    }
    static void from_json(const nlohmann::json& j, Advertising& p)
    {
        p.clear();
        getOptional("enabled", p.enabled, j, false);
        getOptional<int>("intervalMs", p.intervalMs, j, 20000);
        getOptional<bool>("alwaysAdvertise", p.alwaysAdvertise, j, false);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(GroupPriorityTranslation)
    /**
     * @brief Details for priority transmission based on unique network addressing
     *
     * Helper C++ class to serialize and de-serialize GroupPriorityTranslation JSON
     *
     * Example: @include[doc] examples/GroupPriorityTranslation.json
     *
     * @see Group
     */
    class GroupPriorityTranslation : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(GroupPriorityTranslation)

    public:
        /** @brief RX addressing */
        NetworkAddress          rx;

        /** @brief TX addressing */
        NetworkAddress          tx;

        /** @brief Engage priority for RX & TX */
        int                     priority;

        GroupPriorityTranslation()
        {
            clear();
        }

        void clear()
        {
            rx.clear();
            tx.clear();
            priority = 0;
        }
    };

    static void to_json(nlohmann::json& j, const GroupPriorityTranslation& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(rx),
            TOJSON_IMPL(tx),
            TOJSON_IMPL(priority)
        };
    }
    static void from_json(const nlohmann::json& j, GroupPriorityTranslation& p)
    {
        p.clear();
        j.at("rx").get_to(p.rx);
        j.at("tx").get_to(p.tx);
        FROMJSON_IMPL(priority, int, 0);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(GroupTimeline)
    /**
     * @brief Configuration for Timeline functionality for Group.
     *
     * If a GroupTimeline is not specified, then the configursation in the @ref ConfigurationObjects::EnginePolicyTimelines will used as default.
     *
     * Helper C++ class to serialize and de-serialize GroupTimeline JSON
     *
     * Example: @include[doc] examples/GroupTimeline.json
     *
     * @see ConfigurationObjects::Group, ConfigurationObjects::EnginePolicyTimeLines
     */
    class GroupTimeline : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(GroupTimeline)

    public:
        /** @brief [Optional, Default: true] Enables timeline feature. */
        bool        enabled;

        /** @brief [Optional, Default: 30000] Maximum audio block size to record in milliseconds. */
        int         maxAudioTimeMs;
        bool        recordAudio;

        GroupTimeline()
        {
            clear();
        }

        void clear()
        {
            enabled = true;
            maxAudioTimeMs = 30000;
            recordAudio = true;
        }
    };

    static void to_json(nlohmann::json& j, const GroupTimeline& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(maxAudioTimeMs),
            TOJSON_IMPL(recordAudio)
        };
    }
    static void from_json(const nlohmann::json& j, GroupTimeline& p)
    {
        p.clear();
        getOptional("enabled", p.enabled, j, true);
        getOptional<int>("maxAudioTimeMs", p.maxAudioTimeMs, j, 30000);
        getOptional("recordAudio", p.recordAudio, j, true);
    }

    /** @addtogroup groupSources Group source names
     *
     * Names assigned to entities that advertise groups/channels.
     *
     *  @{
     */
    /** @brief Internal to Engage */
    ENGAGE_IGNORE_COMPILER_UNUSED_WARNING static const char *GROUP_SOURCE_ENGAGE_INTERNAL = "com.rallytac.engage.internal";
    /** @brief The source is a Magellan-capable entity */
    ENGAGE_IGNORE_COMPILER_UNUSED_WARNING static const char *GROUP_SOURCE_ENGAGE_MAGELLAN_CORE = "com.rallytac.magellan.core";
    /** @brief The source is CISTECH via Magellan discovery */
    ENGAGE_IGNORE_COMPILER_UNUSED_WARNING static const char *GROUP_SOURCE_ENGAGE_MAGELLAN_CISTECH = "com.rallytac.engage.magellan.cistech";
    /** @brief The source is Trellisware via Magellan discovery */
    ENGAGE_IGNORE_COMPILER_UNUSED_WARNING static const char *GROUP_SOURCE_ENGAGE_MAGELLAN_TRELLISWARE = "com.rallytac.engage.magellan.trellisware";
    /** @brief The source is Silvus via Magellan discovery */
    ENGAGE_IGNORE_COMPILER_UNUSED_WARNING static const char *GROUP_SOURCE_ENGAGE_MAGELLAN_SILVUS = "com.rallytac.engage.magellan.silvus";
    /** @brief The source is Persistent Systems via Magellan discovery */
    ENGAGE_IGNORE_COMPILER_UNUSED_WARNING static const char *GROUP_SOURCE_ENGAGE_MAGELLAN_PERSISTENT = "com.rallytac.engage.magellan.persistent";
    /** @brief The source is Domo Tactical via Magellan discovery */
    ENGAGE_IGNORE_COMPILER_UNUSED_WARNING static const char *GROUP_SOURCE_ENGAGE_MAGELLAN_DOMO = "com.rallytac.engage.magellan.domo";
    /** @brief The source is Kenwood via Magellan discovery */
    ENGAGE_IGNORE_COMPILER_UNUSED_WARNING static const char *GROUP_SOURCE_ENGAGE_MAGELLAN_KENWOOD = "com.rallytac.engage.magellan.kenwood";
    /** @brief The source is Tait via Magellan discovery */
    ENGAGE_IGNORE_COMPILER_UNUSED_WARNING static const char *GROUP_SOURCE_ENGAGE_MAGELLAN_TAIT = "com.rallytac.engage.magellan.tait";
    /** @brief The source is Vocality via Magellan discovery */
    ENGAGE_IGNORE_COMPILER_UNUSED_WARNING static const char *GROUP_SOURCE_ENGAGE_MAGELLAN_VOCALITY = "com.rallytac.engage.magellan.vocality";
    /** @} */

    /** @addtogroup groupDisconnectReasons Reasons for why a group has disconnected
     *
     * These are additional reason descriptors that may accompany a onGroupDisconnected event
     *
     *  @{
     */
    /** @brief No particular reason was provided */
    ENGAGE_IGNORE_COMPILER_UNUSED_WARNING static const char *GROUP_DISCONNECTED_REASON_NO_REAON = "NoReason";
    /** @brief The link to the Rallypoint is down */
    ENGAGE_IGNORE_COMPILER_UNUSED_WARNING static const char *GROUP_DISCONNECTED_REASON_NO_LINK = "NoLink";
    /** @brief The group has been gracefully unregistered from the Rallypoint **/
    ENGAGE_IGNORE_COMPILER_UNUSED_WARNING static const char *GROUP_DISCONNECTED_REASON_UNREGISTERED = "Unregistered";
    /** @brief The Rallypoint is not accepting registration for the group at this time **/
    ENGAGE_IGNORE_COMPILER_UNUSED_WARNING static const char *GROUP_DISCONNECTED_REASON_NOT_ALLOWED = "NotAllowed";
    /** @brief The Rallypoint has denied the registration because the registration is for a security level not allowed on the RP **/
    ENGAGE_IGNORE_COMPILER_UNUSED_WARNING static const char *GROUP_DISCONNECTED_REASON_SECURITY_CLASSIFICATION_LEVEL_TOO_HIGH = "SecurityClassificationLevelTooHigh";
    /** @brief The Rallypoint has denied the registration for no specific reason **/
    ENGAGE_IGNORE_COMPILER_UNUSED_WARNING static const char *GROUP_DISCONNECTED_REASON_GENERAL_DENIAL = "GeneralDenial";
    /** @} */


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(GroupSatPaq)
    /**
     * @brief Configuration for the optional SatPaq transport functionality for Group.
     *
     * Helper C++ class to serialize and de-serialize GroupSatPaq JSON
     *
     * Example: @include[doc] examples/GroupSatPaq.json
     *
     * @see ConfigurationObjects::Group
     */
    class GroupSatPaq : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(GroupSatPaq)

    public:
        /** @brief [Optional, Default: false] Enables SatPaq feature. */
        bool                        enabled;

        /** @brief The sender ID. */
        uint16_t                    senderId;

        /** @brief The talkgroup ID. */
        uint8_t                     talkgroupId;

        GroupSatPaq()
        {
            clear();
        }

        void clear()
        {
            enabled = false;
            senderId = 0;
            talkgroupId = 0;
        }
    };

    static void to_json(nlohmann::json& j, const GroupSatPaq& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(senderId),
            TOJSON_IMPL(talkgroupId)
        };
    }
    static void from_json(const nlohmann::json& j, GroupSatPaq& p)
    {
        p.clear();
        getOptional<bool>("enabled", p.enabled, j, false);
        getOptional<uint16_t>("senderId", p.senderId, j, 0);
        getOptional<uint8_t>("talkgroupId", p.talkgroupId, j, 0);
    }



    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(GroupLynQPro)
    /**
     * @brief Configuration for the optional LynQ Pro transport functionality for Group.
     *
     * Helper C++ class to serialize and de-serialize GroupLynQPro JSON
     *
     * Example: @include[doc] examples/GroupLynQPro.json
     *
     * @see ConfigurationObjects::Group
     */
    class GroupLynQPro : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(GroupLynQPro)

    public:
        /** @brief [Optional, Default: false] Enables SatPaq feature. */
        bool                        enabled;

        /** @brief The sender ID. */
        uint16_t                    senderId;

        /** @brief The talkgroup ID. */
        uint8_t                     talkgroupId;

        GroupLynQPro()
        {
            clear();
        }

        void clear()
        {
            enabled = false;
            senderId = 0;
            talkgroupId = 0;
        }
    };

    static void to_json(nlohmann::json& j, const GroupLynQPro& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(senderId),
            TOJSON_IMPL(talkgroupId)
        };
    }
    static void from_json(const nlohmann::json& j, GroupLynQPro& p)
    {
        p.clear();
        getOptional<bool>("enabled", p.enabled, j, false);
        getOptional<uint16_t>("senderId", p.senderId, j, 0);
        getOptional<uint8_t>("talkgroupId", p.talkgroupId, j, 0);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(RtpProfile)
    /**
     * @brief Configuration for the optional RtpProfile.
     *
     * Helper C++ class to serialize and de-serialize RtpProfile JSON
     *
     * Example: @include[doc] examples/RtpProfile.json
     *
     * @see ConfigurationObjects::Group
     */
    class RtpProfile : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(RtpProfile)

    public:
        /**
         * @brief Jitter buffer mode.
         *
         * Operation mode for the jitter buffer.
         */
        typedef enum
        {
            /** @brief Default */
            jmStandard        = 0,

            /** @brief Low latency */
            jmLowLatency      = 1,

            /** @brief The jitter buffer releases upon positive end of TX notification */
            jmReleaseOnTxEnd   = 2
        } JitterMode_t;

        /** @brief [Optional, Default: jmStandard] Specifies the operation mode (see @ref JitterMode_t). */
        JitterMode_t                        mode;

        /** @brief [Optional, Default: 10000] Maximum number of milliseconds allowed in the queue */
        int                                 jitterMaxMs;

        /** @brief [Optional, Default: 100] Low-water mark for jitter buffers that are in a buffering state. */
        int                                 jitterMinMs;

        /** @brief [Optional, Default: 8] The factor by which to multiply the jitter buffer's active low-water to determine the high-water mark */
        int                                 jitterMaxFactor;

        /** @brief [Optional, Default: 5] The delta in RTP sequence numbers in order to heuristically determine the start of a new talk spurt */
        int                                 latePacketSequenceRange;

        /** @brief [Optional, Default: 500] The delta in milliseconds in order to heuristically determine the start of a new talk spurt */
        int                                 latePacketTimestampRangeMs;

        /** @brief [Optional, Default: 10] The percentage of the overall jitter buffer sample count to trim when potential buffer overflow is encountered */
        int                                 jitterTrimPercentage;

        /** @brief [Optional, Default: 1500] Number of milliseconds of error-free operations in a jitter buffer before the underrun counter begins reducing */
        int                                 jitterUnderrunReductionThresholdMs;

        /** @brief [Optional, Default: 100] Number of jitter buffer operations after which to reduce any underrun */
        int                                 jitterUnderrunReductionAger;

        /** @brief [Optional, Default: 0] Forces trimming of the jitter buffer if the queue length is greater (and not zero) */
        int                                 jitterForceTrimAtMs;

        /** @brief [Optional, Default: 250] Maximum number of milliseconds to be trimmed from a jitter buffer at any one point */
        int                                 jitterMaxTrimMs;

        /** @brief [Optional, Default: 10] Percentage by which maximum number of samples in the queue exceeded computed max before large-scale clipping . */
        int                                 jitterMaxExceededClipPerc;

        /** @brief [Optional, Default: 1500] Number of milliseconds for which the jitter buffer may exceed max before clipping is actually applied. */
        int                                 jitterMaxExceededClipHangMs;

        /** @brief [Optional, Default: 500] The number of milliseconds of RTP inactivity before heuristically determining the end of talk spurt */
        int                                 inboundProcessorInactivityMs;

        /** @brief [Optional, Default: 45000] Timeout for RTCP presence. */
        int                                 rtcpPresenceTimeoutMs;

        /** @brief [Optional, Default: 15000] The number of milliseconds that a "zombified" RTP processor is kept around before being removed for good */
        int                                 zombieLifetimeMs;

        /** @brief [Optional, Default: inboundProcessorInactivityMs * 4] The number of milliseconds of RTP inactivity on an Engage-native/signalled (i.e. where 
         * end-of-tx is expected) stream before heuristically determining the end of talk spurt */
        int                                 signalledInboundProcessorInactivityMs;
        
        RtpProfile()
        {
            clear();
        }

        void clear()
        {
            mode = jmStandard;
            jitterMaxMs = 10000;
            jitterMinMs = 100;
            jitterMaxFactor = 8;
            jitterTrimPercentage = 10;
            jitterUnderrunReductionThresholdMs = 1500;
            jitterUnderrunReductionAger = 100;
            latePacketSequenceRange = 5;
            latePacketTimestampRangeMs = 2000;
            inboundProcessorInactivityMs = 500;
            jitterForceTrimAtMs = 0;
            rtcpPresenceTimeoutMs = 45000;
            jitterMaxExceededClipPerc = 10;
            jitterMaxExceededClipHangMs = 1500;
            zombieLifetimeMs = 15000;
            jitterMaxTrimMs = 250;
            signalledInboundProcessorInactivityMs = (inboundProcessorInactivityMs * 4);
        }
    };

    static void to_json(nlohmann::json& j, const RtpProfile& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(mode),
            TOJSON_IMPL(jitterMaxMs),
            TOJSON_IMPL(inboundProcessorInactivityMs),
            TOJSON_IMPL(jitterMinMs),
            TOJSON_IMPL(jitterMaxFactor),
            TOJSON_IMPL(jitterTrimPercentage),
            TOJSON_IMPL(jitterUnderrunReductionThresholdMs),
            TOJSON_IMPL(jitterUnderrunReductionAger),
            TOJSON_IMPL(latePacketSequenceRange),
            TOJSON_IMPL(latePacketTimestampRangeMs),
            TOJSON_IMPL(inboundProcessorInactivityMs),
            TOJSON_IMPL(jitterForceTrimAtMs),
            TOJSON_IMPL(jitterMaxExceededClipPerc),
            TOJSON_IMPL(jitterMaxExceededClipHangMs),
            TOJSON_IMPL(zombieLifetimeMs),
            TOJSON_IMPL(jitterMaxTrimMs),
            TOJSON_IMPL(signalledInboundProcessorInactivityMs)
        };
    }
    static void from_json(const nlohmann::json& j, RtpProfile& p)
    {
        p.clear();
        FROMJSON_IMPL(mode, RtpProfile::JitterMode_t, RtpProfile::JitterMode_t::jmStandard);
        FROMJSON_IMPL(jitterMaxMs, int, 10000);
        FROMJSON_IMPL(jitterMinMs, int, 20);
        FROMJSON_IMPL(jitterMaxFactor, int, 8);
        FROMJSON_IMPL(jitterTrimPercentage, int, 10);
        FROMJSON_IMPL(jitterUnderrunReductionThresholdMs, int, 1500);
        FROMJSON_IMPL(jitterUnderrunReductionAger, int, 100);        
        FROMJSON_IMPL(latePacketSequenceRange, int, 5);
        FROMJSON_IMPL(latePacketTimestampRangeMs, int, 2000);
        FROMJSON_IMPL(inboundProcessorInactivityMs, int, 500);
        FROMJSON_IMPL(jitterForceTrimAtMs, int, 0);        
        FROMJSON_IMPL(rtcpPresenceTimeoutMs, int, 45000);
        FROMJSON_IMPL(jitterMaxExceededClipPerc, int, 10);
        FROMJSON_IMPL(jitterMaxExceededClipHangMs, int, 1500);
        FROMJSON_IMPL(zombieLifetimeMs, int, 15000);
        FROMJSON_IMPL(jitterMaxTrimMs, int, 250);
        FROMJSON_IMPL(signalledInboundProcessorInactivityMs, int, (p.inboundProcessorInactivityMs * 4));
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(Tls)
    /**
    * @brief TODO: Transport Security Layer (TLS) settings
    *
    * Helper C++ class to serialize and de-serialize Tls JSON
    *
    * Example: @include[doc] examples/Tls.json
    *
    * @see RallypointServer
    */
    class Tls : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(Tls)

    public:

        /** @brief [Optional, Default: true] When true, checks the far-end certificate validity and Engage-specific TLS setup procedure. */
        bool                        verifyPeers;

        /** @brief [Optional, Default: false] When true, accepts far-end certificates that are self-signed. */
        bool                        allowSelfSignedCertificates;

        /** @brief [Optional] Array of CA certificates (PEM or "@" file/certstore references) to be used to validate far-end certificates. */
        std::vector<std::string>    caCertificates;

        /** @brief [NOT USED AT THIS TIME] */
        StringRestrictionList       subjectRestrictions;

        /** @brief [NOT USED AT THIS TIME] */
        StringRestrictionList       issuerRestrictions;

        Tls()
        {
            clear();
        }

        void clear()
        {
            verifyPeers = true;
            allowSelfSignedCertificates = false;
            caCertificates.clear();
            subjectRestrictions.clear();
            issuerRestrictions.clear();
        }
    };

    static void to_json(nlohmann::json& j, const Tls& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(verifyPeers),
            TOJSON_IMPL(allowSelfSignedCertificates),
            TOJSON_IMPL(caCertificates),
            TOJSON_IMPL(subjectRestrictions),
            TOJSON_IMPL(issuerRestrictions)
        };
    }
    static void from_json(const nlohmann::json& j, Tls& p)
    {
        p.clear();
        getOptional<bool>("verifyPeers", p.verifyPeers, j, true);
        getOptional<bool>("allowSelfSignedCertificates", p.allowSelfSignedCertificates, j, false);
        getOptional<std::vector<std::string>>("caCertificates", p.caCertificates, j);
        getOptional<StringRestrictionList>("subjectRestrictions", p.subjectRestrictions, j);
        getOptional<StringRestrictionList>("issuerRestrictions", p.issuerRestrictions, j);
    }
    
    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(RangerPackets)
    /**
     * @brief Options for Ranger packets
     *
     *  Helper C++ class to serialize and de-serialize RangerPackets JSON
     *
     *  TODO: Complete this Class
     *
     *  Example: @include[doc] examples/RangerPackets.json
     *
     *  @see Group
     */
    class RangerPackets : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(RangerPackets)

    public:
        /** @brief [Optional, Default: -1] Number of seconds since last packet transmission before 'count' packets are sent */
        int             hangTimerSecs;

        /** @brief [Optional, Default: 5] Number of ranger packets to send when a new interval starts */
        int             count;

        RangerPackets()
        {
            clear();
        }

        void clear()
        {
            hangTimerSecs = -1;
            count = 5;
        }

        virtual void initForDocumenting()
        {
        }
    };

    static void to_json(nlohmann::json& j, const RangerPackets& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(hangTimerSecs),
            TOJSON_IMPL(count)
        };
    }
    static void from_json(const nlohmann::json& j, RangerPackets& p)
    {
        p.clear();
        getOptional<int>("hangTimerSecs", p.hangTimerSecs, j, 11);
        getOptional<int>("count", p.count, j, 5);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(Source)
    /**
     * @brief Options for Source
     *
     *  Helper C++ class to serialize and de-serialize Source JSON
     *
     *  TODO: Complete this Class
     *
     *  Example: @include[doc] examples/Source.json
     *
     *  @see Group
     */
    class Source : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(Source)

    public:
        /** @brief [Optional] A node ID */
        std::string             nodeId;

        /* NOTE: Not serialized ! */
        uint8_t                 _internal_binary_nodeId[ENGAGE_MAX_NODE_ID_SIZE];

        /** @brief [Optional] An alias */
        std::string             alias;
        
        /* NOTE: Not serialized ! */
        uint8_t                 _internal_binary_alias[ENGAGE_MAX_ALIAS_SIZE];

        Source()
        {
            clear();
        }

        void clear()
        {
            nodeId.clear();
            memset(_internal_binary_nodeId, 0, sizeof(_internal_binary_nodeId));
            
            alias.clear();
            memset(_internal_binary_alias, 0, sizeof(_internal_binary_alias));
        }

        virtual void initForDocumenting()
        {
        }
    };

    static void to_json(nlohmann::json& j, const Source& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(nodeId),
            TOJSON_IMPL(alias)
        };
    }
    static void from_json(const nlohmann::json& j, Source& p)
    {
        p.clear();
        FROMJSON_IMPL_SIMPLE(nodeId);
        FROMJSON_IMPL_SIMPLE(alias);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(Group)
    /**
     * @brief Group Configuration
     *
     * This describes all the group properties for all supported group types
     *
     * Example: @include[doc] examples/Group.json
     *
     * @see engageCreateGroup
     *
     */
    class Group : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(Group)

    public:
        /** @brief Enum describing the group types. */
        typedef enum
        {
            /** @brief Unknown group type */
            gtUnknown   = 0,

            /** @brief Audio group type. This group is used to transmit Audio. */
            gtAudio     = 1,

            /** @brief Presence group type. This group is use to relay presence data to all nodes that are configured for the same presence group. */
            gtPresence  = 2,

            /** @brief Raw group type. No special processing is performed on raw groups - the payload is siomply seen as a binary object to be typically handled by the application logic.  */
            gtRaw       = 3
        } Type_t;


        /** @brief Enum describing bridging operation mode types where applicable. */
        typedef enum
        {
            /** @brief Raw mode (default) - packet payloads are not accessed or modified and forwarded as raw packets */
            bomRaw                      = 0,

            /** @brief Audio payloads are transformed, headers are preserved, multiple parallel output streams are possible/expected */
            bomPayloadTransformation    = 1,

            /** @brief Audio payloads are mixed - output is anonymous (i.e. no metadata) if if the target group(s) allow header extensions */
            bomAnonymousMixing          = 2,

            /** @brief The bridge performs language translations between groups */
            bomLanguageTranslation      = 3
        } BridgingOpMode_t;

        /** @brief Specifies the group type (see @ref Type_t). */
        Type_t                                  type;

        /** @brief Specifies the bridging operation mode if applicable (see @ref BridgingOpMode_t). */
        BridgingOpMode_t                        bom;

        /**
         * @brief Unique identity for the group.
         *
         * NOTE: Groups configured with the same multicast addresses but with different
         * id's will NOT be routed correctly via RallyPoints as they are considered different streams.
         */
        std::string                             id;

        /** @brief The human readable name for the group. */
        std::string                             name;

        /** @brief The group name as spoken - typically by a text-to-speech system  */
        std::string                             spokenName;

        /** @brief The name of the network interface to use for multicasting for this group.  If not provided, the Engine's default multicast NIC is used. */
        std::string                             interfaceName;

        /** @brief The network address for receiving network traffic on. */
        NetworkAddress                          rx;

        /** @brief The network address for transmitting network traffic to. */
        NetworkAddress                          tx;

        /** @brief Transmit options for the group (see @ref NetworkTxOptions). */
        NetworkTxOptions                        txOptions;

        /** @brief Audio transmit options such as codec, framing size etc (see @ref TxAudio). */
        TxAudio                                 txAudio;

        /** @brief Presence configuration (see @ref Presence). */
        Presence                                presence;

        /** @brief Password to be used for encryption. Note that this is not the encryption key but, rather, the hexidecimal representation of the baseline material to be used in a PBFDK2 algorithm for key derivation. */
        std::string                             cryptoPassword;

        /** @brief [Optional, Default: false] Use low-bandwidth crypto */
        bool                                    lbCrypto;

        /** @brief [DEPRECATED] List of @ref Rallypoint (s) the Group should use to connect to a Rallypoint router. Use @ref RallypointCluster instead. */
        std::vector<Rallypoint>                 rallypoints;

        /** @brief Cluster of one or more @ref Rallypoints the group may use. */
        RallypointCluster                       rallypointCluster;

        /** @brief Sets audio properties like which audio device to use, audio gain etc (see @ref Audio). */
        Audio                                   audio;

        /**
         * @brief Audio timeline is configuration.
         *
         * Specifies how the Engine should record times lines. Timelines are used for instant replay and audio archival.
         *
         * @see engageQueryGroupTimeline, PFN_ENGAGE_GROUP_TIMELINE_EVENT_??? events
         */
        GroupTimeline                           timeline;

        /** @brief User alias to transmit as part of the realtime audio stream when using the @ref engageBeginGroupTx API.  */
        std::string                             alias;

         /** @brief [Optional, Default: false] Set this to true if you do not want the Engine to advertise this Group on the Presence group. */
        bool                                    blockAdvertising;

        /** @brief [Optional, Default: null] Indicates the source of this configuration - e.g. from the application or discovered via Magellan */
        std::string                             source;

        /**
         * @brief [Optional, Default: 0] Maximum number of seconds the Engine will receive for on this group.
         *
         * When the time limit is exceeded, the Engine will fire a PFN_ENGAGE_GROUP_MAX_RX_TIME_EXCEEDED event.
         */
        int                                     maxRxSecs;

        /** @brief [Optional, Default: false] Set this to true to enable failover to multicast operation if a Rallypoint connection cannot be established. */
        bool                                    enableMulticastFailover;

        /** @brief [Optional, Default: 10] Specifies the number fo seconds to wait after Rallypoint connection failure to switch to multicast operation. */
        int                                     multicastFailoverSecs;

        /** @brief [Optional, Default: false] Set this to true to have event notifications fire when human speech is detected in incoming audio streams. */
        bool                                    enableRxVad;

        /** @brief The network address for receiving RTCP presencing packets */
        NetworkAddress                          rtcpPresenceRx;

        /** @brief List of presence group IDs with which this group has an affinity */
        std::vector<std::string>                presenceGroupAffinities;

        /** @brief [Optional, Default: false] Disable packet events. */
        bool                                    disablePacketEvents;

        /** @brief [Optional, Default: 0] The RTP payload ID by which to identify (RX and TX) payloads encoded according to RFC 4733 (RFC 2833). */
        int                                     rfc4733RtpPayloadId;

        /** @brief [Optional] A vector of translations from external entity RTP payload types to those used by Engage */
        std::vector<RtpPayloadTypeTranslation>  inboundRtpPayloadTypeTranslations;

        /** @brief [Optional] Describe how traffic for this group on a different addressing scheme translates to priority for the group */
        GroupPriorityTranslation                priorityTranslation;

        /** @brief [Optional, Default: 10] The number of seconds after which "sticky" transmission IDs expire. */
        int                                     stickyTidHangSecs;

        /** @brief [Optional] Alias to use for inbound streams that do not have an alias component  */
        std::string                             anonymousAlias;

        /** @brief [Optional] Settings necessary if the group is transported via the SatPaq protocol  */
        GroupSatPaq                             satPaq;

        /** @brief [Optional] Settings necessary if the group is transported via the LynQPro protocol  */
        GroupLynQPro                            lynqPro;

        /** @brief [Optional] The name of the registered transport to use  */
        std::string                             transportName;

        /** @brief [Optional, Default: false] Allows for processing of looped back packets - primarily meant for debugging  */
        bool                                    allowLoopback;

        /** @brief [Optional] RTP profile the group  */
        RtpProfile                              rtpProfile;

        /** @brief [Optional] Ranger packet options  */
        RangerPackets                           rangerPackets;
        
        /** @brief [Internal - not serialized  */
        bool                                    _wasDeserialized_rtpProfile;

        /** @brief [Optional] The TX impairment to apply */
        TransportImpairment                     txImpairment;

        /** @brief [Optional] The RX impairment to apply */
        TransportImpairment                     rxImpairment;

        /** @brief List of specializer IDs that the local node has an affinity for/member of */
        std::vector<uint16_t>                   specializerAffinities;

        /** @brief [Optional, Default: 0] The security classification level of the group. */
        uint32_t                                securityLevel;

        /** @brief [Optional] List of sources to ignore for this group */
        std::vector<Source>                     ignoreSources;

        /** @brief ISO 639-2 language code for the group  */
        std::string                             languageCode;

        /** @brief Name of the synthesis voice to use for the group  */
        std::string                             synVoice;

        /** @brief Details for capture of received packets  */
        PacketCapturer                          rxCapture;

        /** @brief Details for capture of transmitted packets  */
        PacketCapturer                          txCapture;

        Group()
        {
            clear();
        }

        void clear()
        {
            type = gtUnknown;
            bom = bomRaw;
            id.clear();
            name.clear();
            spokenName.clear();
            interfaceName.clear();
            rx.clear();
            tx.clear();
            txOptions.clear();
            txAudio.clear();
            presence.clear();
            cryptoPassword.clear();

            alias.clear();

            rallypoints.clear();
            rallypointCluster.clear();

            audio.clear();
            timeline.clear();

            blockAdvertising = false;

            source.clear();

            maxRxSecs = 0;

            enableMulticastFailover = false;
            multicastFailoverSecs = 10;

            rtcpPresenceRx.clear();

            enableRxVad = false;

            presenceGroupAffinities.clear();
            disablePacketEvents = false;

            rfc4733RtpPayloadId = 0;
            inboundRtpPayloadTypeTranslations.clear();
            priorityTranslation.clear();

            stickyTidHangSecs = 10;
            anonymousAlias.clear();
            lbCrypto = false;

            satPaq.clear();
            lynqPro.clear();
            transportName.clear();
            allowLoopback = false;

            rtpProfile.clear();
            rangerPackets.clear();

            _wasDeserialized_rtpProfile = false;

            txImpairment.clear();
            rxImpairment.clear();

            specializerAffinities.clear();

            securityLevel = 0;

            ignoreSources.clear();

            languageCode.clear();
            synVoice.clear();

            rxCapture.clear();
            txCapture.clear();
        }
    };

    static void to_json(nlohmann::json& j, const Group& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(type),
            TOJSON_IMPL(bom),
            TOJSON_IMPL(id),
            TOJSON_IMPL(name),
            TOJSON_IMPL(spokenName),
            TOJSON_IMPL(interfaceName),
            TOJSON_IMPL(rx),
            TOJSON_IMPL(tx),
            TOJSON_IMPL(txOptions),
            TOJSON_IMPL(txAudio),
            TOJSON_IMPL(presence),
            TOJSON_IMPL(cryptoPassword),
            TOJSON_IMPL(alias),

            // See below
            //TOJSON_IMPL(rallypoints),
            //TOJSON_IMPL(rallypointCluster),

            TOJSON_IMPL(alias),
            TOJSON_IMPL(audio),
            TOJSON_IMPL(timeline),
            TOJSON_IMPL(blockAdvertising),
            TOJSON_IMPL(source),
            TOJSON_IMPL(maxRxSecs),
            TOJSON_IMPL(enableMulticastFailover),
            TOJSON_IMPL(multicastFailoverSecs),
            TOJSON_IMPL(rtcpPresenceRx),
            TOJSON_IMPL(enableRxVad),
            TOJSON_IMPL(presenceGroupAffinities),
            TOJSON_IMPL(disablePacketEvents),
            TOJSON_IMPL(rfc4733RtpPayloadId),
            TOJSON_IMPL(inboundRtpPayloadTypeTranslations),
            TOJSON_IMPL(priorityTranslation),
            TOJSON_IMPL(stickyTidHangSecs),
            TOJSON_IMPL(anonymousAlias),
            TOJSON_IMPL(lbCrypto),
            TOJSON_IMPL(satPaq),
            TOJSON_IMPL(lynqPro),
            TOJSON_IMPL(transportName),
            TOJSON_IMPL(allowLoopback),
            TOJSON_IMPL(rangerPackets),

            TOJSON_IMPL(txImpairment),
            TOJSON_IMPL(rxImpairment),

            TOJSON_IMPL(specializerAffinities),

            TOJSON_IMPL(securityLevel),

            TOJSON_IMPL(ignoreSources),

            TOJSON_IMPL(languageCode),
            TOJSON_IMPL(synVoice),

            TOJSON_IMPL(rxCapture),
            TOJSON_IMPL(txCapture)
        };

        TOJSON_BASE_IMPL();

        // TODO: need a better way to indicate whether rtpProfile is present
        if(p._wasDeserialized_rtpProfile || p.isDocumenting())
        {
            j["rtpProfile"] = p.rtpProfile;
        }

        if(p.isDocumenting())
        {
            j["rallypointCluster"] = p.rallypointCluster;
            j["rallypoints"] = p.rallypoints;
        }
        else
        {
            // rallypointCluster takes precedence if it has elements
            if(!p.rallypointCluster.rallypoints.empty())
            {
                j["rallypointCluster"] = p.rallypointCluster;
            }
            else if(!p.rallypoints.empty())
            {
                j["rallypoints"] = p.rallypoints;
            }
        }
    }
    static void from_json(const nlohmann::json& j, Group& p)
    {
        p.clear();
        j.at("type").get_to(p.type);
        getOptional<Group::BridgingOpMode_t>("bom", p.bom, j, Group::BridgingOpMode_t::bomRaw);
        j.at("id").get_to(p.id);
        getOptional<std::string>("name", p.name, j);
        getOptional<std::string>("spokenName", p.spokenName, j);
        getOptional<std::string>("interfaceName", p.interfaceName, j);
        getOptional<NetworkAddress>("rx", p.rx, j);
        getOptional<NetworkAddress>("tx", p.tx, j);
        getOptional<NetworkTxOptions>("txOptions", p.txOptions, j);
        getOptional<std::string>("cryptoPassword", p.cryptoPassword, j);
        getOptional<std::string>("alias", p.alias, j);
        getOptional<TxAudio>("txAudio", p.txAudio, j);
        getOptional<Presence>("presence", p.presence, j);
        getOptional<std::vector<Rallypoint>>("rallypoints", p.rallypoints, j);
        getOptional<RallypointCluster>("rallypointCluster", p.rallypointCluster, j);        
        getOptional<Audio>("audio", p.audio, j);
        getOptional<GroupTimeline>("timeline", p.timeline, j);
        getOptional<bool>("blockAdvertising", p.blockAdvertising, j, false);
        getOptional<std::string>("source", p.source, j);
        getOptional<int>("maxRxSecs", p.maxRxSecs, j, 0);
        getOptional<bool>("enableMulticastFailover", p.enableMulticastFailover, j, false);
        getOptional<int>("multicastFailoverSecs", p.multicastFailoverSecs, j, 10);
        getOptional<NetworkAddress>("rtcpPresenceRx", p.rtcpPresenceRx, j);
        getOptional<bool>("enableRxVad", p.enableRxVad, j, false);
        getOptional<std::vector<std::string>>("presenceGroupAffinities", p.presenceGroupAffinities, j);
        getOptional<bool>("disablePacketEvents", p.disablePacketEvents, j, false);
        getOptional<int>("rfc4733RtpPayloadId", p.rfc4733RtpPayloadId, j, 0);
        getOptional<std::vector<RtpPayloadTypeTranslation>>("inboundRtpPayloadTypeTranslations", p.inboundRtpPayloadTypeTranslations, j);
        getOptional<GroupPriorityTranslation>("priorityTranslation", p.priorityTranslation, j);
        getOptional<int>("stickyTidHangSecs", p.stickyTidHangSecs, j, 10);
        getOptional<std::string>("anonymousAlias", p.anonymousAlias, j);
        getOptional<bool>("lbCrypto", p.lbCrypto, j, false);
        getOptional<GroupSatPaq>("satPaq", p.satPaq, j);
        getOptional<GroupLynQPro>("lynqPro", p.lynqPro, j);
        getOptional<std::string>("transportName", p.transportName, j);
        getOptional<bool>("allowLoopback", p.allowLoopback, j, false);
        getOptionalWithIndicator<RtpProfile>("rtpProfile", p.rtpProfile, j, &p._wasDeserialized_rtpProfile);
        getOptional<RangerPackets>("rangerPackets", p.rangerPackets, j);
        getOptional<TransportImpairment>("txImpairment", p.txImpairment, j);
        getOptional<TransportImpairment>("rxImpairment", p.rxImpairment, j);
        getOptional<std::vector<uint16_t>>("specializerAffinities", p.specializerAffinities, j);
        getOptional<uint32_t>("securityLevel", p.securityLevel, j, 0);
        getOptional<std::vector<Source>>("ignoreSources", p.ignoreSources, j);
        getOptional<std::string>("languageCode", p.languageCode, j);
        getOptional<std::string>("synVoice", p.synVoice, j);        

        getOptional<PacketCapturer>("rxCapture", p.rxCapture, j);
        getOptional<PacketCapturer>("txCapture", p.txCapture, j);

        FROMJSON_BASE_IMPL();
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(Mission)
    class Mission : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(Mission)

    public:
        std::string                             id;
        std::string                             name;
        std::vector<Group>                      groups;
        std::chrono::system_clock::time_point   begins;
        std::chrono::system_clock::time_point   ends;
        std::string                             certStoreId;
        int                                     multicastFailoverPolicy;
        Rallypoint                              rallypoint;

        void clear()
        {
            id.clear();
            name.clear();
            groups.clear();
            certStoreId.clear();
            multicastFailoverPolicy = 0;
            rallypoint.clear();
        }
    };

    static void to_json(nlohmann::json& j, const Mission& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(name),
            TOJSON_IMPL(groups),
            TOJSON_IMPL(certStoreId),
            TOJSON_IMPL(multicastFailoverPolicy),
            TOJSON_IMPL(rallypoint)
        };
    }

    static void from_json(const nlohmann::json& j, Mission& p)
    {
        p.clear();
        j.at("id").get_to(p.id);
        j.at("name").get_to(p.name);

        // Groups are optional
        try
        {
            j.at("groups").get_to(p.groups);
        }
        catch(...)
        {
            p.groups.clear();
        }

        FROMJSON_IMPL(certStoreId, std::string, EMPTY_STRING);
        FROMJSON_IMPL(multicastFailoverPolicy, int, 0);
        getOptional<Rallypoint>("rallypoint", p.rallypoint, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(LicenseDescriptor)
    /**
    * @brief Helper class for serializing and deserializing the LicenseDescriptor JSON
    *
    * Helper C++ class to serialize and de-serialize LicenseDescriptor JSON
    *
    * Example: @include[doc] examples/LicenseDescriptor.json
    *
    * @see TODO: engageGetActiveLicenseDescriptor, engageGetLicenseDescriptor
    */
    class LicenseDescriptor : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(LicenseDescriptor)

    public:
        /** @addtogroup licensingStatusCodes Licensing Codes
         *
         * Status codes used to determine licensing status
         *  @{
         */
        static const int STATUS_OK = 0;
        static const int ERR_NULL_ENTITLEMENT_KEY = -1;
        static const int ERR_NULL_LICENSE_KEY = -2;
        static const int ERR_INVALID_LICENSE_KEY_LEN = -3;
        static const int ERR_LICENSE_KEY_VERIFICATION_FAILURE = -4;
        static const int ERR_ACTIVATION_CODE_VERIFICATION_FAILURE = -5;
        static const int ERR_INVALID_EXPIRATION_DATE = -6;
        static const int ERR_GENERAL_FAILURE = -7;
        static const int ERR_NOT_INITIALIZED = -8;
        static const int ERR_REQUIRES_ACTIVATION = -9;
        static const int ERR_LICENSE_NOT_SUITED_FOR_ACTIVATION = -10;
        /** @} */

        /** @addtogroup licensingFlags Licensing Flags
         *
         * Reserved flags for cargo
         *  @{
         */
        static const uint8_t LIC_CARGO_FLAG_LIMIT_TO_FEATURES = 0x01;
        /** @} */

        /**
         * @brief Entitlement key to use for the product.
         *
         * The entitlement key is a unique key generated by an application developer so that any license keys generated by
         * Rally Tactical's licensing system can only be used on a product with the same matching entitlement key. E.g If you develop two
         * products and you would like to issue license keys independently and would like the Engage Engine and licensing system to handle the
         * entitlement, then you should generate and separate entitlement key for each application.
         */
        std::string                             entitlement;

        /**
         * @brief License Key to be used for the application.
         *
         * This key is generated by the Rally Tactical Licensing systems and requires the entitlement key for creation. This key is then
         * locked to the entitlement key and can never be changed.
        */
        std::string                             key;

        /** @brief If the key required activation, this is the activation code generated using the entitlement, key and deviceId. */
        std::string                             activationCode;

        /** @brief [Read only] Unique device identifier generated by the Engine. */
        std::string                             deviceId;

        /** @brief [Read only] 0 = unknown, 1 = perpetual, 2 = expires */
        int                                     type;

        /** @brief [Read only] The time that the license key or activation code expires in Unix timestamp - Zulu/UTC. */
        time_t                                  expires;

        /** @brief [Read only] The time that the license key or activation code expires formatted in ISO 8601 format, Zulu/UTC. */
        std::string                             expiresFormatted;

        /** @brief Reserved 
         * 
         * @see licensingFlags
        */
        uint32_t                                flags;

        /** @brief Reserved for internal use */
        std::string                             cargo;

        /** @brief Reserved for internal use */
        uint8_t                                 cargoFlags;

        /** @brief The current licensing status.
         *
         * @see licensingStatusCodes
         */
        int                                     status;

        /** @brief [Read only] Manufacturer ID. */
        std::string                             manufacturerId;

        LicenseDescriptor()
        {
            clear();
        }

        void clear()
        {
            entitlement.clear();
            key.clear();
            activationCode.clear();
            type = 0;
            expires = 0;
            expiresFormatted.clear();
            flags = 0;
            cargo.clear();
            cargoFlags = 0;
            deviceId.clear();
            status = ERR_NOT_INITIALIZED;
            manufacturerId.clear();
        }
    };

    static void to_json(nlohmann::json& j, const LicenseDescriptor& p)
    {
        j = nlohmann::json{
            //TOJSON_IMPL(entitlement),
            {"entitlement", "*entitlement*"},
            TOJSON_IMPL(key),
            TOJSON_IMPL(activationCode),
            TOJSON_IMPL(type),
            TOJSON_IMPL(expires),
            TOJSON_IMPL(expiresFormatted),
            TOJSON_IMPL(flags),
            TOJSON_IMPL(deviceId),
            TOJSON_IMPL(status),
            //TOJSON_IMPL(manufacturerId),
            {"manufacturerId", "*manufacturerId*"},
            TOJSON_IMPL(cargo),
            TOJSON_IMPL(cargoFlags)
        };
    }

    static void from_json(const nlohmann::json& j, LicenseDescriptor& p)
    {
        p.clear();
        FROMJSON_IMPL(entitlement, std::string, EMPTY_STRING);
        FROMJSON_IMPL(key, std::string, EMPTY_STRING);
        FROMJSON_IMPL(activationCode, std::string, EMPTY_STRING);
        FROMJSON_IMPL(type, int, 0);
        FROMJSON_IMPL(expires, time_t, 0);
        FROMJSON_IMPL(expiresFormatted, std::string, EMPTY_STRING);
        FROMJSON_IMPL(flags, uint32_t, 0);
        FROMJSON_IMPL(deviceId, std::string, EMPTY_STRING);
        FROMJSON_IMPL(status, int, LicenseDescriptor::ERR_NOT_INITIALIZED);
        FROMJSON_IMPL(manufacturerId, std::string, EMPTY_STRING);
        FROMJSON_IMPL(cargo, std::string, EMPTY_STRING);
        FROMJSON_IMPL(cargoFlags, uint8_t, 0);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(EngineNetworkingRpUdpStreaming)
    /**
     * @brief Configuration for RP UDP streaming
     *
     *  Helper C++ class to serialize and de-serialize EngineNetworkingRpUdpStreaming JSON
     *
     *  TODO: Complete this Class
     *
     *  Example: @include[doc] examples/EngineNetworkingRpUdpStreaming.json
     *
     *  @see Group
     */
    class EngineNetworkingRpUdpStreaming : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(EngineNetworkingRpUdpStreaming)

    public:
        /** @brief [Optional, false] Enables UDP streaming if the RP supports it */
        bool                                enabled;

        /** @brief [Optional, EnginePolicyNetworking::defaultNic] Name of NIC to bind to - uses Engine defaultNic if empty */
        std::string                         nic;

        /** @brief [Optional, 0] The port to be used for Rallypoint UDP streaming.  A value of 0 will result in an ephemeral port being assigned.*/
        int                                port;

        /** @brief Optional, Default: 10] Seconds interval at which to send UDP keepalives to Rallypoints.  This is important for NAT "hole-punching". */
        int                                keepAliveIntervalSecs;

        /** @brief [Optional, Default: @ref priVoice] Transmission priority. This has meaning on some operating systems based on how their IP stack operates.  It may or may not affect final packet marking. */
        NetworkTxOptions::TxPriority_t     priority;

        /** @brief [Optional, Default: 64] Time to live or hop limit is a mechanism that limits the lifespan or lifetime of data in a network. TTL prevents a data packet from circulating indefinitely. */
        int                                ttl;

        EngineNetworkingRpUdpStreaming()
        {
            clear();
        }

        void clear()
        {
            enabled = false;
            nic.clear();
            keepAliveIntervalSecs = 10;
            priority = NetworkTxOptions::priVoice;
            ttl = 64;
        }

        virtual void initForDocumenting()
        {
        }
    };

    static void to_json(nlohmann::json& j, const EngineNetworkingRpUdpStreaming& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(nic),
            TOJSON_IMPL(port),
            TOJSON_IMPL(keepAliveIntervalSecs),
            TOJSON_IMPL(priority),
            TOJSON_IMPL(ttl)
        };
    }
    static void from_json(const nlohmann::json& j, EngineNetworkingRpUdpStreaming& p)
    {
        p.clear();
        getOptional<bool>("enabled", p.enabled, j, false);
        getOptional<std::string>("nic", p.nic, j, EMPTY_STRING);
        getOptional<int>("port", p.port, j, 0);
        getOptional<int>("keepAliveIntervalSecs", p.keepAliveIntervalSecs, j, 10);
        getOptional<NetworkTxOptions::TxPriority_t>("priority", p.priority, j, NetworkTxOptions::priVoice);
        getOptional<int>("ttl", p.ttl, j, 64);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(EnginePolicyNetworking)
    /**
    *
    * Helper C++ class to serialize and de-serialize EnginePolicyNetworking JSON
    *
    * Example: @include[doc] examples/EnginePolicyNetworking.json
    *
    * @see TODO: Add references
    */
    class EnginePolicyNetworking : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(EnginePolicyNetworking)

    public:
        /** @brief The default network interface card the Engage Engine should bind to. */
        std::string         defaultNic;

        /** @brief [Optional, Default: 8] Number of seconds elapsed between RX of multicast packets before an IGMP rejoin is made */
        int                 multicastRejoinSecs;

        /** @brief [Optional, Default: 60000] Milliseconds between sending Rallypoint round-trip test requests */
        int                 rallypointRtTestIntervalMs;

        /** @brief [Optional, Default: false] If true, logs RTP jitter buffer statistics periodically */
        bool                logRtpJitterBufferStats;

        /** @brief [Optional, Default: false] Overrides/cancels group-level multicast failover if set to true */
        bool                preventMulticastFailover;        

        /** @brief [Optional] Configuration for UDP streaming */
        EngineNetworkingRpUdpStreaming  rpUdpStreaming;

        /** @brief [Optional] Configuration for RTP profile */
        RtpProfile          rtpProfile;

        EnginePolicyNetworking()
        {
            clear();
        }

        void clear()
        {
            defaultNic.clear();
            multicastRejoinSecs = 8;
            rallypointRtTestIntervalMs = 60000;
            logRtpJitterBufferStats = false;
            preventMulticastFailover = false;

            rpUdpStreaming.clear();
            rtpProfile.clear();
        }
    };

    static void to_json(nlohmann::json& j, const EnginePolicyNetworking& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(defaultNic),
            TOJSON_IMPL(multicastRejoinSecs),

            TOJSON_IMPL(rallypointRtTestIntervalMs),
            TOJSON_IMPL(logRtpJitterBufferStats),
            TOJSON_IMPL(preventMulticastFailover),

            TOJSON_IMPL(rpUdpStreaming),
            TOJSON_IMPL(rtpProfile)
        };
    }
    static void from_json(const nlohmann::json& j, EnginePolicyNetworking& p)
    {
        p.clear();
        FROMJSON_IMPL(defaultNic, std::string, EMPTY_STRING);
        FROMJSON_IMPL(multicastRejoinSecs, int, 8);
        FROMJSON_IMPL(rallypointRtTestIntervalMs, int, 60000);
        FROMJSON_IMPL(logRtpJitterBufferStats, bool, false);
        FROMJSON_IMPL(preventMulticastFailover, bool, false);

        getOptional<EngineNetworkingRpUdpStreaming>("rpUdpStreaming", p.rpUdpStreaming, j);
        getOptional<RtpProfile>("rtpProfile", p.rtpProfile, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(Aec)
    /**
    * @brief Acoustic Echo Cancellation settings
    *
    * Helper C++ class to serialize and de-serialize Aec JSON
    *
    * Example: @include[doc] examples/Aec.json
    *
    * @see TODO: ConfigurationObjects::Aec
    */
    class Aec : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(Aec)

    public:
        /**
         * @brief Acoustic echo cancellation mode enum.
         *
         * More detailed Mode_t description.
         */
        typedef enum
        {
            /** @brief Default */
            aecmDefault         = 0,

            /** @brief Low */
            aecmLow             = 1,

            /** @brief Medium */
            aecmMedium          = 2,

            /** @brief High */
            aecmHigh            = 3,

            /** @brief Very High */
            aecmVeryHigh        = 4,

            /** @brief Highest */
            aecmHighest         = 5
        } Mode_t;

        /** @brief [Optional, Default: false] Enable acoustic echo cancellation */
        bool                enabled;

        /** @brief [Optional, Default: @ref aecmDefault] Specifies AEC mode. See @ref Mode_t for all modes */
        Mode_t              mode;

        /** @brief [Optional, Default: 60] Milliseconds of speaker tail */
        int                 speakerTailMs;

        /** @brief [Optional, Default: true] Enable comfort noise generation */
        bool                cng;

        Aec()
        {
            clear();
        }

        void clear()
        {
            enabled = false;
            mode = aecmDefault;
            speakerTailMs = 60;
            cng = true;
        }
    };

    static void to_json(nlohmann::json& j, const Aec& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(mode),
            TOJSON_IMPL(speakerTailMs),
            TOJSON_IMPL(cng)
        };
    }
    static void from_json(const nlohmann::json& j, Aec& p)
    {
        p.clear();
        FROMJSON_IMPL(enabled, bool, false);
        FROMJSON_IMPL(mode, Aec::Mode_t, Aec::Mode_t::aecmDefault);
        FROMJSON_IMPL(speakerTailMs, int, 60);
        FROMJSON_IMPL(cng, bool, true);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(Vad)
    /**
    * @brief Voice Activity Detection settings
    *
    * Helper C++ class to serialize and de-serialize Vad JSON
    *
    * Example: @include[doc] examples/Vad.json
    *
    * @see TODO: ConfigurationObjects::Vad
    */
    class Vad : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(Vad)

    public:
        /**
         * @brief VAD enum.
         *
         * More detailed Mode_t description.
         */
        typedef enum
        {
            /** @brief Default */
            vamDefault          = 0,

            /** @brief Low bit rate */
            vamLowBitRate       = 1,

            /** @brief Aggressive */
            vamAggressive       = 2,

            /** @brief High */
            vamVeryAggressive   = 3
        } Mode_t;

        /** @brief [Optional, Default: false] Enable VAD */
        bool                enabled;

        /** @brief [Optional, Default: @ref vamDefault] Specifies VAD mode. See @ref Mode_t for all modes */
        Mode_t              mode;

        Vad()
        {
            clear();
        }

        void clear()
        {
            enabled = false;
            mode = vamDefault;
        }
    };

    static void to_json(nlohmann::json& j, const Vad& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(mode)
        };
    }
    static void from_json(const nlohmann::json& j, Vad& p)
    {
        p.clear();
        FROMJSON_IMPL(enabled, bool, false);
        FROMJSON_IMPL(mode, Vad::Mode_t, Vad::Mode_t::vamDefault);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(Bridge)
    /**
    * @brief Bridging session settings
    *
    * Helper C++ class to serialize and de-serialize Bridge JSON
    *
    * Example: @include[doc] examples/Bridge.json
    *
    * @see TODO: ConfigurationObjects::Bridge
    */
    class Bridge : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(Bridge)

    public:
        /** @brief ID */
        std::string                 id;

        /** @brief Name */
        std::string                 name;

        /** @brief List of group IDs to be included in the session */
        std::vector<std::string>    groups;

        /** @brief [Optional, Default: true] Enable the bridge */
        bool                        enabled;

        Bridge()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            name.clear();
            groups.clear();
            enabled = true;
        }
    };

    static void to_json(nlohmann::json& j, const Bridge& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(name),
            TOJSON_IMPL(groups),
            TOJSON_IMPL(enabled)
        };
    }
    static void from_json(const nlohmann::json& j, Bridge& p)
    {
        p.clear();
        FROMJSON_IMPL(id, std::string, EMPTY_STRING);
        FROMJSON_IMPL(name, std::string, EMPTY_STRING);
        getOptional<std::vector<std::string>>("groups", p.groups, j);
        FROMJSON_IMPL(enabled, bool, true);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(AndroidAudio)
    /**
    * @brief Default audio settings for AndroidAudio.
    *
    * Helper C++ class to serialize and de-serialize AndroidAudio JSON
    *
    * Example: @include[doc] examples/AndroidAudio.json
    *
    * @see TODO: ConfigurationObjects::AndroidAudio
    */
    class AndroidAudio : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(AndroidAudio)

    public:
        constexpr static int INVALID_SESSION_ID = -9999;

        /** @brief [Optional, Default 0] Android audio API version: 0=Unspecified, 1=AAudio, 2=OpenGLES */
        int                 api;

        /** @brief [Optional, Default 0] Sharing mode: 0=Exclusive, 1=Shared */
        int                 sharingMode;

        /** @brief [Optional, Default 12] Performance mode: 10=None/Default, 11=PowerSaving, 12=LowLatency */
        int                 performanceMode;

        /** @brief [Optional, Default 2] Usage type: 
         *          1=Media
         *          2=VoiceCommunication
         *          3=VoiceCommunicationSignalling
         *          4=Alarm
         *          5=Notification
         *          6=NotificationRingtone
         *          10=NotificationEvent
         *          11=AssistanceAccessibility
         *          12=AssistanceNavigationGuidance
         *          13=AssistanceSonification
         *          14=Game
         *          16=Assistant
         *  */
        int                 usage;

        /** @brief [Optional, Default 1] Usage type: 
         *          1=Speech
         *          2=Music
         *          3=Movie
         *          4=Sonification
         * */
        int                 contentType;

        /** @brief [Optional, Default 7] Input preset: 
         *          1=Generic
         *          5=Camcorder
         *          6=VoiceRecognition
         *          7=VoiceCommunication
         *          9=Unprocessed
         *          10=VoicePerformance
         * */
        int                 inputPreset;

        /** @brief [Optional, Default INVALID_SESSION_ID] A session ID from the Android AudioManager */
        int                 sessionId;

        /** @brief [Optional, Default 0] 0=use legacy low-level APIs, 1=use high-level Android APIs */
        int                 engineMode;


        AndroidAudio()
        {
            clear();
        }

        void clear()
        {
            api = 0;
            sharingMode = 0;
            performanceMode = 12;
            usage = 2;
            contentType = 1;
            inputPreset = 7;
            sessionId = AndroidAudio::INVALID_SESSION_ID;
            engineMode = 0;
        }
    };

    static void to_json(nlohmann::json& j, const AndroidAudio& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(api),
            TOJSON_IMPL(sharingMode),
            TOJSON_IMPL(performanceMode),
            TOJSON_IMPL(usage),
            TOJSON_IMPL(contentType),
            TOJSON_IMPL(inputPreset),
            TOJSON_IMPL(sessionId),
            TOJSON_IMPL(engineMode)
        };
    }
    static void from_json(const nlohmann::json& j, AndroidAudio& p)
    {
        p.clear();
        FROMJSON_IMPL(api, int, 0);
        FROMJSON_IMPL(sharingMode, int, 0);
        FROMJSON_IMPL(performanceMode, int, 12);
        FROMJSON_IMPL(usage, int, 2);
        FROMJSON_IMPL(contentType, int, 1);
        FROMJSON_IMPL(inputPreset, int, 7);
        FROMJSON_IMPL(sessionId, int, AndroidAudio::INVALID_SESSION_ID);
        FROMJSON_IMPL(engineMode, int, 0);
    }    
    
    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(EnginePolicyAudio)
    /**
    * @brief Default audio settings for Engage Engine policy.
    *
    * Helper C++ class to serialize and de-serialize EnginePolicyAudio JSON
    *
    * Example: @include[doc] examples/EnginePolicyAudio.json
    *
    * @see TODO: ConfigurationObjects::EnginePolicy
    */
    class EnginePolicyAudio : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(EnginePolicyAudio)

    public:
        /** @brief [Optional, Default: true] Enables audio processing */
        bool                enabled;

        /** @brief [Optional, Default: 16000] Internal sampling rate - 8000 or 16000 */
        int                 internalRate;

        /** @brief [Optional, Default: 2] Internal audio channel count rate - 1 or 2 */
        int                 internalChannels;

        /** @brief [Optional, Default: false] Automatically mute TX when TX begins */
        bool                muteTxOnTx;

        /** @brief [Optional] Acoustic echo cancellation settings */
        Aec                 aec;

        /** @brief [Optional] Voice activity detection settings */
        Vad                 vad;

        /** @brief [Optional] Android-specific audio settings */
        AndroidAudio        android;

        /** @brief [Optional] Automatic Gain Control for audio inputs */
        Agc                 inputAgc;

        /** @brief [Optional] Automatic Gain Control for audio outputs */
        Agc                 outputAgc;

        /** @brief [Optional, Default: false] Denoise input */
        bool                denoiseInput;

        /** @brief [Optional, Default: false] Denoise output */
        bool                denoiseOutput;

        EnginePolicyAudio()
        {
            clear();
        }

        void clear()
        {
            enabled = true;
            internalRate = 16000;
            internalChannels = 2;
            muteTxOnTx = false;
            aec.clear();
            vad.clear();
            android.clear();
            inputAgc.clear();
            outputAgc.clear();
            denoiseInput = false;
            denoiseOutput = false;
        }
    };

    static void to_json(nlohmann::json& j, const EnginePolicyAudio& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(internalRate),
            TOJSON_IMPL(internalChannels),
            TOJSON_IMPL(muteTxOnTx),
            TOJSON_IMPL(aec),
            TOJSON_IMPL(vad),
            TOJSON_IMPL(android),
            TOJSON_IMPL(inputAgc),
            TOJSON_IMPL(outputAgc),
            TOJSON_IMPL(denoiseInput),
            TOJSON_IMPL(denoiseOutput)
        };
    }
    static void from_json(const nlohmann::json& j, EnginePolicyAudio& p)
    {
        p.clear();
        getOptional<bool>("enabled", p.enabled, j, true);
        FROMJSON_IMPL(internalRate, int, 16000);
        FROMJSON_IMPL(internalChannels, int, 2);

        FROMJSON_IMPL(muteTxOnTx, bool, false);
        getOptional<Aec>("aec", p.aec, j);
        getOptional<Vad>("vad", p.vad, j);
        getOptional<AndroidAudio>("android", p.android, j);
        getOptional<Agc>("inputAgc", p.inputAgc, j);
        getOptional<Agc>("outputAgc", p.outputAgc, j);
        FROMJSON_IMPL(denoiseInput, bool, false);
        FROMJSON_IMPL(denoiseOutput, bool, false);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(SecurityCertificate)
    /**
    * @brief Configuration for a Security Certificate used in various configurations.
    *
    * Helper C++ class to serialize and de-serialize SecurityCertificate JSON
    *
    * Example: @include[doc] examples/SecurityCertificate.json
    *
    * @see ConfigurationObjects::EnginePolicySecurity, ConfigurationObjects::RallypointServer
    */
    class SecurityCertificate : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(SecurityCertificate)

    public:

        /**
         * @brief Contains the PEM-formatted text of the certificate, OR, a reference to a PEM file denoted by "@file://" URI, OR 
         *        a reference to named element in the currently active certificate store using the "@certstore://" URI.
         *
         */
        std::string         certificate;

        /** @brief As for above but for certificate's private key. */
        std::string         key;

        SecurityCertificate()
        {
            clear();
        }

        void clear()
        {
            certificate.clear();
            key.clear();
        }
    };

    static void to_json(nlohmann::json& j, const SecurityCertificate& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(certificate),
            TOJSON_IMPL(key)
        };
    }
    static void from_json(const nlohmann::json& j, SecurityCertificate& p)
    {
        p.clear();
        FROMJSON_IMPL(certificate, std::string, EMPTY_STRING);
        FROMJSON_IMPL(key, std::string, EMPTY_STRING);
    }

    // This is where spell checking stops
    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(EnginePolicySecurity)

    /**
    * @brief Default certificate to use for security operation in the Engage Engine.
    *
    * Helper C++ class to serialize and de-serialize EnginePolicySecurity JSON
    *
    * Example: @include[doc] examples/EnginePolicySecurity.json
    *
    * @see ConfigurationObjects::EnginePolicy
    */
    class EnginePolicySecurity : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(EnginePolicySecurity)

    public:

        /** 
         * @brief The default certificate and private key for the Engine instance 
         * 
         * Description
         * 
         * The certificate and private key are used for a variety of purposes including:
         * - Mission generation
         * - As the default for timeline signing
         * - As the default for Rallypoint connectivity
         * */
        SecurityCertificate     certificate;

        /** 
         * @brief [Optional] An array of CA certificates to be used for validation of far-end X.509 certificates 
         * 
         * Description
         * 
         * These certificates will be used on Rallypoint connections where no specific CA certificates have been provided.
        */
        std::vector<std::string>    caCertificates;

        EnginePolicySecurity()
        {
            clear();
        }

        void clear()
        {
            certificate.clear();
            caCertificates.clear();
        }
    };

    static void to_json(nlohmann::json& j, const EnginePolicySecurity& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(certificate),
            TOJSON_IMPL(caCertificates)
        };
    }
    static void from_json(const nlohmann::json& j, EnginePolicySecurity& p)
    {
        p.clear();
        getOptional("certificate", p.certificate, j);
        getOptional<std::vector<std::string>>("caCertificates", p.caCertificates, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(EnginePolicyLogging)
    /**
    * @brief Engine logging settings.
    *
    * Helper C++ class to serialize and de-serialize EnginePolicyLogging JSON
    *
    * Example: @include[doc] examples/EnginePolicyLogging.json
    *
    * @see ConfigurationObjects::EnginePolicy
    */
    class EnginePolicyLogging : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(EnginePolicyLogging)

    public:

        /**
         * @brief [Optional, Default: 4, Range: 0-4] This is the maximum logging level to display in other words, any logging with levels equal or lower than this level will be logged.
         * @see loggingLevels
         * 
         * Logging levels
         * 
         * Value    | Severity      | Description
         * ---      | ---           | ---
         * 0        | Fatal         | A fatal, non-recoverable, error has occurred
         * 1        | Error         | An error has occurred but the system has recovered
         * 2        | Warning       | A warning condition exists
         * 3        | Informational | The message is informational in nature
         * 4        | Debug         | The message is useful for debugging and troubleshooting
         *
         */
        int     maxLevel;

        /** [Optional, Default: false] When enabled, the Engage Engine will output logging to Sys log as well. */
        bool    enableSyslog;

        EnginePolicyLogging()
        {
            clear();
        }

        void clear()
        {
            maxLevel = 4;                                   // ILogger::Level::debug
            enableSyslog = false;
        }
    };

    static void to_json(nlohmann::json& j, const EnginePolicyLogging& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(maxLevel),
            TOJSON_IMPL(enableSyslog)
        };
    }
    static void from_json(const nlohmann::json& j, EnginePolicyLogging& p)
    {
        p.clear();
        getOptional("maxLevel", p.maxLevel, j, 4);          // ILogger::Level::debug
        getOptional("enableSyslog", p.enableSyslog, j);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(EnginePolicyDatabase)
    class EnginePolicyDatabase : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(EnginePolicyDatabase)

    public:
        typedef enum
        {
            dbtFixedMemory          = 0,
            dbtPagedMemory          = 1,
            dbtFixedFile            = 2
        } DatabaseType_t;

        DatabaseType_t      type;
        std::string         fixedFileName;
        bool                forceMaintenance;
        bool                reclaimSpace;

        EnginePolicyDatabase()
        {
            clear();
        }

        void clear()
        {
            type = DatabaseType_t::dbtFixedMemory;
            fixedFileName.clear();
            forceMaintenance = false;
            reclaimSpace = false;
        }
    };

    static void to_json(nlohmann::json& j, const EnginePolicyDatabase& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(type),
            TOJSON_IMPL(fixedFileName),
            TOJSON_IMPL(forceMaintenance),
            TOJSON_IMPL(reclaimSpace)
        };
    }
    static void from_json(const nlohmann::json& j, EnginePolicyDatabase& p)
    {
        p.clear();
        FROMJSON_IMPL(type, EnginePolicyDatabase::DatabaseType_t, EnginePolicyDatabase::DatabaseType_t::dbtFixedMemory);
        FROMJSON_IMPL(fixedFileName, std::string, EMPTY_STRING);
        FROMJSON_IMPL(forceMaintenance, bool, false);
        FROMJSON_IMPL(reclaimSpace, bool, false);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(NamedAudioDevice)
    class NamedAudioDevice : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(NamedAudioDevice)

    public:
        std::string                 name;
        std::string                 manufacturer;
        std::string                 model;
        std::string                 id;
        std::string                 serialNumber;
        std::string                 type;
        std::string                 extra;
        bool                        isDefault;

        NamedAudioDevice()
        {
            clear();
        }

        void clear()
        {
            name.clear();
            manufacturer.clear();
            model.clear();
            id.clear();
            serialNumber.clear();
            type.clear();
            extra.clear();
            isDefault = false;
        }
    };

    static void to_json(nlohmann::json& j, const NamedAudioDevice& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(name),
            TOJSON_IMPL(manufacturer),
            TOJSON_IMPL(model),
            TOJSON_IMPL(id),
            TOJSON_IMPL(serialNumber),
            TOJSON_IMPL(type),
            TOJSON_IMPL(extra),
            TOJSON_IMPL(isDefault),
        };
    }
    static void from_json(const nlohmann::json& j, NamedAudioDevice& p)
    {
        p.clear();
        getOptional<std::string>("name", p.name, j, EMPTY_STRING);
        getOptional<std::string>("manufacturer", p.manufacturer, j, EMPTY_STRING);
        getOptional<std::string>("model", p.model, j, EMPTY_STRING);
        getOptional<std::string>("id", p.id, j, EMPTY_STRING);
        getOptional<std::string>("serialNumber", p.serialNumber, j, EMPTY_STRING);
        getOptional<std::string>("type", p.type, j, EMPTY_STRING);
        getOptional<std::string>("extra", p.extra, j, EMPTY_STRING);
        getOptional<bool>("isDefault", p.isDefault, j, false);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(EnginePolicyNamedAudioDevices)
    class EnginePolicyNamedAudioDevices : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(EnginePolicyNamedAudioDevices)

    public:
        std::vector<NamedAudioDevice>    inputs;
        std::vector<NamedAudioDevice>    outputs;

        EnginePolicyNamedAudioDevices()
        {
            clear();
        }

        void clear()
        {
            inputs.clear();
            outputs.clear();
        }
    };

    static void to_json(nlohmann::json& j, const EnginePolicyNamedAudioDevices& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(inputs),
            TOJSON_IMPL(outputs)
        };
    }
    static void from_json(const nlohmann::json& j, EnginePolicyNamedAudioDevices& p)
    {
        p.clear();
        getOptional<std::vector<NamedAudioDevice>>("inputs", p.inputs, j);
        getOptional<std::vector<NamedAudioDevice>>("outputs", p.outputs, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(Licensing)
    /**
    * @brief Licensing settings
    *
    * Used to enable the Engage Engine for production features
    *
    * Helper C++ class to serialize and de-serialize Licensing JSON
    *
    * Example: @include[doc] examples/Licensing.json
    *
    * @see engageInitialize, engageGetActiveLicenseDescriptor, engageGetLicenseDescriptor, engageUpdateLicense, ConfigurationObjects::EnginePolicy,
    */
    class Licensing : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(Licensing)

    public:

        /** @brief Entitlement key to use for the product. See @ref LicenseDescriptor::entitlement for details */
        std::string         entitlement;

        /** @brief License key. See @ref LicenseDescriptor::key for details */
        std::string         key;

        /** @brief Activation Code issued for the license key. See @ref LicenseDescriptor::activationCode for details */
        std::string         activationCode;

        /** @brief Device Identifier. See @ref LicenseDescriptor::deviceId for details */
        std::string         deviceId;

        /** @brief Manufacturer ID to use for the product. See @ref LicenseDescriptor::manufacturerId for details */
        std::string         manufacturerId;

        Licensing()
        {
            clear();
        }

        void clear()
        {
            entitlement.clear();
            key.clear();
            activationCode.clear();
            deviceId.clear();
            manufacturerId.clear();
        }
    };

    static void to_json(nlohmann::json& j, const Licensing& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(entitlement),
            TOJSON_IMPL(key),
            TOJSON_IMPL(activationCode),
            TOJSON_IMPL(deviceId),
            TOJSON_IMPL(manufacturerId)
        };
    }
    static void from_json(const nlohmann::json& j, Licensing& p)
    {
        p.clear();
        FROMJSON_IMPL(entitlement, std::string, EMPTY_STRING);
        FROMJSON_IMPL(key, std::string, EMPTY_STRING);
        FROMJSON_IMPL(activationCode, std::string, EMPTY_STRING);
        FROMJSON_IMPL(deviceId, std::string, EMPTY_STRING);
        FROMJSON_IMPL(manufacturerId, std::string, EMPTY_STRING);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(DiscoveryMagellan)
    /**
    * @brief DiscoveryMagellan Discovery settings
    *
    * Helper C++ class to serialize and de-serialize DiscoveryMagellan JSON
    *
    * Example: @include[doc] examples/DiscoveryMagellan.json
    *
    * @see engageInitialize, ConfigurationObjects::DiscoveryConfiguration
    */
    class DiscoveryMagellan : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(DiscoveryMagellan)

    public:

        /** [Optional, Default false] Enable Magellan discovery. */
        bool                                    enabled;

        /** @brief [Optional, Default: default system interface] The network interface to bind to for discovery packets.  */
        std::string                             interfaceName;

        /** [Optional if disabled] X.509 certificate information for interrogation */
        SecurityCertificate                     security;

        /** @brief [Optional] Details concerning Transport Layer Security.  @see Tls */
        Tls                                         tls;

        DiscoveryMagellan()
        {
            clear();
        }

        void clear()
        {
            enabled = false;
            interfaceName.clear();
            security.clear();
            tls.clear();
        }
    };

    static void to_json(nlohmann::json& j, const DiscoveryMagellan& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(interfaceName),
            TOJSON_IMPL(security),
            TOJSON_IMPL(tls)
        };
    }
    static void from_json(const nlohmann::json& j, DiscoveryMagellan& p)
    {
        p.clear();
        getOptional("enabled", p.enabled, j, false);
        getOptional<Tls>("tls", p.tls, j);
        getOptional<SecurityCertificate>("security", p.security, j);
        FROMJSON_IMPL(interfaceName, std::string, EMPTY_STRING);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(DiscoverySsdp)
    /**
    * @brief <a href="https://en.wikipedia.org/wiki/Simple_Service_Discovery_Protocol" target="_blank">Simple Service Discovery Protocol</a>  settings.
    *
    * Helper C++ class to serialize and de-serialize DiscoverySsdp JSON
    *
    * Example: @include[doc] examples/DiscoverySsdp.json
    *
    * @see engageInitialize, ConfigurationObjects::DiscoveryConfiguration
    */
    class DiscoverySsdp : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(DiscoverySsdp)

    public:

        /** @brief [Optional, Default: false] Enables the Engage Engine to use SSDP for asset discovery. */
        bool                                    enabled;

        /** @brief [Optional, Default: default system interface] The network interface to bind to for discovery packets.  */
        std::string                             interfaceName;

        /** @brief  [Optional, Default 255.255.255.255:1900] IP address and port. */
        NetworkAddress                          address;

        /** @brief [Optional] An array of regex strings to be used to filter SSDP requests and responses. */
        std::vector<std::string>                searchTerms;

        /** @brief [Optional, Default 30000] Number of milliseconds of no SSDP announcment before the advertised entity is considered "gone". */
        int                                     ageTimeoutMs;

        /** @brief Parameters for advertising. */
        Advertising                             advertising;

        DiscoverySsdp()
        {
            clear();
        }

        void clear()
        {
            enabled = false;
            interfaceName.clear();
            address.clear();
            searchTerms.clear();
            ageTimeoutMs = 30000;
            advertising.clear();
        }
    };

    static void to_json(nlohmann::json& j, const DiscoverySsdp& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(interfaceName),
            TOJSON_IMPL(address),
            TOJSON_IMPL(searchTerms),
            TOJSON_IMPL(ageTimeoutMs),
            TOJSON_IMPL(advertising)
        };
    }
    static void from_json(const nlohmann::json& j, DiscoverySsdp& p)
    {
        p.clear();
        getOptional("enabled", p.enabled, j, false);
        getOptional<std::string>("interfaceName", p.interfaceName, j);

        getOptional<NetworkAddress>("address", p.address, j);
        if(p.address.address.empty())
        {
            p.address.address = "255.255.255.255";
        }
        if(p.address.port <= 0)
        {
            p.address.port = 1900;
        }

        getOptional<std::vector<std::string>>("searchTerms", p.searchTerms, j);
        getOptional<int>("ageTimeoutMs", p.ageTimeoutMs, j, 30000);
        getOptional<Advertising>("advertising", p.advertising, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(DiscoverySap)
    /**
    * @brief <a href="https://en.wikipedia.org/wiki/Session_Announcement_Protocol" target="_blank">Session Announcement Discovery settings</a>  settings.
    *
    * Helper C++ class to serialize and de-serialize DiscoverySap JSON
    *
    * Example: @include[doc] examples/DiscoverySap.json
    *
    * @see engageInitialize, ConfigurationObjects::DiscoveryConfiguration
    */
    class DiscoverySap : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(DiscoverySap)

    public:
        /** @brief [Optional, Default: false] Enables the Engage Engine to use SAP for asset discovery. */
        bool                                    enabled;

        /** @brief [Optional, Default: default system interface] The network interface to bind to for discovery packets.  */
        std::string                             interfaceName;

        /** @brief  [Optional, Default 224.2.127.254:9875] IP address and port. */
        NetworkAddress                          address;

        /** @brief [Optional, Default 30000] Number of milliseconds of no SAP announcment before the advertised entity is considered "gone". */
        int                                     ageTimeoutMs;

        /** @brief Parameters for advertising. */
        Advertising                             advertising;

        DiscoverySap()
        {
            clear();
        }

        void clear()
        {
            enabled = false;
            interfaceName.clear();
            address.clear();
            ageTimeoutMs = 30000;
            advertising.clear();
        }
    };

    static void to_json(nlohmann::json& j, const DiscoverySap& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(interfaceName),
            TOJSON_IMPL(address),
            TOJSON_IMPL(ageTimeoutMs),
            TOJSON_IMPL(advertising)
        };
    }
    static void from_json(const nlohmann::json& j, DiscoverySap& p)
    {
        p.clear();
        getOptional("enabled", p.enabled, j, false);
        getOptional<std::string>("interfaceName", p.interfaceName, j);
        getOptional<NetworkAddress>("address", p.address, j);
        if(p.address.address.empty())
        {
            p.address.address = "224.2.127.254";
        }
        if(p.address.port <= 0)
        {
            p.address.port = 9875;
        }

        getOptional<int>("ageTimeoutMs", p.ageTimeoutMs, j, 30000);
        getOptional<Advertising>("advertising", p.advertising, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(DiscoveryCistech)
    /**
    * @brief Cistech Discovery settings
    *
    * Used to configure the Engage Engine to discovery Cistech Gateways on the network using a propriety Citech protocol
    *
    * Helper C++ class to serialize and de-serialize DiscoveryCistech JSON
    *
    * Example: @include[doc] examples/DiscoveryCistech.json
    *
    * @see engageInitialize, ConfigurationObjects::DiscoveryConfiguration
    */
    class DiscoveryCistech : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(DiscoveryCistech)

    public:
        bool                                    enabled;
        std::string                             interfaceName;
        NetworkAddress                          address;
        int                                     ageTimeoutMs;

        DiscoveryCistech()
        {
            clear();
        }

        void clear()
        {
            enabled = false;
            interfaceName.clear();
            address.clear();
            ageTimeoutMs = 30000;
        }
    };

    static void to_json(nlohmann::json& j, const DiscoveryCistech& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(interfaceName),
            TOJSON_IMPL(address),
            TOJSON_IMPL(ageTimeoutMs)
        };
    }
    static void from_json(const nlohmann::json& j, DiscoveryCistech& p)
    {
        p.clear();
        getOptional("enabled", p.enabled, j, false);
        getOptional<std::string>("interfaceName", p.interfaceName, j);
        getOptional<NetworkAddress>("address", p.address, j);
        getOptional<int>("ageTimeoutMs", p.ageTimeoutMs, j, 30000);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(DiscoveryTrellisware)
    /**
    * @brief Trellisware Discovery settings
    *
    * Helper C++ class to serialize and de-serialize DiscoveryTrellisware JSON
    *
    * Example: @include[doc] examples/DiscoveryTrellisware.json
    *
    * @see engageInitialize, ConfigurationObjects::DiscoveryConfiguration
    */
    class DiscoveryTrellisware : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(DiscoveryTrellisware)

    public:

        /** [Optional, Default false] Enable Trellisware discovery. */
        bool                                    enabled;

        /** [Optional if disabled] X.509 certificate information for interrogation */
        SecurityCertificate                     security;

        DiscoveryTrellisware()
        {
            clear();
        }

        void clear()
        {
            enabled = false;
            security.clear();            
        }
    };

    static void to_json(nlohmann::json& j, const DiscoveryTrellisware& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(security)
        };
    }
    static void from_json(const nlohmann::json& j, DiscoveryTrellisware& p)
    {
        p.clear();
        getOptional("enabled", p.enabled, j, false);
        getOptional<SecurityCertificate>("security", p.security, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(DiscoveryConfiguration)
    /**
    * @brief Configuration for the Discovery features
    *
    * Helper C++ class to serialize and de-serialize DiscoveryConfiguration JSON
    *
    * Example: @include[doc] examples/DiscoveryConfiguration.json
    *
    * @see engageInitialize, ConfigurationObjects::EnginePolicy
    */
    class DiscoveryConfiguration : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(DiscoveryConfiguration)

    public:
        /** [Optional] Magellan (Magellan Discovery Protocol)  configuration. */
        DiscoveryMagellan                       magellan;

        /** [Optional] SSDP (Simple Session Discovery Protocol)  configuration. */
        DiscoverySsdp                           ssdp;

        /** [Optional] SAP (Simple Session Discovery Protocol) configuration. */
        DiscoverySap                            sap;

        /** [Optional] SSDP (Simple Session Discovery Protocol) configuration. */
        DiscoveryCistech                        cistech;

        /** [Optional] SSDP (Simple Session Discovery Protocol) configuration. */
        DiscoveryTrellisware                    trellisware;

        DiscoveryConfiguration()
        {
            clear();
        }

        void clear()
        {
            magellan.clear();
            ssdp.clear();
            sap.clear();
            cistech.clear();
        }
    };

    static void to_json(nlohmann::json& j, const DiscoveryConfiguration& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(magellan),
            TOJSON_IMPL(ssdp),
            TOJSON_IMPL(sap),
            TOJSON_IMPL(cistech),
            TOJSON_IMPL(trellisware)
        };
    }
    static void from_json(const nlohmann::json& j, DiscoveryConfiguration& p)
    {
        p.clear();
        getOptional<DiscoveryMagellan>("magellan", p.magellan, j);
        getOptional<DiscoverySsdp>("ssdp", p.ssdp, j);
        getOptional<DiscoverySap>("sap", p.sap, j);
        getOptional<DiscoveryCistech>("cistech", p.cistech, j);
        getOptional<DiscoveryTrellisware>("trellisware", p.trellisware, j);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(EnginePolicyInternals)
    /**
    * @brief Internal Engage Engine settings
    *
    * These settings are used to configure internal parameters.
    *
    * Helper C++ class to serialize and de-serialize EnginePolicyInternals JSON
    *
    * Example: @include[doc] examples/EnginePolicyInternals.json
    *
    * @see engageInitialize, ConfigurationObjects::EnginePolicy
    */
    class EnginePolicyInternals : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(EnginePolicyInternals)

    public:
        /** @brief [Optional] Settings for the Engine's watchdog. */
        WatchdogSettings    watchdog;

        /** @brief [Optional, Default: 1000] Interval at which to run the housekeeper thread. */
        int                 housekeeperIntervalMs;

        /** @brief [Optional, Default: 30] The default duration the @ref engageBeginGroupTx and @ref engageBeginGroupTxAdvanced function will transmit for. */
        int                 maxTxSecs;

        int                 maxRxSecs;

        int                 logTaskQueueStatsIntervalMs;

        bool                enableLazySpeakerClosure;

        /** @brief [Optional, Default: 60] The packet framing interval for audio streaming from a URI. */
        int                 uriStreamingIntervalMs;

        /** @brief [Optional, Default: @ref csRoundRobin] Specifies the default RP cluster connection strategy to be followed. See @ref ConnectionStrategy_t for all strategy types */
        RallypointCluster::ConnectionStrategy_t rpClusterStrategy;

        /** @brief [Optional, Default: 10] Seconds between switching to a new target in a RP cluster */
        int                                     rpClusterRolloverSecs;

        /** @brief [Optional, Default: 250] Interval at which to check for RTP expiration. */
        int                                     rtpExpirationCheckIntervalMs;

        /** @brief [Optional, Default: 5] Connection timeout in seconds to RP */
        int                                     rpConnectionTimeoutSecs;

        /** @brief [Optional, Default: 10] The number of seconds after which "sticky" transmission IDs expire. */
        int                                     stickyTidHangSecs;

        /** @brief [Optional, Default: 15] The number of seconds to cache an open microphone before actually closing it. */
        int                                     delayedMicrophoneClosureSecs;

        EnginePolicyInternals()
        {
            clear();
        }

        void clear()
        {
            watchdog.clear();
            housekeeperIntervalMs = 1000;
            logTaskQueueStatsIntervalMs = 0;
            maxTxSecs = 30;
            maxRxSecs = 0;
            enableLazySpeakerClosure = false;
            rpClusterStrategy = RallypointCluster::ConnectionStrategy_t::csRoundRobin;
            rpClusterRolloverSecs = 10;
            rtpExpirationCheckIntervalMs = 250;
            rpConnectionTimeoutSecs = 5;
            stickyTidHangSecs = 10;
            uriStreamingIntervalMs = 60;
            delayedMicrophoneClosureSecs = 15;
        }
    };

    static void to_json(nlohmann::json& j, const EnginePolicyInternals& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(watchdog),
            TOJSON_IMPL(housekeeperIntervalMs),
            TOJSON_IMPL(logTaskQueueStatsIntervalMs),
            TOJSON_IMPL(maxTxSecs),
            TOJSON_IMPL(maxRxSecs),
            TOJSON_IMPL(enableLazySpeakerClosure),
            TOJSON_IMPL(rpClusterStrategy),
            TOJSON_IMPL(rpClusterRolloverSecs),
            TOJSON_IMPL(rtpExpirationCheckIntervalMs),
            TOJSON_IMPL(rpConnectionTimeoutSecs),
            TOJSON_IMPL(stickyTidHangSecs),
            TOJSON_IMPL(uriStreamingIntervalMs),
            TOJSON_IMPL(delayedMicrophoneClosureSecs)
        };
    }
    static void from_json(const nlohmann::json& j, EnginePolicyInternals& p)
    {
        p.clear();
        getOptional<WatchdogSettings>("watchdog", p.watchdog, j);
        getOptional<int>("housekeeperIntervalMs", p.housekeeperIntervalMs, j, 1000);
        getOptional<int>("logTaskQueueStatsIntervalMs", p.logTaskQueueStatsIntervalMs, j, 0);
        getOptional<int>("maxTxSecs", p.maxTxSecs, j, 30);
        getOptional<int>("maxRxSecs", p.maxRxSecs, j, 0);
        getOptional<bool>("enableLazySpeakerClosure", p.enableLazySpeakerClosure, j, false);
        getOptional<RallypointCluster::ConnectionStrategy_t>("rpClusterStrategy", p.rpClusterStrategy, j, RallypointCluster::ConnectionStrategy_t::csRoundRobin);
        getOptional<int>("rpClusterRolloverSecs", p.rpClusterRolloverSecs, j, 10);
        getOptional<int>("rtpExpirationCheckIntervalMs", p.rtpExpirationCheckIntervalMs, j, 250);
        getOptional<int>("rpConnectionTimeoutSecs", p.rpConnectionTimeoutSecs, j, 5);
        getOptional<int>("stickyTidHangSecs", p.stickyTidHangSecs, j, 10);
        getOptional<int>("uriStreamingIntervalMs", p.uriStreamingIntervalMs, j, 60);
        getOptional<int>("delayedMicrophoneClosureSecs", p.delayedMicrophoneClosureSecs, j, 15);        
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(EnginePolicyTimelines)
    /**
    * @brief Engine Policy Timeline configuration.
    *
    * Timelines are used to record audio for instant replay and archival purposes. The audio files contain "anti tampering" features.
    *
    * Helper C++ class to serialize and de-serialize EnginePolicyTimelines JSON
    *
    * Example: @include[doc] examples/EnginePolicyTimelines.json
    *
    * @see engageInitialize, engageQueryGroupTimeline, EnginePolicy
    */
    class EnginePolicyTimelines : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(EnginePolicyTimelines)

    public:

        /**
         * @brief [Optional, Default: true] Specifies if Time Lines are enabled by default.
         *
         * Certain time line settings can be overridden at a per group level using the @ref ConfigurationObjects::GroupTimeline configuration.
         */
        bool                                    enabled;

        /** @brief Specifies where the timeline recordings will be stored physically. */
        std::string                             storageRoot;

        /** @brief Specifies the maximum storage space to use. */
        int                                     maxStorageMb;

        /** @brief Maximum age of an event after which it is to be erased */
        long                                    maxEventAgeSecs;

        /** @brief Maximum number of events to be retained */
        int                                     maxEvents;

        /** @brief Interval at which events are to be checked for age-based grooming */
        long                                    groomingIntervalSecs;

        /**
         * @brief The certificate to use for signing the recording.
         *
         * This is part of the anti tampering feature where the certificate and it's private key are used to digitally sign the event.  The public portion
         * of the certificate is added to the event file for later verification.
         *
         */
        SecurityCertificate                     security;

        /** @brief [Default 5] Interval at which events are to be saved from memory to disk (a slow operation) */
        long                                    autosaveIntervalSecs;

        /** @brief [Default false] If true, prevents signing of events - i.e. no anti-tanpering features will be available */
        bool                                    disableSigningAndVerification;

        /** @brief [Default false] If true, recordings are automatically purged when the Engine is shut down and/or reinitialized. */
        bool                                    ephemeral;

        EnginePolicyTimelines()
        {
            clear();
        }

        void clear()
        {
            enabled = true;
            storageRoot.clear();
            maxStorageMb = 1024;                    // 1 Gigabyte
            maxEventAgeSecs = (86400 * 30);         // 30 days
            groomingIntervalSecs = (60 * 30);       // 30 minutes
            maxEvents = 1000;
            autosaveIntervalSecs = 5;
            security.clear();
            disableSigningAndVerification = false;
            ephemeral = false;
        }
    };

    static void to_json(nlohmann::json& j, const EnginePolicyTimelines& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(storageRoot),
            TOJSON_IMPL(maxStorageMb),
            TOJSON_IMPL(maxEventAgeSecs),
            TOJSON_IMPL(maxEvents),
            TOJSON_IMPL(groomingIntervalSecs),
            TOJSON_IMPL(autosaveIntervalSecs),
            TOJSON_IMPL(security),
            TOJSON_IMPL(disableSigningAndVerification),
            TOJSON_IMPL(ephemeral)
        };
    }
    static void from_json(const nlohmann::json& j, EnginePolicyTimelines& p)
    {
        p.clear();
        getOptional<bool>("enabled", p.enabled, j, true);
        getOptional<std::string>("storageRoot", p.storageRoot, j, EMPTY_STRING);

        getOptional<int>("maxStorageMb", p.maxStorageMb, j, 1024);
        getOptional<long>("maxEventAgeSecs", p.maxEventAgeSecs, j, (86400 * 30));
        getOptional<long>("groomingIntervalSecs", p.groomingIntervalSecs, j, (60 * 30));
        getOptional<long>("autosaveIntervalSecs", p.autosaveIntervalSecs, j, 5);
        getOptional<int>("maxEvents", p.maxEvents, j, 1000);
        getOptional<SecurityCertificate>("security", p.security, j);
        getOptional<bool>("disableSigningAndVerification", p.disableSigningAndVerification, j, false);
        getOptional<bool>("ephemeral", p.ephemeral, j, false);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(RtpMapEntry)
    /**
    * @brief An RTP map entry
    *
    * Helper C++ class to serialize and de-serialize RtpMapEntry JSON
    *
    * Example: @include[doc] examples/RtpMapEntry.json
    *
    * @see TODO: ConfigurationObjects::RtpMapEntry
    */
    class RtpMapEntry : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(RtpMapEntry)

    public:
        /** @brief Name of the CODEC */
        std::string         name;

        /** @brief An integer representing the codec type @see TxAudio::TxCodec_t */
        int                 engageType;

        /** @brief The RTP payload type identifier */
        int                 rtpPayloadType;

        RtpMapEntry()
        {
            clear();
        }

        void clear()
        {
            name.clear();
            engageType = -1;
            rtpPayloadType = -1;
        }
    };

    static void to_json(nlohmann::json& j, const RtpMapEntry& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(name),
            TOJSON_IMPL(engageType),
            TOJSON_IMPL(rtpPayloadType)
        };
    }
    static void from_json(const nlohmann::json& j, RtpMapEntry& p)
    {
        p.clear();
        getOptional<std::string>("name", p.name, j, EMPTY_STRING);
        getOptional<int>("engageType", p.engageType, j, -1);
        getOptional<int>("rtpPayloadType", p.rtpPayloadType, j, -1);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(ExternalModule)
    /**
    * @brief Base for a description of an external module
    *
    * Helper C++ class to serialize and de-serialize ExternalModuleBase JSON
    *
    * Example: @include[doc] examples/ExternalModuleBase.json
    *
    * @see engageInitialize, EnginePolicy
    */
    class ExternalModule : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(ExternalModule)

    public:
        /** @brief Name */
        std::string                             name;

        /** @brief File spec */
        std::string                             file;

        /** @brief Optional free-form JSON configuration to be passed to the module */
        nlohmann::json                          configuration;

        ExternalModule()
        {
            clear();
        }

        void clear()
        {
            name.clear();
            file.clear();
            configuration.clear();
        }
    };

    static void to_json(nlohmann::json& j, const ExternalModule& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(name),
            TOJSON_IMPL(file)
        };        

        if(!p.configuration.empty())
        {
            j["configuration"] = p.configuration;
        }
    }
    static void from_json(const nlohmann::json& j, ExternalModule& p)
    {
        p.clear();
        getOptional<std::string>("name", p.name, j, EMPTY_STRING);
        getOptional<std::string>("file", p.file, j, EMPTY_STRING);
        
        try
        {
            p.configuration = j.at("configuration");
        }
        catch(...)
        {
            p.configuration.clear();
        }
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(ExternalCodecDescriptor)
    /**
    * @brief Describes an external codec
    *
    * Helper C++ class to serialize and de-serialize ExternalCodecDescriptor JSON
    *
    * Example: @include[doc] examples/ExternalCodecDescriptor.json
    *
    * @see engageInitialize, EnginePolicy
    */
    class ExternalCodecDescriptor : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(ExternalCodecDescriptor)

    public:
        /** @brief RTP payload type */
        int                         rtpPayloadType;

        /** @brief Sampling rate */
        int                         samplingRate;

        /** @brief Channels */
        int                         channels;

        /** @brief RTP timestamp multiplier */
        int                         rtpTsMultiplier;

        ExternalCodecDescriptor()
        {            
            clear();
        }

        void clear()
        {
            rtpPayloadType = -1;
            samplingRate = -1;
            channels = -1;
            rtpTsMultiplier = 0;
        }
    };

    static void to_json(nlohmann::json& j, const ExternalCodecDescriptor& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(rtpPayloadType),
            TOJSON_IMPL(samplingRate),
            TOJSON_IMPL(channels),
            TOJSON_IMPL(rtpTsMultiplier)
        };
    }
    static void from_json(const nlohmann::json& j, ExternalCodecDescriptor& p)
    {
        p.clear();
                
        getOptional<int>("rtpPayloadType", p.rtpPayloadType, j, -1);
        getOptional<int>("samplingRate", p.samplingRate, j, -1);
        getOptional<int>("channels", p.channels, j, -1);
        getOptional<int>("rtpTsMultiplier", p.rtpTsMultiplier, j, -1);
    }
    
    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(EnginePolicy)
    /**
    * @brief Provides Engage Engine policy configuration.
    *
    * Helper C++ class to serialize and de-serialize EnginePolicy JSON
    *
    * Provided at Engage Engine initialization time to specify all the policy settings for the Engage Engine.
    *
    * Example JSON: @include[doc] examples/EnginePolicy.json
    *
    * @see engageInitialize
    */
    class EnginePolicy : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(EnginePolicy)

    public:

        /** @brief Specifies the root of the physical path to store data. */
        std::string                 dataDirectory;

        /** @brief Licensing settings */
        Licensing                   licensing;

        /** @brief Security settings */
        EnginePolicySecurity        security;

        /** @brief Security settings */
        EnginePolicyNetworking      networking;

        /** @brief Audio settings */
        EnginePolicyAudio           audio;

        /** @brief Discovery settings */
        DiscoveryConfiguration      discovery;

        /** @brief Logging settings */
        EnginePolicyLogging         logging;

        /** @brief Internal settings */
        EnginePolicyInternals       internals;

        /** @brief Timelines settings */
        EnginePolicyTimelines       timelines;

        /** @brief Database settings */
        EnginePolicyDatabase        database;

        /** @brief Optional feature set */
        Featureset                  featureset;

        /** @brief Optional named audio devices (Linux only) */
        EnginePolicyNamedAudioDevices namedAudioDevices;

        /** @brief Optional external codecs */
        std::vector<ExternalModule> externalCodecs;

        /** @brief Optional RTP - overrides the default */
        std::vector<RtpMapEntry>    rtpMap;

        EnginePolicy()
        {
            clear();
        }

        void clear()
        {
            dataDirectory.clear();
            licensing.clear();
            security.clear();
            networking.clear();
            audio.clear();
            discovery.clear();
            logging.clear();
            internals.clear();
            timelines.clear();
            database.clear();
            featureset.clear();
            namedAudioDevices.clear();
            externalCodecs.clear();
            rtpMap.clear();
        }
    };

    static void to_json(nlohmann::json& j, const EnginePolicy& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(dataDirectory),
            TOJSON_IMPL(licensing),
            TOJSON_IMPL(security),
            TOJSON_IMPL(networking),
            TOJSON_IMPL(audio),
            TOJSON_IMPL(discovery),
            TOJSON_IMPL(logging),
            TOJSON_IMPL(internals),
            TOJSON_IMPL(timelines),
            TOJSON_IMPL(database),
            TOJSON_IMPL(featureset),
            TOJSON_IMPL(namedAudioDevices),
            TOJSON_IMPL(externalCodecs),
            TOJSON_IMPL(rtpMap)
        };
    }
    static void from_json(const nlohmann::json& j, EnginePolicy& p)
    {
        p.clear();
        FROMJSON_IMPL_SIMPLE(dataDirectory);
        FROMJSON_IMPL_SIMPLE(licensing);
        FROMJSON_IMPL_SIMPLE(security);
        FROMJSON_IMPL_SIMPLE(networking);
        FROMJSON_IMPL_SIMPLE(audio);
        FROMJSON_IMPL_SIMPLE(discovery);
        FROMJSON_IMPL_SIMPLE(logging);
        FROMJSON_IMPL_SIMPLE(internals);
        FROMJSON_IMPL_SIMPLE(timelines);
        FROMJSON_IMPL_SIMPLE(database);
        FROMJSON_IMPL_SIMPLE(featureset);
        FROMJSON_IMPL_SIMPLE(namedAudioDevices);
        FROMJSON_IMPL_SIMPLE(externalCodecs);
        FROMJSON_IMPL_SIMPLE(rtpMap);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(TalkgroupAsset)
    /**
    * @brief TODO: Complete class
    *
    * Helper C++ class to serialize and de-serialize TalkgroupAsset JSON
    *
    * Example: @include[doc] examples/TalkgroupAsset.json
    *
    * @see TODO: Add references
    */
    class TalkgroupAsset : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(TalkgroupAsset)

    public:

        /** @brief A unique identifier for the asset */
        std::string     nodeId;

        /** @brief Details for the talkgroup */
        Group           group;

        TalkgroupAsset()
        {
            clear();
        }

        void clear()
        {
            nodeId.clear();
            group.clear();
        }
    };

    static void to_json(nlohmann::json& j, const TalkgroupAsset& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(nodeId),
            TOJSON_IMPL(group)
        };
    }
    static void from_json(const nlohmann::json& j, TalkgroupAsset& p)
    {
        p.clear();
        getOptional<std::string>("nodeId", p.nodeId, j);
        getOptional<Group>("group", p.group, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(EngageDiscoveredGroup)
    /**
    * @brief Internal RTS use
    *
    * Helper C++ class to serialize and de-serialize EngageDiscoveredGroup JSON
    *
    * Example: @include[doc] examples/EngageDiscoveredGroup.json
    */
    class EngageDiscoveredGroup : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(EngageDiscoveredGroup)

    public:
        /** @brief Internal ID. */
        std::string     id;

        /** @brief Internal type. */
        int             type;

        /** @brief Internal RX detail. */
        NetworkAddress  rx;

        /** @brief Internal TX detail. */
        NetworkAddress  tx;

        EngageDiscoveredGroup()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            type = 0;
            rx.clear();
            tx.clear();
        }
    };

    static void to_json(nlohmann::json& j, const EngageDiscoveredGroup& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(type),
            TOJSON_IMPL(rx),
            TOJSON_IMPL(tx)
        };
    }
    static void from_json(const nlohmann::json& j, EngageDiscoveredGroup& p)
    {
        p.clear();
        getOptional<std::string>("id", p.id, j);
        getOptional<int>("type", p.type, j, 0);
        getOptional<NetworkAddress>("rx", p.rx, j);
        getOptional<NetworkAddress>("tx", p.tx, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(RallypointPeer)
    /**
    * @brief RTS internal use
    *
    * Helper C++ class to serialize and de-serialize RallypointPeer JSON
    *
    * Example: @include[doc] examples/RallypointPeer.json
    *
    * @see RallypointServer
    */
    class RallypointPeer : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(RallypointPeer)

    public:
        /** @brief Internal ID. */
        std::string             id;

        /** @brief Internal enablement setting. */
        bool                    enabled;

        /** @brief Internal host detail. */
        NetworkAddress          host;

        /** @brief Internal certificate detail. */
        SecurityCertificate     certificate;

        /** @brief [Optional, Default: 0 - OS platform default] Connection timeout in seconds to the peer */
        int                      connectionTimeoutSecs;

        RallypointPeer()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            enabled = true;
            host.clear();
            certificate.clear();
            connectionTimeoutSecs = 0;
        }
    };

    static void to_json(nlohmann::json& j, const RallypointPeer& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(host),
            TOJSON_IMPL(certificate),
            TOJSON_IMPL(connectionTimeoutSecs)
        };
    }
    static void from_json(const nlohmann::json& j, RallypointPeer& p)
    {
        p.clear();
        j.at("id").get_to(p.id);
        getOptional<bool>("enabled", p.enabled, j, true);
        getOptional<NetworkAddress>("host", p.host, j);
        getOptional<SecurityCertificate>("certificate", p.certificate, j);
        getOptional<int>("connectionTimeoutSecs", p.connectionTimeoutSecs, j, 0);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(RallypointServerLimits)
    /**
    * @brief TODO: Configuration for Rallypoint limits
    *
    * Helper C++ class to serialize and de-serialize RallypointServerLimits JSON
    *
    * Example: @include[doc] examples/RallypointServerLimits.json
    *
    * @see RallypointServer
    */
    class RallypointServerLimits : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(RallypointServerLimits)

    public:
        /** @brief Maximum number of clients (0 = unlimited) */
        uint32_t            maxClients;

        /** @brief Maximum number of peers (0 = unlimited) */
        uint32_t            maxPeers;

        /** @brief Maximum number of multicastReflectors (0 = unlimited) */
        uint32_t            maxMulticastReflectors;

        /** @brief Maximum number of registered streams (0 = unlimited) */
        uint32_t            maxRegisteredStreams;

        /** @brief Maximum number of bidirectional stream paths (0 = unlimited) */
        uint32_t            maxStreamPaths;

        /** @brief Maximum number of packets received per second (0 = unlimited) */
        uint32_t            maxRxPacketsPerSec;

        /** @brief Maximum number of packets transmitted per second (0 = unlimited) */
        uint32_t            maxTxPacketsPerSec;

        /** @brief Maximum number of bytes received per second (0 = unlimited) */
        uint32_t            maxRxBytesPerSec;

        /** @brief Maximum number of bytes transmitted per second (0 = unlimited) */
        uint32_t            maxTxBytesPerSec;

        /** @brief Maximum number of queue operations per second (0 = unlimited) */
        uint32_t            maxQOpsPerSec;

        /** @brief Maximum number of inbound backlog requests the Rallypoint will accept */
        uint32_t            maxInboundBacklog;

        /** @brief Number of low priority queue operations after which new connections will not be accepted */
        uint32_t            lowPriorityQueueThreshold;

        /** @brief Number of normal priority queue operations after which new connections will not be accepted */
        uint32_t            normalPriorityQueueThreshold;

        /** @brief The CPU utilization threshold percentage (0-100) beyond which new connections are denied */
        uint32_t            denyNewConnectionCpuThreshold;

        /** @brief The CPU utilization threshold percentage (0-100) beyond which warnings are logged */
        uint32_t            warnAtCpuThreshold;

        RallypointServerLimits()
        {
            clear();
        }

        void clear()
        {
            maxClients = 0;
            maxPeers = 0;
            maxMulticastReflectors = 0;
            maxRegisteredStreams = 0;
            maxStreamPaths = 0;
            maxRxPacketsPerSec = 0;
            maxTxPacketsPerSec = 0;
            maxRxBytesPerSec = 0;
            maxTxBytesPerSec = 0;
            maxQOpsPerSec = 0;
            maxInboundBacklog = 64;
            lowPriorityQueueThreshold = 64;
            normalPriorityQueueThreshold = 256;
            denyNewConnectionCpuThreshold = 75;
            warnAtCpuThreshold = 65;
        }
    };

    static void to_json(nlohmann::json& j, const RallypointServerLimits& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(maxClients),
            TOJSON_IMPL(maxPeers),
            TOJSON_IMPL(maxMulticastReflectors),
            TOJSON_IMPL(maxRegisteredStreams),
            TOJSON_IMPL(maxStreamPaths),
            TOJSON_IMPL(maxRxPacketsPerSec),
            TOJSON_IMPL(maxTxPacketsPerSec),
            TOJSON_IMPL(maxRxBytesPerSec),
            TOJSON_IMPL(maxTxBytesPerSec),
            TOJSON_IMPL(maxQOpsPerSec),
            TOJSON_IMPL(maxInboundBacklog),
            TOJSON_IMPL(lowPriorityQueueThreshold),
            TOJSON_IMPL(normalPriorityQueueThreshold),
            TOJSON_IMPL(denyNewConnectionCpuThreshold),
            TOJSON_IMPL(warnAtCpuThreshold)
        };
    }
    static void from_json(const nlohmann::json& j, RallypointServerLimits& p)
    {
        p.clear();
        getOptional<uint32_t>("maxClients", p.maxClients, j, 0);
        getOptional<uint32_t>("maxPeers", p.maxPeers, j, 0);
        getOptional<uint32_t>("maxMulticastReflectors", p.maxMulticastReflectors, j, 0);
        getOptional<uint32_t>("maxRegisteredStreams", p.maxRegisteredStreams, j, 0);
        getOptional<uint32_t>("maxStreamPaths", p.maxStreamPaths, j, 0);
        getOptional<uint32_t>("maxRxPacketsPerSec", p.maxRxPacketsPerSec, j, 0);
        getOptional<uint32_t>("maxTxPacketsPerSec", p.maxTxPacketsPerSec, j, 0);
        getOptional<uint32_t>("maxRxBytesPerSec", p.maxRxBytesPerSec, j, 0);
        getOptional<uint32_t>("maxTxBytesPerSec", p.maxTxBytesPerSec, j, 0);
        getOptional<uint32_t>("maxQOpsPerSec", p.maxQOpsPerSec, j, 0);
        getOptional<uint32_t>("maxInboundBacklog", p.maxInboundBacklog, j, 64);
        getOptional<uint32_t>("lowPriorityQueueThreshold", p.lowPriorityQueueThreshold, j, 64);
        getOptional<uint32_t>("normalPriorityQueueThreshold", p.normalPriorityQueueThreshold, j, 256);
        getOptional<uint32_t>("denyNewConnectionCpuThreshold", p.denyNewConnectionCpuThreshold, j, 75);
        getOptional<uint32_t>("warnAtCpuThreshold", p.warnAtCpuThreshold, j, 65);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(RallypointServerStatusReportConfiguration)
    /**
    * @brief TODO: Configuration for the Rallypoint status report file
    *
    * Helper C++ class to serialize and de-serialize RallypointServerStatusReportConfiguration JSON
    *
    * Example: @include[doc] examples/RallypointServerStatusReportConfiguration.json
    *
    * @see RallypointServer
    */
    class RallypointServerStatusReportConfiguration : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(RallypointServerStatusReportConfiguration)

    public:
        /** File name to use for the status report. */
        std::string                     fileName;

        /** [Optional, Default: 30] The interval at which to write out the status report to file. */
        int                             intervalSecs;

        /** [Optional, Default: false] Indicates if status reporting is enabled. */
        bool                            enabled;

        /** [Optional, Default: false] Indicates whether summarized link information is to be included. */
        bool                            includeLinks;

        /** [Optional, Default: false] Indicates whether details of peer links are to be included. */
        bool                            includePeerLinkDetails;

        /** [Optional, Default: false] Indicates whether details of client links are to be included. */
        bool                            includeClientLinkDetails;

        /** [Optional, Default: null] Command to be executed every time the status report is produced. */
        std::string                     runCmd;

        RallypointServerStatusReportConfiguration()
        {
            clear();
        }

        void clear()
        {
            fileName.clear();
            intervalSecs = 60;
            enabled = false;
            includeLinks = false;
            includePeerLinkDetails = false;
            includeClientLinkDetails = false;
            runCmd.clear();
        }
    };

    static void to_json(nlohmann::json& j, const RallypointServerStatusReportConfiguration& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(fileName),
            TOJSON_IMPL(intervalSecs),
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(includeLinks),
            TOJSON_IMPL(includePeerLinkDetails),
            TOJSON_IMPL(includeClientLinkDetails),
            TOJSON_IMPL(runCmd)
        };
    }
    static void from_json(const nlohmann::json& j, RallypointServerStatusReportConfiguration& p)
    {
        p.clear();
        getOptional<std::string>("fileName", p.fileName, j);
        getOptional<int>("intervalSecs", p.intervalSecs, j, 60);
        getOptional<bool>("enabled", p.enabled, j, false);
        getOptional<bool>("includeLinks", p.includeLinks, j, false);
        getOptional<bool>("includePeerLinkDetails", p.includePeerLinkDetails, j, false);
        getOptional<bool>("includeClientLinkDetails", p.includeClientLinkDetails, j, false);
        getOptional<std::string>("runCmd", p.runCmd, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(RallypointServerLinkGraph)
    class RallypointServerLinkGraph : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(RallypointServerLinkGraph)

    public:
        /** File name to use for the status report. */
        std::string                     fileName;

        /** [Optional, Default: 5] Minimum update time for link graph updates. */
        int                             minRefreshSecs;

        /** [Optional, Default: false] Indicates if link reporting is enabled. */
        bool                            enabled;

        /** [Optional, Default: true] Indicates if the output should be enclosed in a digraph. */
        bool                            includeDigraphEnclosure;

        /** [Optional, Default: false] Indicates if client links should be incldued. */

        /** [Optional] GraphViz styling for nodes representing client. */
        bool                            includeClients;

        /** [Optional] GraphViz styling for nodes representing core Rallypoint peers. */
        std::string                     coreRpStyling;

        /** [Optional] GraphViz styling for nodes representing leaf Rallypoint peers. */
        std::string                     leafRpStyling;

        /** [Optional] GraphViz styling for nodes representing clients. */
        std::string                     clientStyling;

        /** [Optional, Default: null] Command to be executed every time the link graph is produced. */
        std::string                     runCmd;

        RallypointServerLinkGraph()
        {
            clear();
        }

        void clear()
        {
            fileName.clear();
            minRefreshSecs = 5;
            enabled = false;
            includeDigraphEnclosure = true;
            includeClients = false;
            coreRpStyling = "[shape=hexagon color=firebrick style=filled]";
            leafRpStyling = "[shape=box color=gray style=filled]";
            clientStyling.clear();
            runCmd.clear();
        }
    };

    static void to_json(nlohmann::json& j, const RallypointServerLinkGraph& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(fileName),
            TOJSON_IMPL(minRefreshSecs),
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(includeDigraphEnclosure),
            TOJSON_IMPL(includeClients),
            TOJSON_IMPL(coreRpStyling),
            TOJSON_IMPL(leafRpStyling),
            TOJSON_IMPL(clientStyling),
            TOJSON_IMPL(runCmd)
        };
    }
    static void from_json(const nlohmann::json& j, RallypointServerLinkGraph& p)
    {
        p.clear();
        getOptional<std::string>("fileName", p.fileName, j);
        getOptional<int>("minRefreshSecs", p.minRefreshSecs, j, 5);
        getOptional<bool>("enabled", p.enabled, j, false);
        getOptional<bool>("includeDigraphEnclosure", p.includeDigraphEnclosure, j, true);
        getOptional<bool>("includeClients", p.includeClients, j, false);
        getOptional<std::string>("coreRpStyling", p.coreRpStyling, j, "[shape=hexagon color=firebrick style=filled]");
        getOptional<std::string>("leafRpStyling", p.leafRpStyling, j, "[shape=box color=gray style=filled]");
        getOptional<std::string>("clientStyling", p.clientStyling, j);
        getOptional<std::string>("runCmd", p.runCmd, j);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(RallypointServerRouteMap)
    class RallypointServerRouteMap : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(RallypointServerRouteMap)

    public:
        /** File name to use for the route map report. */
        std::string                     fileName;

        /** [Optional, Default: 5] Minimum update time for updates. */
        int                             minRefreshSecs;

        /** [Optional, Default: false] Indicates if reporting is enabled. */
        bool                            enabled;

        /** [Optional, Default: null] Command to be executed every time the report is produced. */
        std::string                     runCmd;

        RallypointServerRouteMap()
        {
            clear();
        }

        void clear()
        {
            fileName.clear();
            minRefreshSecs = 5;
            enabled = false;
        }
    };

    static void to_json(nlohmann::json& j, const RallypointServerRouteMap& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(fileName),
            TOJSON_IMPL(minRefreshSecs),
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(runCmd)
        };
    }
    static void from_json(const nlohmann::json& j, RallypointServerRouteMap& p)
    {
        p.clear();
        getOptional<std::string>("fileName", p.fileName, j);
        getOptional<int>("minRefreshSecs", p.minRefreshSecs, j, 5);
        getOptional<bool>("enabled", p.enabled, j, false);
        getOptional<std::string>("runCmd", p.runCmd, j);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(ExternalHealthCheckResponder)
    /**
    * @brief TODO: Configuration to enable external systems to use to check if the service is still running.
    *
    * Helper C++ class to serialize and de-serialize ExternalHealthCheckResponder JSON
    *
    * Example: @include[doc] examples/ExternalHealthCheckResponder.json
    *
    * @see RallypointServer
    */
    class ExternalHealthCheckResponder : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(ExternalHealthCheckResponder)

    public:

        /** The network port to listen on for connections */
        int                             listenPort;

        /** [Optional, Default: true] If true, closes the inbound socket connection immediately. */
        bool                            immediateClose;

        ExternalHealthCheckResponder()
        {
            clear();
        }

        void clear()
        {
            listenPort = 0;
            immediateClose = true;
        }
    };

    static void to_json(nlohmann::json& j, const ExternalHealthCheckResponder& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(listenPort),
            TOJSON_IMPL(immediateClose)
        };
    }
    static void from_json(const nlohmann::json& j, ExternalHealthCheckResponder& p)
    {
        p.clear();
        getOptional<int>("listenPort", p.listenPort, j, 0);
        getOptional<bool>("immediateClose", p.immediateClose, j, true);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(PeeringConfiguration)
    /**
    * @brief Configuration for Rallypoint peers
    *
    * Helper C++ class to serialize and de-serialize PeeringConfiguration JSON
    *
    * @see RallypointServer
    */
    class PeeringConfiguration : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(PeeringConfiguration)

    public:

        /** @brief An identifier useful for organizations that track different mesh configurations by ID */
        std::string                     id;

        /** @brief TODO: A version number for the mesh configuration.  Change this whenever you update your configuration */
        int                             version;

        /** @brief Comments */
        std::string                     comments;

        /** @brief List of Rallypoint peers to connect to */
        std::vector<RallypointPeer>     peers;

        PeeringConfiguration()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            version = 0;
            comments.clear();
        }
    };

    static void to_json(nlohmann::json& j, const PeeringConfiguration& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(version),
            TOJSON_IMPL(comments),
            TOJSON_IMPL(peers)
        };
    }
    static void from_json(const nlohmann::json& j, PeeringConfiguration& p)
    {
        p.clear();
        getOptional<std::string>("id", p.id, j);
        getOptional<int>("version", p.version, j, 0);
        getOptional<std::string>("comments", p.comments, j);
        getOptional<std::vector<RallypointPeer>>("peers", p.peers, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(IgmpSnooping)
    /**
    * @brief Configuration for IGMP snooping
    *
    * Helper C++ class to serialize and de-serialize IgmpSnooping JSON
    *
    * @see RallypointServer
    */
    class IgmpSnooping : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(IgmpSnooping)

    public:

        /** @brief Enables IGMP.  Default is false. */
        bool                            enabled;

        /** @brief TODO */
        int                             queryIntervalMs;

        /** @brief TODO */
        int                             lastMemberQueryIntervalMs;

        /** @brief TODO */
        int                             lastMemberQueryCount;

        IgmpSnooping()
        {
            clear();
        }

        void clear()
        {
            enabled = false;
            queryIntervalMs = 60000;
            lastMemberQueryIntervalMs = 1000;
            lastMemberQueryCount = 1;
        }
    };

    static void to_json(nlohmann::json& j, const IgmpSnooping& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(queryIntervalMs),
            TOJSON_IMPL(lastMemberQueryIntervalMs),
            TOJSON_IMPL(lastMemberQueryCount)
        };
    }
    static void from_json(const nlohmann::json& j, IgmpSnooping& p)
    {
        p.clear();
        getOptional<bool>("enabled", p.enabled, j);
        getOptional<int>("queryIntervalMs", p.queryIntervalMs, j, 60000);
        getOptional<int>("lastMemberQueryIntervalMs", p.lastMemberQueryIntervalMs, j, 1000);
        getOptional<int>("lastMemberQueryCount", p.lastMemberQueryCount, j, 1);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(RallypointReflector)
    /**
     * @brief Definition of a static group for Rallypoints
     *
     * Example: @include[doc] examples/RallypointReflector.json
     *
     */
    class RallypointReflector : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(RallypointReflector)

    public:
        /**
         * @brief Unique identity for the group.
         */
        std::string                             id;

        /** @brief The network address for receiving network traffic on. */
        NetworkAddress                          rx;

        /** @brief The network address for transmitting network traffic to. */
        NetworkAddress                          tx;

        /** @brief [Optional] The name of the NIC on which to send and receive multicast traffic. */
        std::string                             multicastInterfaceName;

        RallypointReflector()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            rx.clear();
            tx.clear();
            multicastInterfaceName.clear();
        }
    };

    static void to_json(nlohmann::json& j, const RallypointReflector& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(rx),
            TOJSON_IMPL(tx),
            TOJSON_IMPL(multicastInterfaceName)
        };
    }
    static void from_json(const nlohmann::json& j, RallypointReflector& p)
    {
        p.clear();
        j.at("id").get_to(p.id);
        j.at("rx").get_to(p.rx);
        j.at("tx").get_to(p.tx);
        getOptional<std::string>("multicastInterfaceName", p.multicastInterfaceName, j);
    }    


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(RallypointUdpStreaming)
    /**
     * @brief Streaming configuration for RP clients
     *
     * Example: @include[doc] examples/RallypointUdpStreaming.json
     *
     */
    class RallypointUdpStreaming : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(RallypointUdpStreaming)

    public:
        /** @brief Enum describing UDP streaming modes. */
        typedef enum
        {
            /** @brief Unknown */
            etUnknown,

            /** @brief Shared-key encryption */
            etSharedKey
        } EnvelopeType_t;

        /** @brief Specifies the streaming mode type (see @ref EnvelopeType_t). */
        EnvelopeType_t                          envelopeType;

        /** @brief Network address for listening */
        NetworkAddress                          listen;

        /** @brief Network address for external entities to transmit to */
        NetworkAddress                          external;

        RallypointUdpStreaming()
        {
            clear();
        }

        void clear()
        {
            envelopeType = etUnknown;
            listen.clear();
            external.clear();
        }
    };

    static void to_json(nlohmann::json& j, const RallypointUdpStreaming& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(envelopeType),
            TOJSON_IMPL(listen),
            TOJSON_IMPL(external)
        };
    }
    static void from_json(const nlohmann::json& j, RallypointUdpStreaming& p)
    {
        p.clear();
        j.at("envelopeType").get_to(p.envelopeType);
        j.at("listen").get_to(p.listen);
        j.at("external").get_to(p.external);
    }    

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(RallypointRpRtTimingBehavior)
    /**
     * @brief Defines a behavior for a Rallypoint peer roundtrip time
     *
     * Example: @include[doc] examples/RallypointRpRtTimingBehavior.json
     *
     */
    class RallypointRpRtTimingBehavior : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(RallypointRpRtTimingBehavior)

    public:
        /** @brief Enum describing behavior types. */
        typedef enum
        {
            /** @brief Unknown */
            btNone,

            /** @brief Report at level info */
            btReportInfo,

            /** @brief Report at level warning */
            btReportWarn,

            /** @brief Report at level error */
            btReportError,  

            /** @brief Report at level fatal and drop connection */
            btDrop = 99        
        } BehaviorType_t;

        /** @brief Specifies the streaming mode type (see @ref BehaviorType_t). */
        BehaviorType_t                          behavior;

        /** @brief Network address for listening */
        uint32_t                                atOrAboveMs;

        /** [Optional, Default: null] Command to be executed. */
        std::string                             runCmd;

        RallypointRpRtTimingBehavior()
        {
            clear();
        }

        void clear()
        {
            behavior = btNone;
            atOrAboveMs = 0;
            runCmd.clear();
        }
    };

    static void to_json(nlohmann::json& j, const RallypointRpRtTimingBehavior& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(behavior),
            TOJSON_IMPL(atOrAboveMs),
            TOJSON_IMPL(runCmd)
        };
    }
    static void from_json(const nlohmann::json& j, RallypointRpRtTimingBehavior& p)
    {
        p.clear();
        getOptional<RallypointRpRtTimingBehavior::BehaviorType_t>("behavior", p.behavior, j, RallypointRpRtTimingBehavior::BehaviorType_t::btNone);
        getOptional<uint32_t>("atOrAboveMs", p.atOrAboveMs, j, 0);
        getOptional<std::string>("runCmd", p.runCmd, j);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(RallypointWebsocketSettings)
    /**
     * @brief Defines settings for Rallypoint websockets functionality
     *
     * Example: @include[doc] examples/RallypointWebsocketSettings.json
     *
     */
    class RallypointWebsocketSettings : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(RallypointWebsocketSettings)

    public:
        /** @brief [Default: false] Websocket is enabled */
        bool                                    enabled;

        /** @brief Listen port (TCP).  Default is 8443 */
        int                                     listenPort;

        /** @brief Certificate to be used for WebSockets */
        SecurityCertificate                     certificate;

        RallypointWebsocketSettings()
        {
            clear();
        }

        void clear()
        {
            enabled = false;
            listenPort = 8443;
            certificate.clear();
        }
    };

    static void to_json(nlohmann::json& j, const RallypointWebsocketSettings& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(listenPort),
            TOJSON_IMPL(certificate)
        };
    }
    static void from_json(const nlohmann::json& j, RallypointWebsocketSettings& p)
    {
        p.clear();
        getOptional<bool>("enabled", p.enabled, j, false);
        getOptional<int>("listenPort", p.listenPort, j, 8443);
        getOptional<SecurityCertificate>("certificate", p.certificate, j);
    }



    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(RallypointAdvertisingSettings)
    /**
     * @brief Defines settings for Rallypoint advertising
     *
     * Example: @include[doc] examples/RallypointAdvertisingSettings.json
     *
     */
    class RallypointAdvertisingSettings : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(RallypointAdvertisingSettings)

    public:
        /** @brief [Default: false] Advertising is enabled */
        bool                                    enabled;

        /** @brief [Optional] This Rallypoint's mesh name */
        std::string                             meshName;

        /** @brief [Optional] This Rallypoint's DNS-SD host name */
        std::string                             hostName;

        /** @brief [Optional, Default "_rallypoint._tcp.local."] The service name */
        std::string                             serviceName;

        /** @brief The multicast network interface for mDNS */
        std::string                             interfaceName;

        /** @brief [Default: RP port] The multicast network interface for mDNS */
        int                                     port;

        /** @brief [Default: 60] TTL for service TTL */
        int                                     ttl;

        /** @brief [Optional] List of additional meshes that can be reached via this RP */
        std::vector<std::string>                extraMeshes;


        RallypointAdvertisingSettings()
        {
            clear();
        }

        void clear()
        {
            enabled = false;
            meshName.clear();
            hostName.clear();
            serviceName = "_rallypoint._tcp.local.";
            interfaceName.clear();
            port = 0;
            ttl = 60;
            extraMeshes.clear();
        }
    };

    static void to_json(nlohmann::json& j, const RallypointAdvertisingSettings& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(meshName),
            TOJSON_IMPL(hostName),
            TOJSON_IMPL(serviceName),
            TOJSON_IMPL(interfaceName),
            TOJSON_IMPL(port),
            TOJSON_IMPL(ttl),
            TOJSON_IMPL(extraMeshes)
        };
    }
    static void from_json(const nlohmann::json& j, RallypointAdvertisingSettings& p)
    {
        p.clear();
        getOptional<bool>("enabled", p.enabled, j, false);
        getOptional<std::string>("meshName", p.meshName, j);
        getOptional<std::string>("hostName", p.hostName, j);
        getOptional<std::string>("serviceName", p.serviceName, j, "_rallypoint._tcp.local.");
        getOptional<std::string>("interfaceName", p.interfaceName, j);

        getOptional<int>("port", p.port, j, 0);
        getOptional<int>("ttl", p.ttl, j, 60);
        getOptional<std::vector<std::string>>("extraMeshes", p.extraMeshes, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(RallypointServer)
    /**
    * @brief Configuration for the Rallypoint server
    *
    * Helper C++ class to serialize and de-serialize RallypointServer JSON
    *
    * Example: @include[doc] examples/RallypointServer.json
    *
    */
    class RallypointServer : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(RallypointServer)

    public:
        /** @brief [Optional] Settings for the FIPS crypto. */
        FipsCryptoSettings                          fipsCrypto;
    
        /** @brief [Optional] Settings for the Rallypoint's watchdog. */
        WatchdogSettings                            watchdog;

        /** @brief A unqiue identifier for the Rallypoint */
        std::string                                 id;

        /** @brief TCP port to listen on.  Default is 7443. */
        int                                         listenPort;

        /** @brief Name of the NIC to bind to for listening for incoming TCP connections. */
        std::string                                 interfaceName;

        /** @brief X.509 certificate and private key that identifies the Rallypoint.  @see SecurityCertificate*/
        SecurityCertificate                         certificate;

        /** @brief Name of a file containing a JSON array of Rallypoint peers to connect to. */
        std::string                                 peeringConfigurationFileName;

        /** @brief Command-line to execute that returns a JSON array of Rallypoint peers to connect to. */
        std::string                                 peeringConfigurationFileCommand;

        /** @brief Number of seconds between checks to see if the peering configuration has been updated.  Default is 60.*/
        int                                         peeringConfigurationFileCheckSecs;

        /** @brief Allows traffic received on unicast links to be forwarded to the multicast network. */
        bool                                        allowMulticastForwarding;

        /** @brief Number of threading pools to create for network I/O.  Default is -1 which creates 1 I/O pool per CPU core. */
        int                                         ioPools;

        /** @brief Details for producing a status report. @see RallypointServerStatusReportConfiguration */
        RallypointServerStatusReportConfiguration   statusReport;

        /** @brief Details for capacity limits and determining processing load. @see RallypointServerLimits */
        RallypointServerLimits                      limits;

        /** @brief Details for producing a Graphviz-compatible link graph. @see RallypointServerLinkGraph */
        RallypointServerLinkGraph                   linkGraph;

        /** @brief Details concerning the Rallypoint's interaction with an external health-checker such as a load-balancer.  @see ExternalHealthCheckResponder */
        ExternalHealthCheckResponder                externalHealthCheckResponder;

        /** @brief Set to true to allow forwarding of packets received from other Rallypoints to all other Rallypoints.  *WARNING* Be exceptionally careful when enabling this capability! */
        bool                                        allowPeerForwarding;

        /** @brief The name of the NIC on which to send and receive multicast traffic. */
        std::string                                 multicastInterfaceName;

        /** @brief Details concerning Transport Layer Security.  @see Tls */
        Tls                                         tls;

        /** @brief Details discovery capabilities.  @see DiscoveryConfiguration */
        DiscoveryConfiguration                      discovery;

        /** @brief Enables automatic forwarding of discovered multicast traffic to peer Rallypoints. */
        bool                                        forwardDiscoveredGroups;

        /** @brief Enables forwarding of multicast addressing to peer Rallypoints. */
        bool                                        forwardMulticastAddressing;

        /** @brief Internal - not serialized. */
        PeeringConfiguration                        peeringConfiguration;       // NOTE: This is NOT serialized

        /** @brief Indicates whether this Rallypoint is part of a core mesh or hangs off the periphery as a leaf node. */
        bool                                        isMeshLeaf;

        /** @brief Set to true to forgo DSA signing of messages.  Doing so is is a security risk but can be useful on CPU-constrained systems on already-secure environments. */
        bool                                        disableMessageSigning;

        /** @brief Multicasts to be restricted (inclusive or exclusive) */
        NetworkAddressRestrictionList               multicastRestrictions;

        /** @brief IGMP snooping configuration. */
        IgmpSnooping                                igmpSnooping;

        /** @brief Vector of static groups. */
        std::vector<RallypointReflector>            staticReflectors;

        /** @brief Tx options for TCP. */
        TcpNetworkTxOptions                         tcpTxOptions;

        /** @brief Tx options for multicast. */
        NetworkTxOptions                            multicastTxOptions;

        /** @brief Path to the certificate store */
        std::string                                 certStoreFileName;

        /** @brief Hex password for the certificate store (if any) */
        std::string                                 certStorePasswordHex;

        /** @brief Group IDs to be restricted (inclusive or exclusive) */
        StringRestrictionList                       groupRestrictions;

        /** @brief Name to use for signalling a configuration check */
        std::string                                 configurationCheckSignalName;

        /** @brief Licensing settings */
        Licensing                                   licensing;

        /** @brief Optional feature set */
        Featureset                                  featureset;

        /** @brief Optional configuration for high-performance UDP streaming */
        RallypointUdpStreaming                      udpStreaming;

        /** @brief [Optional, Default 0] Internal system flags */
        uint32_t                                    sysFlags;

        /** @brief [Optional, Default 0] Sets the queue's normal task bias */
        uint32_t                                    normalTaskQueueBias;

        /** @brief If enabled, causes a mesh leaf to reverse-subscribe to a core node upon the core subscribing and a reflector having been setup */
        bool                                        enableLeafReflectionReverseSubscription;

        /** @brief If true, turns off loop detection. */
        bool                                        disableLoopDetection;

        /** @brief [Optional, Default 0] Sets the maximum item security level that can be registered with the RP */
        uint32_t                                    maxSecurityLevel;

        /** @brief Details for producing a report containing the route map. @see RallypointServerRouteMap */
        RallypointServerRouteMap                    routeMap;

        /** @brief [Optional, Default 15] Sets the delta value for the maximum number of seconds to delay when attempting outbound peer connections */
        uint32_t                                    maxOutboundPeerConnectionIntervalDeltaSecs;

        /** @brief [Optional, Default: 60000] Milliseconds between sending round-trip test requests to peers */
        int                                         peerRtTestIntervalMs;

        /** @brief [Optional] Array of behaviors for roundtrip times to peers */
        std::vector<RallypointRpRtTimingBehavior>   peerRtBehaviors;

        /** @brief [Optional] Settings for websocket operation */
        RallypointWebsocketSettings                 websocket;

        /** @brief [Optional] Settings for NSM. */
        NsmConfiguration                            nsm;

        /** @brief [Optional] Settings for advertising. */
        RallypointAdvertisingSettings               advertising;

        RallypointServer()
        {
            clear();
        }

        void clear()
        {
            fipsCrypto.clear();
            watchdog.clear();
            id.clear();
            listenPort = 7443;
            interfaceName.clear();
            certificate.clear();
            allowMulticastForwarding = false;
            peeringConfiguration.clear();
            peeringConfigurationFileName.clear();
            peeringConfigurationFileCommand.clear();
            peeringConfigurationFileCheckSecs = 60;
            ioPools = -1;
            statusReport.clear();
            limits.clear();
            linkGraph.clear();
            externalHealthCheckResponder.clear();
            allowPeerForwarding = false;
            multicastInterfaceName.clear();
            tls.clear();
            discovery.clear();
            forwardDiscoveredGroups = false;
            forwardMulticastAddressing = false;
            isMeshLeaf = false;
            disableMessageSigning = false;
            multicastRestrictions.clear();
            igmpSnooping.clear();
            staticReflectors.clear();
            tcpTxOptions.clear();
            multicastTxOptions.clear();
            certStoreFileName.clear();
            certStorePasswordHex.clear();
            groupRestrictions.clear();
            configurationCheckSignalName = "rts.7b392d1.${id}";
            licensing.clear();
            featureset.clear();
            udpStreaming.clear();
            sysFlags = 0;
            normalTaskQueueBias = 0;
            enableLeafReflectionReverseSubscription = false;
            disableLoopDetection = false;
            maxSecurityLevel = 0;
            routeMap.clear();
            maxOutboundPeerConnectionIntervalDeltaSecs = 15;
            peerRtTestIntervalMs = 60000;
            peerRtBehaviors.clear();
            websocket.clear();
            nsm.clear();
            advertising.clear();
        }
    };

    static void to_json(nlohmann::json& j, const RallypointServer& p)
    {
        j = nlohmann::json{            
            TOJSON_IMPL(fipsCrypto),
            TOJSON_IMPL(watchdog),
            TOJSON_IMPL(id),
            TOJSON_IMPL(listenPort),
            TOJSON_IMPL(interfaceName),
            TOJSON_IMPL(certificate),
            TOJSON_IMPL(allowMulticastForwarding),
            // TOJSON_IMPL(peeringConfiguration),               // NOTE: Not serialized!
            TOJSON_IMPL(peeringConfigurationFileName),
            TOJSON_IMPL(peeringConfigurationFileCommand),
            TOJSON_IMPL(peeringConfigurationFileCheckSecs),
            TOJSON_IMPL(ioPools),
            TOJSON_IMPL(statusReport),
            TOJSON_IMPL(limits),
            TOJSON_IMPL(linkGraph),
            TOJSON_IMPL(externalHealthCheckResponder),
            TOJSON_IMPL(allowPeerForwarding),
            TOJSON_IMPL(multicastInterfaceName),
            TOJSON_IMPL(tls),
            TOJSON_IMPL(discovery),
            TOJSON_IMPL(forwardDiscoveredGroups),
            TOJSON_IMPL(forwardMulticastAddressing),
            TOJSON_IMPL(isMeshLeaf),
            TOJSON_IMPL(disableMessageSigning),
            TOJSON_IMPL(multicastRestrictions),
            TOJSON_IMPL(igmpSnooping),
            TOJSON_IMPL(staticReflectors),
            TOJSON_IMPL(tcpTxOptions),
            TOJSON_IMPL(multicastTxOptions),
            TOJSON_IMPL(certStoreFileName),
            TOJSON_IMPL(certStorePasswordHex),
            TOJSON_IMPL(groupRestrictions),
            TOJSON_IMPL(configurationCheckSignalName),
            TOJSON_IMPL(featureset),
            TOJSON_IMPL(licensing),
            TOJSON_IMPL(udpStreaming),
            TOJSON_IMPL(sysFlags),
            TOJSON_IMPL(normalTaskQueueBias),
            TOJSON_IMPL(enableLeafReflectionReverseSubscription),
            TOJSON_IMPL(disableLoopDetection),
            TOJSON_IMPL(maxSecurityLevel),
            TOJSON_IMPL(routeMap),
            TOJSON_IMPL(maxOutboundPeerConnectionIntervalDeltaSecs),
            TOJSON_IMPL(peerRtTestIntervalMs),
            TOJSON_IMPL(peerRtBehaviors),
            TOJSON_IMPL(websocket),
            TOJSON_IMPL(nsm),
            TOJSON_IMPL(advertising)
        };
    }
    static void from_json(const nlohmann::json& j, RallypointServer& p)
    {
        p.clear();
        getOptional<FipsCryptoSettings>("fipsCrypto", p.fipsCrypto, j);
        getOptional<WatchdogSettings>("watchdog", p.watchdog, j);
        getOptional<std::string>("id", p.id, j);
        getOptional<SecurityCertificate>("certificate", p.certificate, j);
        getOptional<std::string>("interfaceName", p.interfaceName, j);
        getOptional<int>("listenPort", p.listenPort, j, 7443);
        getOptional<bool>("allowMulticastForwarding", p.allowMulticastForwarding, j, false);
        //getOptional<PeeringConfiguration>("peeringConfiguration", p.peeringConfiguration, j);         // NOTE: Not serialized!
        getOptional<std::string>("peeringConfigurationFileName", p.peeringConfigurationFileName, j);
        getOptional<std::string>("peeringConfigurationFileCommand", p.peeringConfigurationFileCommand, j);
        getOptional<int>("peeringConfigurationFileCheckSecs", p.peeringConfigurationFileCheckSecs, j, 60);
        getOptional<int>("ioPools", p.ioPools, j, -1);
        getOptional<RallypointServerStatusReportConfiguration>("statusReport", p.statusReport, j);
        getOptional<RallypointServerLimits>("limits", p.limits, j);
        getOptional<RallypointServerLinkGraph>("linkGraph", p.linkGraph, j);
        getOptional<ExternalHealthCheckResponder>("externalHealthCheckResponder", p.externalHealthCheckResponder, j);
        getOptional<bool>("allowPeerForwarding", p.allowPeerForwarding, j, false);
        getOptional<std::string>("multicastInterfaceName", p.multicastInterfaceName, j);
        getOptional<Tls>("tls", p.tls, j);
        getOptional<DiscoveryConfiguration>("discovery", p.discovery, j);        
        getOptional<bool>("forwardDiscoveredGroups", p.forwardDiscoveredGroups, j, false);
        getOptional<bool>("forwardMulticastAddressing", p.forwardMulticastAddressing, j, false);
        getOptional<bool>("isMeshLeaf", p.isMeshLeaf, j, false);
        getOptional<bool>("disableMessageSigning", p.disableMessageSigning, j, false);
        getOptional<NetworkAddressRestrictionList>("multicastRestrictions", p.multicastRestrictions, j);
        getOptional<IgmpSnooping>("igmpSnooping", p.igmpSnooping, j);
        getOptional<std::vector<RallypointReflector>>("staticReflectors", p.staticReflectors, j);
        getOptional<TcpNetworkTxOptions>("tcpTxOptions", p.tcpTxOptions, j);
        getOptional<NetworkTxOptions>("multicastTxOptions", p.multicastTxOptions, j);
        getOptional<std::string>("certStoreFileName", p.certStoreFileName, j);
        getOptional<std::string>("certStorePasswordHex", p.certStorePasswordHex, j);
        getOptional<StringRestrictionList>("groupRestrictions", p.groupRestrictions, j);
        getOptional<std::string>("configurationCheckSignalName", p.configurationCheckSignalName, j, "rts.7b392d1.${id}");
        getOptional<Licensing>("licensing", p.licensing, j);
        getOptional<Featureset>("featureset", p.featureset, j);
        getOptional<RallypointUdpStreaming>("udpStreaming", p.udpStreaming, j);
        getOptional<uint32_t>("sysFlags", p.sysFlags, j, 0);
        getOptional<uint32_t>("normalTaskQueueBias", p.normalTaskQueueBias, j, 0);
        getOptional<bool>("enableLeafReflectionReverseSubscription", p.enableLeafReflectionReverseSubscription, j, false);
        getOptional<bool>("disableLoopDetection", p.disableLoopDetection, j, false);
        getOptional<uint32_t>("maxSecurityLevel", p.maxSecurityLevel, j, 0);
        getOptional<RallypointServerRouteMap>("routeMap", p.routeMap, j);
        getOptional<uint32_t>("maxOutboundPeerConnectionIntervalDeltaSecs", p.maxOutboundPeerConnectionIntervalDeltaSecs, j, 15);        
        getOptional<int>("peerRtTestIntervalMs", p.peerRtTestIntervalMs, j, 60000);
        getOptional<std::vector<RallypointRpRtTimingBehavior>>("peerRtBehaviors", p.peerRtBehaviors, j);
        getOptional<RallypointWebsocketSettings>("websocket", p.websocket, j);
        getOptional<NsmConfiguration>("nsm", p.nsm, j);
        getOptional<RallypointAdvertisingSettings>("advertising", p.advertising, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(PlatformDiscoveredService)
    /**
    * @brief RTS internal use
    *
    * Helper C++ class to serialize and de-serialize PlatformDiscoveredService JSON
    *
    * Example: @include[doc] examples/PlatformDiscoveredService.json
    *
    * @see TODO: Add references
    */
    class PlatformDiscoveredService : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(PlatformDiscoveredService)

    public:

        /** @brief Internal ID */
        std::string                             id;

        /** @brief Internal type. */
        std::string                             type;

        /** @brief Internal name. */
        std::string                             name;

        /** @brief Internal address. */
        NetworkAddress                          address;

        /** @brief Internal URI. */
        std::string                             uri;

        /** @brief Internal configuration version. */
        uint32_t                                configurationVersion;

        PlatformDiscoveredService()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            type.clear();
            name.clear();
            address.clear();
            uri.clear();
            configurationVersion = 0;
        }
    };

    static void to_json(nlohmann::json& j, const PlatformDiscoveredService& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(type),
            TOJSON_IMPL(name),
            TOJSON_IMPL(address),
            TOJSON_IMPL(uri),
            TOJSON_IMPL(configurationVersion)
        };
    }
    static void from_json(const nlohmann::json& j, PlatformDiscoveredService& p)
    {
        p.clear();
        getOptional<std::string>("id", p.id, j);
        getOptional<std::string>("type", p.type, j);
        getOptional<std::string>("name", p.name, j);
        getOptional<NetworkAddress>("address", p.address, j);
        getOptional<std::string>("uri", p.uri, j);
        getOptional<uint32_t>("configurationVersion", p.configurationVersion, j, 0);
    }


    //-----------------------------------------------------------
    class TimelineEvent
    {
    public:
        typedef enum
        {
            etUndefined           = 0,
            etAudio               = 1,
            etLocation            = 2,
            etUser                = 3
        } EventType_t;

        typedef enum
        {
            dNone        = 0,
            dInbound     = 1,
            dOutbound    = 2,
            dBoth        = 3,
            dUndefined   = 4,
        } Direction_t;
    };


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(TimelineQueryParameters)
    /**
    * @brief Parameters for querying the group timeline.
    *
    * Helper C++ class to serialize and de-serialize TimelineQueryParameters JSON
    *
    * Example: @include[doc] examples/TimelineQueryParameters.json
    *
    * @see TODO: Add references
    */
    class TimelineQueryParameters : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(TimelineQueryParameters)

    public:

        /** @brief Maximum number of records to return */
        long                    maxCount;

        /** @brief Sorted results with most recent timestamp first */
        bool                    mostRecentFirst;

        /** @brief Include events that started on or after this UNIX millisecond timestamp */
        uint64_t                startedOnOrAfter;

        /** @brief Include events that ended on or after this UNIX millisecond timestamp */
        uint64_t                endedOnOrBefore;

        /** @brief Include events for this direction. */
        int                     onlyDirection;

        /** @brief Include events for this type. */
        int                     onlyType;

        /** @brief Include only committed (not in-progress) events. */
        bool                    onlyCommitted;

        /** @brief Include events for this transmitter alias. */
        std::string             onlyAlias;

        /** @brief Include events for this transmitter node ID. */
        std::string             onlyNodeId;

        /** @brief Include events for this transmission ID. */
        int                     onlyTxId;

        /** @brief Ignore all other settings for SQL construction and use this query string instead. */
        std::string             sql;

        TimelineQueryParameters()
        {
            clear();
        }

        void clear()
        {
            maxCount = 50;
            mostRecentFirst = true;
            startedOnOrAfter = 0;
            endedOnOrBefore = 0;
            onlyDirection = 0;
            onlyType = 0;
            onlyCommitted = true;
            onlyAlias.clear();
            onlyNodeId.clear();
            sql.clear();
            onlyTxId = 0;
        }
    };

    static void to_json(nlohmann::json& j, const TimelineQueryParameters& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(maxCount),
            TOJSON_IMPL(mostRecentFirst),
            TOJSON_IMPL(startedOnOrAfter),
            TOJSON_IMPL(endedOnOrBefore),
            TOJSON_IMPL(onlyDirection),
            TOJSON_IMPL(onlyType),
            TOJSON_IMPL(onlyCommitted),
            TOJSON_IMPL(onlyAlias),
            TOJSON_IMPL(onlyNodeId),
            TOJSON_IMPL(onlyTxId),
            TOJSON_IMPL(sql)
        };
    }
    static void from_json(const nlohmann::json& j, TimelineQueryParameters& p)
    {
        p.clear();
        getOptional<long>("maxCount", p.maxCount, j, 50);
        getOptional<bool>("mostRecentFirst", p.mostRecentFirst, j, false);
        getOptional<uint64_t>("startedOnOrAfter", p.startedOnOrAfter, j, 0);
        getOptional<uint64_t>("endedOnOrBefore", p.endedOnOrBefore, j, 0);
        getOptional<int>("onlyDirection", p.onlyDirection, j, 0);
        getOptional<int>("onlyType", p.onlyType, j, 0);
        getOptional<bool>("onlyCommitted", p.onlyCommitted, j, true);
        getOptional<std::string>("onlyAlias", p.onlyAlias, j, EMPTY_STRING);
        getOptional<std::string>("onlyNodeId", p.onlyNodeId, j, EMPTY_STRING);
        getOptional<int>("onlyTxId", p.onlyTxId, j, 0);
        getOptional<std::string>("sql", p.sql, j, EMPTY_STRING);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(CertStoreCertificate)
    /**
    * @brief Holds a certificate and (optionally) a private key in a certstore
    *
    * Helper C++ class to serialize and de-serialize CertStoreCertificate JSON
    *
    */
    class CertStoreCertificate : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(CertStoreCertificate)

    public:
        /** @brief Id of the certificate */
        std::string                     id;

        /** @brief Certificate in PEM format */
        std::string                     certificatePem;

        /** @brief Private key in PEM format */
        std::string                     privateKeyPem;

        /** @brief Unserialized internal data */
        void                            *internalData;

        /** @brief Additional tags */
        std::string                     tags;
        
        CertStoreCertificate()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            certificatePem.clear();
            privateKeyPem.clear();
            internalData = nullptr;
            tags.clear();
        }
    };

    static void to_json(nlohmann::json& j, const CertStoreCertificate& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(certificatePem),
            TOJSON_IMPL(privateKeyPem),
            TOJSON_IMPL(tags)
        };
    }
    static void from_json(const nlohmann::json& j, CertStoreCertificate& p)
    {
        p.clear();
        j.at("id").get_to(p.id);
        j.at("certificatePem").get_to(p.certificatePem);
        getOptional<std::string>("privateKeyPem", p.privateKeyPem, j, EMPTY_STRING);
        getOptional<std::string>("tags", p.tags, j, EMPTY_STRING);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(CertStore)
    /**
    * @brief Holds a certstore
    *
    * Helper C++ class to serialize and de-serialize CertStore JSON
    *
    */
    class CertStore : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(CertStore)

    public:
        /** @brief The ID of the certstore */
        std::string                         id;

        /** @brief Array of certificates in this store */
        std::vector<CertStoreCertificate>    certificates;

        CertStore()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            certificates.clear();
        }
    };

    static void to_json(nlohmann::json& j, const CertStore& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(certificates)
        };
    }
    static void from_json(const nlohmann::json& j, CertStore& p)
    {
        p.clear();
        getOptional<std::string>("id", p.id, j, EMPTY_STRING);
        getOptional<std::vector<CertStoreCertificate>>("certificates", p.certificates, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(CertStoreCertificateElement)
    /**
    * @brief Description of a certstore certificate element
    *
    * Helper C++ class to serialize and de-serialize CertStoreCertificateElement JSON
    *
    */
    class CertStoreCertificateElement : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(CertStoreCertificateElement)

    public:
        /** @brief ID */
        std::string                     id;

        /** @brief True if the certificate has a private key associated with it */
        bool                            hasPrivateKey;

        /** @brief PEM of the certificate */
        std::string                     certificatePem;

        /** @brief Additional attributes */
        std::string                     tags;

        CertStoreCertificateElement()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            hasPrivateKey = false;
            tags.clear();
        }
    };

    static void to_json(nlohmann::json& j, const CertStoreCertificateElement& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(hasPrivateKey),
            TOJSON_IMPL(tags)
        };

        if(!p.certificatePem.empty())
        {
            j["certificatePem"] = p.certificatePem;            
        }
    }
    static void from_json(const nlohmann::json& j, CertStoreCertificateElement& p)
    {
        p.clear();
        getOptional<std::string>("id", p.id, j, EMPTY_STRING);
        getOptional<bool>("hasPrivateKey", p.hasPrivateKey, j, false);
        getOptional<std::string>("certificatePem", p.certificatePem, j, EMPTY_STRING);
        getOptional<std::string>("tags", p.tags, j, EMPTY_STRING);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(CertStoreDescriptor)
    /**
    * @brief Description of a certstore
    *
    * Helper C++ class to serialize and de-serialize CertStoreDescriptor JSON
    *
    */
    class CertStoreDescriptor : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(CertStoreDescriptor)

    public:
        /** @brief Certstore ID */
        std::string                                     id;

        /** @brief Name of the file the certstore resides in. */
        std::string                                     fileName;

        /** @brief Version of the certstore. */
        int                                             version;

        /** @brief Flags set for the certstore. */
        int                                             flags;

        /** @brief Array of certificate elements. */
        std::vector<CertStoreCertificateElement>        certificates;

        CertStoreDescriptor()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            fileName.clear();
            version = 0;
            flags = 0;
            certificates.clear();
        }
    };

    static void to_json(nlohmann::json& j, const CertStoreDescriptor& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(fileName),
            TOJSON_IMPL(version),
            TOJSON_IMPL(flags),
            TOJSON_IMPL(certificates)
        };
    }
    static void from_json(const nlohmann::json& j, CertStoreDescriptor& p)
    {
        p.clear();
        getOptional<std::string>("id", p.id, j, EMPTY_STRING);
        getOptional<std::string>("fileName", p.fileName, j, EMPTY_STRING);
        getOptional<int>("version", p.version, j, 0);
        getOptional<int>("flags", p.flags, j, 0);
        getOptional<std::vector<CertStoreCertificateElement>>("certificates", p.certificates, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(CertificateDescriptor)
    /**
    * @brief Description of a certificate
    *
    * Helper C++ class to serialize and de-serialize CertificateDescriptor JSON
    *
    */
    class CertificateDescriptor : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(CertificateDescriptor)

    public:
        /** @brief Subject */
        std::string                                     subject;

        /** @brief Issuer */
        std::string                                     issuer;

        /** @brief Indicates whether the certificqte is self-signed */
        bool                                            selfSigned;

        /** @brief Version */
        int                                             version;

        /** @brief Validity date notBefore */
        std::string                                     notBefore;

        /** @brief Validity date notAfter */
        std::string                                     notAfter;

        /** @brief Serial # */
        std::string                                     serial;

        /** @brief Fingerprint */
        std::string                                     fingerprint;

        CertificateDescriptor()
        {
            clear();
        }

        void clear()
        {
            subject.clear();
            issuer.clear();
            selfSigned = false;
            version = 0;
            notBefore.clear();
            notAfter.clear();
            serial.clear();
            fingerprint.clear();
        }
    };

    static void to_json(nlohmann::json& j, const CertificateDescriptor& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(subject),
            TOJSON_IMPL(issuer),
            TOJSON_IMPL(selfSigned),
            TOJSON_IMPL(version),
            TOJSON_IMPL(notBefore),
            TOJSON_IMPL(notAfter),
            TOJSON_IMPL(serial),
            TOJSON_IMPL(fingerprint)
        };
    }
    static void from_json(const nlohmann::json& j, CertificateDescriptor& p)
    {
        p.clear();
        getOptional<std::string>("subject", p.subject, j, EMPTY_STRING);
        getOptional<std::string>("issuer", p.issuer, j, EMPTY_STRING);
        getOptional<bool>("selfSigned", p.selfSigned, j, false);
        getOptional<int>("version", p.version, j, 0);
        getOptional<std::string>("notBefore", p.notBefore, j, EMPTY_STRING);
        getOptional<std::string>("notAfter", p.notAfter, j, EMPTY_STRING);
        getOptional<std::string>("serial", p.serial, j, EMPTY_STRING);
        getOptional<std::string>("fingerprint", p.fingerprint, j, EMPTY_STRING);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(RiffDescriptor)
    /**
    * @brief Helper class for serializing and deserializing the RiffDescriptor JSON
    *
    * Helper C++ class to serialize and de-serialize RiffDescriptor JSON
    *
    * Example: @include[doc] examples/RiffDescriptor.json
    *
    * @see TODO: engageGetRiffDescriptor
    */
    class RiffDescriptor : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(RiffDescriptor)

    public:
        /** @brief Name of the RIFF file. */
        std::string                             file;

        /** @brief True if the ECDSA signature is verified. */
        bool                                    verified;

        /** @brief Number of audio channels */
        int                                     channels;

        /** @brief Number of audio samples */
        int                                     sampleCount;

        /** @brief [Optional] Meta data associated with the file - typically a stringified JSON object. */
        std::string                             meta;

        /** @brief [Optional] X.509 certificate in PEM format used to sign the RIFF file. */
        std::string                             certPem;

        /** @brief [Optional] X.509 certificate parsed into a CertificateDescriptor object. */
        CertificateDescriptor                   certDescriptor;

        /** @brief [Optional] ECDSA signature */
        std::string                             signature;

        RiffDescriptor()
        {
            clear();
        }

        void clear()
        {
            file.clear();
            verified = false;
            channels = 0;
            sampleCount = 0;
            meta.clear();
            certPem.clear();
            certDescriptor.clear();
            signature.clear();
        }
    };

    static void to_json(nlohmann::json& j, const RiffDescriptor& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(file),
            TOJSON_IMPL(verified),
            TOJSON_IMPL(channels),
            TOJSON_IMPL(sampleCount),
            TOJSON_IMPL(meta),
            TOJSON_IMPL(certPem),
            TOJSON_IMPL(certDescriptor),
            TOJSON_IMPL(signature)
        };
    }

    static void from_json(const nlohmann::json& j, RiffDescriptor& p)
    {
        p.clear();
        FROMJSON_IMPL(file, std::string, EMPTY_STRING);
        FROMJSON_IMPL(verified, bool, false);
        FROMJSON_IMPL(channels, int, 0);
        FROMJSON_IMPL(sampleCount, int, 0);
        FROMJSON_IMPL(meta, std::string, EMPTY_STRING);
        FROMJSON_IMPL(certPem, std::string, EMPTY_STRING);
        getOptional<CertificateDescriptor>("certDescriptor", p.certDescriptor, j);
        FROMJSON_IMPL(signature, std::string, EMPTY_STRING);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(BridgeCreationDetail)
    /**
    * @brief Detailed information for a bridge creation
    *
    * Helper C++ class to serialize and de-serialize BridgeCreationDetail JSON
    *
    */
    class BridgeCreationDetail : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_WRAPPED_JSON_SERIALIZATION(BridgeCreationDetail)
        IMPLEMENT_JSON_DOCUMENTATION(BridgeCreationDetail)

    public:
        /** @brief Creation status */
        typedef enum
        {
            /** @brief Undefined */
            csUndefined                         = 0,

            /** @brief Creation OK */
            csOk                                = 1,

            /** @brief Configuration JSON is empty */
            csNoJson                            = -1,

            /** @brief Bridge already exists */
            csAlreadyExists                     = -3,

            /** @brief Invalid configuration */
            csInvalidConfiguration              = -4,

            /** @brief Invalid JSON */
            csInvalidJson                       = -5,

            /** @brief Insufficient groups in bridge */
            csInsufficientGroups                = -6,

            /** @brief Too many groups in bridge */
            csTooManyGroups                     = -7,

            /** @brief Duplicate group in the same bridge */
            csDuplicateGroup                    = -8,

            /** @brief Duplicate group in the same bridge */
            csLocalLoopDetected                 = -9,
        } CreationStatus_t;

        /** @brief ID of the bridge */
        std::string                                     id;

        /** @brief The creation status */
        CreationStatus_t                                status;

        BridgeCreationDetail()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            status = csUndefined;
        }
    };

    static void to_json(nlohmann::json& j, const BridgeCreationDetail& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(status)
        };
    }
    static void from_json(const nlohmann::json& j, BridgeCreationDetail& p)
    {
        p.clear();
        getOptional<std::string>("id", p.id, j, EMPTY_STRING);
        getOptional<BridgeCreationDetail::CreationStatus_t>("status", p.status, j, BridgeCreationDetail::CreationStatus_t::csUndefined);
    }
    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(GroupConnectionDetail)
    /**
    * @brief Detailed information for a group connection
    *
    * Helper C++ class to serialize and de-serialize GroupConnectionDetail JSON
    *
    */
    class GroupConnectionDetail : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_WRAPPED_JSON_SERIALIZATION(GroupConnectionDetail)
        IMPLEMENT_JSON_DOCUMENTATION(GroupConnectionDetail)

    public:
        /** @brief Connection type. */
        typedef enum
        {
            /** @brief Undefined */
            ctUndefined                     = 0,

            /** @brief Direct datagram */
            ctDirectDatagram                = 1,

            /** @brief Rallypoint */
            ctRallypoint                    = 2
        } ConnectionType_t;

        /** @brief ID of the group */
        std::string                                     id;

        /** @brief The connection type */
        ConnectionType_t                                connectionType;

        /** @brief Peer information */
        std::string                                     peer;

        /** @brief Indicates whether the connection is for purposes of failover */
        bool                                            asFailover;

        /** @brief [Optional] Additional reason information */
        std::string                                     reason;

        GroupConnectionDetail()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            connectionType = ctUndefined;
            peer.clear();
            asFailover = false;
            reason.clear();
        }
    };

    static void to_json(nlohmann::json& j, const GroupConnectionDetail& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(connectionType),
            TOJSON_IMPL(peer),
            TOJSON_IMPL(asFailover),
            TOJSON_IMPL(reason)
        };

        if(p.asFailover)
        {
            j["asFailover"] = p.asFailover;
        }
    }
    static void from_json(const nlohmann::json& j, GroupConnectionDetail& p)
    {
        p.clear();
        getOptional<std::string>("id", p.id, j, EMPTY_STRING);
        getOptional<GroupConnectionDetail::ConnectionType_t>("connectionType", p.connectionType, j, GroupConnectionDetail::ConnectionType_t::ctUndefined);
        getOptional<std::string>("peer", p.peer, j, EMPTY_STRING);
        getOptional<bool>("asFailover", p.asFailover, j, false);
        getOptional<std::string>("reason", p.reason, j, EMPTY_STRING);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(GroupTxDetail)
    /**
    * @brief Detailed information for a group transmit
    *
    * Helper C++ class to serialize and de-serialize GroupTxDetail JSON
    *
    */
    class GroupTxDetail : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_WRAPPED_JSON_SERIALIZATION(GroupTxDetail)
        IMPLEMENT_JSON_DOCUMENTATION(GroupTxDetail)

    public:
        /** @brief TxStatus */
        typedef enum
        {
            /** @brief Undefined */
            txsUndefined                    = 0,

            /** @brief  TX has started */
            txsTxStarted                    = 1,

            /** @brief  TX has ended */
            txsTxEnded                      = 2,

            /** @brief  This is not an audio group */
            txsNotAnAudioGroup              = -1,

            /** @brief Group has not been joined */
            txsNotJoined                    = -2,

            /** @brief Group has not been connected */
            txsNotConnected                 = -3,

            /** @brief Group is already transmitting */
            txsAlreadyTransmitting          = -4,

            /** @brief Invalid TX JSON parameters */
            txsInvalidParams                = -5,

            /** @brief TX priority is too low @see remotePriority @see remotePriority */
            txsPriorityTooLow               = -6,

            /** @brief RX active on a non-FDX configuration @see nonFdxMsHangRemaining */
            txsRxActiveOnNonFdx             = -7,

            /** @brief Cannot subscribe to the input */
            txsCannotSubscribeToInput       = -8,

            /** @brief Invalid ID */
            txsInvalidId                    = -9,

            /** @brief  TX has ended with a failure */
            txsTxEndedWithFailure           = -10            
        } TxStatus_t;

        /** @brief ID of the group */
        std::string                                     id;

        /** @brief The TX status */
        TxStatus_t                                      status;

        /** @brief Local TX priority (optional) */
        int                                             localPriority;

        /** @brief Remote TX priority (optional) */
        int                                             remotePriority;

        /** @brief Milliseconds of hang time remaining on a non-FDX group (optional) */
        long                                            nonFdxMsHangRemaining;

        /** @brief Transmission ID (optional) */
        uint32_t                                        txId;

        GroupTxDetail()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            status = txsUndefined;
            localPriority = 0;
            remotePriority = 0;
            nonFdxMsHangRemaining = 0;
            txId = 0;
        }
    };

    static void to_json(nlohmann::json& j, const GroupTxDetail& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(status),
            TOJSON_IMPL(localPriority),
            TOJSON_IMPL(txId)
        };

        // Include remote priority if status is related to that
        if(p.status == GroupTxDetail::TxStatus_t::txsPriorityTooLow)
        {
            j["remotePriority"] = p.remotePriority;
        }
        else if(p.status == GroupTxDetail::TxStatus_t::txsRxActiveOnNonFdx)
        {
            j["nonFdxMsHangRemaining"] = p.nonFdxMsHangRemaining;
        }
    }
    static void from_json(const nlohmann::json& j, GroupTxDetail& p)
    {
        p.clear();
        getOptional<std::string>("id", p.id, j, EMPTY_STRING);
        getOptional<GroupTxDetail::TxStatus_t>("status", p.status, j, GroupTxDetail::TxStatus_t::txsUndefined);
        getOptional<int>("localPriority", p.localPriority, j, 0);
        getOptional<int>("remotePriority", p.remotePriority, j, 0);
        getOptional<long>("nonFdxMsHangRemaining", p.nonFdxMsHangRemaining, j, 0);
        getOptional<uint32_t>("txId", p.txId, j, 0);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(GroupCreationDetail)
    /**
    * @brief Detailed information for a group creation
    *
    * Helper C++ class to serialize and de-serialize GroupCreationDetail JSON
    *
    */
    class GroupCreationDetail : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_WRAPPED_JSON_SERIALIZATION(GroupCreationDetail)
        IMPLEMENT_JSON_DOCUMENTATION(GroupCreationDetail)

    public:
        /** @brief Creation status */
        typedef enum
        {
            /** @brief Undefined */
            csUndefined                         = 0,

            /** @brief Creation OK */
            csOk                                = 1,

            /** @brief Configuration JSON is empty */
            csNoJson                            = -1,

            /** @brief Conflicting Rallypoint list and cluster */
            csConflictingRpListAndCluster       = -2,

            /** @brief Group already exists */
            csAlreadyExists                     = -3,

            /** @brief Invalid configuration */
            csInvalidConfiguration              = -4,

            /** @brief Invalid JSON */
            csInvalidJson                       = -5,

            /** @brief Crypto failure */
            csCryptoFailure                     = -6,

            /** @brief Audio input failure */
            csAudioInputFailure                 = -7,

            /** @brief Audio input failure */
            csAudioOutputFailure                = -8,

            /** @brief Unsupported audio encoder */
            csUnsupportedAudioEncoder          = -9,

            /** @brief Insufficient group licenses available */
            csNoLicense                         = -10,

            /** @brief The transport type is invalid */
            csInvalidTransport                  = -11,
        } CreationStatus_t;

        /** @brief ID of the group */
        std::string                                     id;

        /** @brief The creation status */
        CreationStatus_t                                status;

        GroupCreationDetail()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            status = csUndefined;
        }
    };

    static void to_json(nlohmann::json& j, const GroupCreationDetail& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(status)
        };
    }
    static void from_json(const nlohmann::json& j, GroupCreationDetail& p)
    {
        p.clear();
        getOptional<std::string>("id", p.id, j, EMPTY_STRING);
        getOptional<GroupCreationDetail::CreationStatus_t>("status", p.status, j, GroupCreationDetail::CreationStatus_t::csUndefined);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(GroupReconfigurationDetail)
    /**
    * @brief Detailed information for a group reconfiguration
    *
    * Helper C++ class to serialize and de-serialize GroupReconfigurationDetail JSON
    *
    */
    class GroupReconfigurationDetail : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_WRAPPED_JSON_SERIALIZATION(GroupReconfigurationDetail)
        IMPLEMENT_JSON_DOCUMENTATION(GroupReconfigurationDetail)

    public:
        /** @brief Reconfiguration status */
        typedef enum
        {
            /** @brief Undefined */
            rsUndefined                         = 0,

            /** @brief Reconfiguration OK */
            rsOk                                = 1,

            /** @brief Configuration JSON is empty */
            rsNoJson                            = -1,

            /** @brief Invalid configuration */
            rsInvalidConfiguration              = -2,

            /** @brief Invalid JSON */
            rsInvalidJson                       = -3,

            /** @brief Audio input failure */
            rsAudioInputFailure                 = -4,

            /** @brief Audio input failure */
            rsAudioOutputFailure                = -5,

            /** @brief Group does not exist */
            rsDoesNotExist                      = -6,

            /** @brief Audio input device is in use (group is transmitting) */
            rsAudioInputInUse                   = -7,

            /** @brief Audio disabled for audio group */
            rsAudioDisabledForGroup             = -8,

            /** @brief Group is not an audio group */
            rsGroupIsNotAudio                   = -9
        } ReconfigurationStatus_t;

        /** @brief ID of the group */
        std::string                                     id;

        /** @brief The creation status */
        ReconfigurationStatus_t                         status;

        GroupReconfigurationDetail()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            status = rsUndefined;
        }
    };

    static void to_json(nlohmann::json& j, const GroupReconfigurationDetail& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(status)
        };
    }
    static void from_json(const nlohmann::json& j, GroupReconfigurationDetail& p)
    {
        p.clear();
        getOptional<std::string>("id", p.id, j, EMPTY_STRING);
        getOptional<GroupReconfigurationDetail::ReconfigurationStatus_t>("status", p.status, j, GroupReconfigurationDetail::ReconfigurationStatus_t::rsUndefined);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(GroupHealthReport)
    /**
    * @brief Detailed information regarding a group's health
    *
    * Helper C++ class to serialize and de-serialize GroupHealthReport JSON
    *
    */
    class GroupHealthReport : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_WRAPPED_JSON_SERIALIZATION(GroupHealthReport)
        IMPLEMENT_JSON_DOCUMENTATION(GroupHealthReport)

    public:
        std::string id;
        uint64_t    lastErrorTs;
        uint64_t    decryptionErrors;
        uint64_t    encryptionErrors;
        uint64_t    unsupportDecoderErrors;
        uint64_t    decoderFailures;
        uint64_t    decoderStartFailures;
        uint64_t    inboundRtpPacketAllocationFailures;
        uint64_t    inboundRtpPacketLoadFailures;
        uint64_t    latePacketsDiscarded;
        uint64_t    jitterBufferInsertionFailures;
        uint64_t    presenceDeserializationFailures;
        uint64_t    notRtpErrors;
        uint64_t    generalErrors;
        
        GroupHealthReport()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            lastErrorTs = 0;
            decryptionErrors = 0;
            encryptionErrors = 0;
            unsupportDecoderErrors = 0;
            decoderFailures = 0;
            decoderStartFailures = 0;
            inboundRtpPacketAllocationFailures = 0;
            inboundRtpPacketLoadFailures = 0;
            latePacketsDiscarded = 0;
            jitterBufferInsertionFailures = 0;
            presenceDeserializationFailures = 0;
            notRtpErrors = 0;
            generalErrors = 0;
        }
    };

    static void to_json(nlohmann::json& j, const GroupHealthReport& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(lastErrorTs),
            TOJSON_IMPL(decryptionErrors),
            TOJSON_IMPL(encryptionErrors),
            TOJSON_IMPL(unsupportDecoderErrors),
            TOJSON_IMPL(decoderFailures),
            TOJSON_IMPL(decoderStartFailures),
            TOJSON_IMPL(inboundRtpPacketAllocationFailures),
            TOJSON_IMPL(inboundRtpPacketLoadFailures),
            TOJSON_IMPL(latePacketsDiscarded),
            TOJSON_IMPL(jitterBufferInsertionFailures),
            TOJSON_IMPL(presenceDeserializationFailures),
            TOJSON_IMPL(notRtpErrors),
            TOJSON_IMPL(generalErrors)
        };
    }
    static void from_json(const nlohmann::json& j, GroupHealthReport& p)
    {
        p.clear();
        getOptional<std::string>("id", p.id, j, EMPTY_STRING);
        getOptional<uint64_t>("lastErrorTs", p.lastErrorTs, j, 0);
        getOptional<uint64_t>("decryptionErrors", p.decryptionErrors, j, 0);
        getOptional<uint64_t>("encryptionErrors", p.encryptionErrors, j, 0);
        getOptional<uint64_t>("unsupportDecoderErrors", p.unsupportDecoderErrors, j, 0);
        getOptional<uint64_t>("decoderFailures", p.decoderFailures, j, 0);
        getOptional<uint64_t>("decoderStartFailures", p.decoderStartFailures, j, 0);
        getOptional<uint64_t>("inboundRtpPacketAllocationFailures", p.inboundRtpPacketAllocationFailures, j, 0);
        getOptional<uint64_t>("inboundRtpPacketLoadFailures", p.inboundRtpPacketLoadFailures, j, 0);
        getOptional<uint64_t>("latePacketsDiscarded", p.latePacketsDiscarded, j, 0);
        getOptional<uint64_t>("jitterBufferInsertionFailures", p.jitterBufferInsertionFailures, j, 0);
        getOptional<uint64_t>("presenceDeserializationFailures", p.presenceDeserializationFailures, j, 0);
        getOptional<uint64_t>("notRtpErrors", p.notRtpErrors, j, 0);
        getOptional<uint64_t>("generalErrors", p.generalErrors, j, 0);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(InboundProcessorStats)
    /**
    * @brief Detailed statistics for an inbound processor
    *
    * Helper C++ class to serialize and de-serialize InboundProcessorStats JSON
    *
    */
    class InboundProcessorStats : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_WRAPPED_JSON_SERIALIZATION(InboundProcessorStats)
        IMPLEMENT_JSON_DOCUMENTATION(InboundProcessorStats)

    public:
        uint32_t                    ssrc;
        double                      jitter;
        uint64_t                    minRtpSamplesInQueue;
        uint64_t                    maxRtpSamplesInQueue;
        uint64_t                    totalSamplesTrimmed;
        uint64_t                    underruns;
        uint64_t                    overruns;
        uint64_t                    samplesInQueue;
        uint64_t                    totalPacketsReceived;
        uint64_t                    totalPacketsLost;
        uint64_t                    totalPacketsDiscarded;

        InboundProcessorStats()
        {
            clear();
        }

        void clear()
        {
            ssrc = 0;
            jitter = 0.0;
            minRtpSamplesInQueue = 0;
            maxRtpSamplesInQueue = 0;
            totalSamplesTrimmed = 0;
            underruns = 0;
            overruns = 0;
            samplesInQueue = 0;
            totalPacketsReceived = 0;
            totalPacketsLost = 0;
            totalPacketsDiscarded = 0;
        }
    };

    static void to_json(nlohmann::json& j, const InboundProcessorStats& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(ssrc),
            TOJSON_IMPL(jitter),
            TOJSON_IMPL(minRtpSamplesInQueue),
            TOJSON_IMPL(maxRtpSamplesInQueue),
            TOJSON_IMPL(totalSamplesTrimmed),
            TOJSON_IMPL(underruns),
            TOJSON_IMPL(overruns),
            TOJSON_IMPL(samplesInQueue),
            TOJSON_IMPL(totalPacketsReceived),
            TOJSON_IMPL(totalPacketsLost),
            TOJSON_IMPL(totalPacketsDiscarded)
        };
    }
    static void from_json(const nlohmann::json& j, InboundProcessorStats& p)
    {
        p.clear();
        getOptional<uint32_t>("ssrc", p.ssrc, j, 0);
        getOptional<double>("jitter", p.jitter, j, 0.0);
        getOptional<uint64_t>("minRtpSamplesInQueue", p.minRtpSamplesInQueue, j, 0);
        getOptional<uint64_t>("maxRtpSamplesInQueue", p.maxRtpSamplesInQueue, j, 0);
        getOptional<uint64_t>("totalSamplesTrimmed", p.totalSamplesTrimmed, j, 0);
        getOptional<uint64_t>("underruns", p.underruns, j, 0);
        getOptional<uint64_t>("overruns", p.overruns, j, 0);
        getOptional<uint64_t>("samplesInQueue", p.samplesInQueue, j, 0);
        getOptional<uint64_t>("totalPacketsReceived", p.totalPacketsReceived, j, 0);
        getOptional<uint64_t>("totalPacketsLost", p.totalPacketsLost, j, 0);
        getOptional<uint64_t>("totalPacketsDiscarded", p.totalPacketsDiscarded, j, 0);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(TrafficCounter)
    /**
    * @brief Traffic counters
    *
    * Helper C++ class to serialize and de-serialize TrafficCounter JSON
    *
    */
    class TrafficCounter : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_WRAPPED_JSON_SERIALIZATION(TrafficCounter)
        IMPLEMENT_JSON_DOCUMENTATION(TrafficCounter)

    public:
        uint64_t                packets;
        uint64_t                bytes;
        uint64_t                errors;

        TrafficCounter()
        {
            clear();
        }

        void clear()
        {
            packets = 0;
            bytes = 0;
            errors = 0;
        }
    };

    static void to_json(nlohmann::json& j, const TrafficCounter& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(packets),
            TOJSON_IMPL(bytes),
            TOJSON_IMPL(errors)
        };
    }
    static void from_json(const nlohmann::json& j, TrafficCounter& p)
    {
        p.clear();
        getOptional<uint64_t>("packets", p.packets, j, 0);
        getOptional<uint64_t>("bytes", p.bytes, j, 0);
        getOptional<uint64_t>("errors", p.errors, j, 0);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(GroupStats)
    /**
    * @brief Detailed statistics for group
    *
    * Helper C++ class to serialize and de-serialize GroupStats JSON
    *
    */
    class GroupStats : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_WRAPPED_JSON_SERIALIZATION(GroupStats)
        IMPLEMENT_JSON_DOCUMENTATION(GroupStats)

    public:
        std::string                             id;
        //std::vector<InboundProcessorStats>      rtpInbounds;
        TrafficCounter                          rxTraffic;
        TrafficCounter                          txTraffic;

        GroupStats()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            //rtpInbounds.clear();
            rxTraffic.clear();
            txTraffic.clear();
        }
    };

    static void to_json(nlohmann::json& j, const GroupStats& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            //TOJSON_IMPL(rtpInbounds),
            TOJSON_IMPL(rxTraffic),
            TOJSON_IMPL(txTraffic)
        };
    }
    static void from_json(const nlohmann::json& j, GroupStats& p)
    {
        p.clear();
        getOptional<std::string>("id", p.id, j, EMPTY_STRING);
        //getOptional<std::vector<InboundProcessorStats>>("rtpInbounds", p.rtpInbounds, j);
        getOptional<TrafficCounter>("rxTraffic", p.rxTraffic, j);
        getOptional<TrafficCounter>("txTraffic", p.txTraffic, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(RallypointConnectionDetail)
    /**
    * @brief Detailed information for a rallypoint connection
    *
    * Helper C++ class to serialize and de-serialize RallypointConnectionDetail JSON
    *
    */
    class RallypointConnectionDetail : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_WRAPPED_JSON_SERIALIZATION(RallypointConnectionDetail)
        IMPLEMENT_JSON_DOCUMENTATION(RallypointConnectionDetail)

    public:
        /** @brief Id */
        std::string                                     internalId;

        /** @brief Host */
        std::string                                     host;

        /** @brief Port */
        int                                             port;

        /** @brief Milliseconds until next connection attempt */
        uint64_t                                        msToNextConnectionAttempt;

        RallypointConnectionDetail()
        {
            clear();
        }

        void clear()
        {
            internalId.clear();
            host.clear();
            port = 0;
            msToNextConnectionAttempt = 0;
        }
    };

    static void to_json(nlohmann::json& j, const RallypointConnectionDetail& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(internalId),
            TOJSON_IMPL(host),
            TOJSON_IMPL(port)
        };

        if(p.msToNextConnectionAttempt > 0)
        {
            j["msToNextConnectionAttempt"] = p.msToNextConnectionAttempt;
        }
    }
    static void from_json(const nlohmann::json& j, RallypointConnectionDetail& p)
    {
        p.clear();
        getOptional<std::string>("internalId", p.internalId, j, EMPTY_STRING);
        getOptional<std::string>("host", p.host, j, EMPTY_STRING);
        getOptional<int>("port", p.port, j, 0);
        getOptional<uint64_t>("msToNextConnectionAttempt", p.msToNextConnectionAttempt, j, 0);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(TranslationSession)
    /**
    * @brief Translation session settings
    *
    * Helper C++ class to serialize and de-serialize TranslationSession JSON
    *
    * Example: @include[doc] examples/TranslationSession.json
    *
    * @see TODO: ConfigurationObjects::TranslationSession
    */
    class TranslationSession : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(TranslationSession)

    public:
        /** @brief ID */
        std::string                 id;

        /** @brief Name */
        std::string                 name;

        /** @brief List of group IDs to be included in the session */
        std::vector<std::string>    groups;

        /** @brief [Optional, Default: true] Enable the session */
        bool                        enabled;

        TranslationSession()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            name.clear();
            groups.clear();
            enabled = true;
        }
    };

    static void to_json(nlohmann::json& j, const TranslationSession& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(name),
            TOJSON_IMPL(groups),
            TOJSON_IMPL(enabled)
        };
    }
    static void from_json(const nlohmann::json& j, TranslationSession& p)
    {
        p.clear();
        FROMJSON_IMPL(id, std::string, EMPTY_STRING);
        FROMJSON_IMPL(name, std::string, EMPTY_STRING);
        getOptional<std::vector<std::string>>("groups", p.groups, j);
        FROMJSON_IMPL(enabled, bool, true);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(TranslationConfiguration)
    /**
    * @brief Translation configuration
    *
    * Helper C++ class to serialize and de-serialize TranslationConfiguration JSON
    *
    * Example: @include[doc] examples/TranslationConfiguration.json
    *
    * @see TODO: ConfigurationObjects::TranslationConfiguration
    */
    class TranslationConfiguration : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(TranslationConfiguration)

    public:
        /** @brief Array of sessions in the configuration */
        std::vector<TranslationSession>         sessions;

        /** @brief Array of groups in the configuration */
        std::vector<Group>                      groups;

        TranslationConfiguration()
        {
            clear();
        }

        void clear()
        {
            sessions.clear();
            groups.clear();
        }
    };

    static void to_json(nlohmann::json& j, const TranslationConfiguration& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(sessions),
            TOJSON_IMPL(groups)
        };
    }
    static void from_json(const nlohmann::json& j, TranslationConfiguration& p)
    {
        p.clear();
        getOptional<std::vector<TranslationSession>>("sessions", p.sessions, j);
        getOptional<std::vector<Group>>("groups", p.groups, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(LingoServerStatusReportConfiguration)
    /**
    * @brief TODO: Configuration for the translation server status report file
    *
    * Helper C++ class to serialize and de-serialize LingoServerStatusReportConfiguration JSON
    *
    * Example: @include[doc] examples/LingoServerStatusReportConfiguration.json
    *
    * @see RallypointServer
    */
    class LingoServerStatusReportConfiguration : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(LingoServerStatusReportConfiguration)

    public:
        /** File name to use for the status report. */
        std::string                     fileName;

        /** [Optional, Default: 30] The interval at which to write out the status report to file. */
        int                             intervalSecs;

        /** [Optional, Default: false] Indicates if status reporting is enabled. */
        bool                            enabled;

        /** [Optional, Default: null] Command to be executed every time the status report is produced. */
        std::string                     runCmd;

        /** [Optional, Default: false] Indicates whether to include details of each group. */
        bool                            includeGroupDetail;

        /** [Optional, Default: false] Indicates whether to include details of each session. */
        bool                            includeSessionDetail;

        /** [Optional, Default: false] Indicates whether to include details of each group in each session. */
        bool                            includeSessionGroupDetail;

        LingoServerStatusReportConfiguration()
        {
            clear();
        }

        void clear()
        {
            fileName.clear();
            intervalSecs = 60;
            enabled = false;
            includeGroupDetail = false;
            includeSessionDetail = false;
            includeSessionGroupDetail = false;
            runCmd.clear();
        }
    };

    static void to_json(nlohmann::json& j, const LingoServerStatusReportConfiguration& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(fileName),
            TOJSON_IMPL(intervalSecs),
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(includeGroupDetail),
            TOJSON_IMPL(includeSessionDetail),
            TOJSON_IMPL(includeSessionGroupDetail),
            TOJSON_IMPL(runCmd)
        };
    }
    static void from_json(const nlohmann::json& j, LingoServerStatusReportConfiguration& p)
    {
        p.clear();
        getOptional<std::string>("fileName", p.fileName, j);
        getOptional<int>("intervalSecs", p.intervalSecs, j, 60);
        getOptional<bool>("enabled", p.enabled, j, false);
        getOptional<std::string>("runCmd", p.runCmd, j);
        getOptional<bool>("includeGroupDetail", p.includeGroupDetail, j, false);
        getOptional<bool>("includeSessionDetail", p.includeSessionDetail, j, false);
        getOptional<bool>("includeSessionGroupDetail", p.includeSessionGroupDetail, j, false);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(LingoServerInternals)
    /**
    * @brief Internal translator server settings
    *
    * These settings are used to configure internal parameters.
    *
    * Helper C++ class to serialize and de-serialize LingoServerInternals JSON
    *
    * Example: @include[doc] examples/LingoServerInternals.json
    *
    * @see engageInitialize, ConfigurationObjects::LingoServerConfiguration
    */
    class LingoServerInternals : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(LingoServerInternals)

    public:
        /** @brief [Optional] Settings for the watchdog. */
        WatchdogSettings    watchdog;

        /** @brief [Optional, Default: 1000] Interval at which to run the housekeeper thread. */
        int                 housekeeperIntervalMs;

        LingoServerInternals()
        {
            clear();
        }

        void clear()
        {
            watchdog.clear();
            housekeeperIntervalMs = 1000;
        }
    };

    static void to_json(nlohmann::json& j, const LingoServerInternals& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(watchdog),
            TOJSON_IMPL(housekeeperIntervalMs)
        };
    }
    static void from_json(const nlohmann::json& j, LingoServerInternals& p)
    {
        p.clear();
        getOptional<WatchdogSettings>("watchdog", p.watchdog, j);
        getOptional<int>("housekeeperIntervalMs", p.housekeeperIntervalMs, j, 1000);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(LingoServerConfiguration)
    /**
    * @brief Configuration for the linguistics server
    *
    * Helper C++ class to serialize and de-serialize LingoServerConfiguration JSON
    *
    * Example: @include[doc] examples/LingoServerConfiguration.json
    *
    */
    class LingoServerConfiguration : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(LingoServerConfiguration)

    public:
        /** @brief A unqiue identifier for the linguistics server */
        std::string                                 id;

        /** @brief Number of seconds between checks to see if the service configuration has been updated.  Default is 60.*/
        int                                         serviceConfigurationFileCheckSecs;
        
        /** @brief Name of a file containing the linguistics configuration. */
        std::string                                 lingoConfigurationFileName;

        /** @brief Command-line to execute that returns a linguistics configuration */
        std::string                                 lingoConfigurationFileCommand;

        /** @brief Number of seconds between checks to see if the linguistics configuration has been updated.  Default is 60.*/
        int                                         lingoConfigurationFileCheckSecs;

        /** @brief Details for producing a status report. @see LingoServerStatusReportConfiguration */
        LingoServerStatusReportConfiguration        statusReport;

        /** @brief Details concerning the server's interaction with an external health-checker such as a load-balancer.  @see ExternalHealthCheckResponder */
        ExternalHealthCheckResponder                externalHealthCheckResponder;

        /** @brief Internal settings */
        LingoServerInternals                        internals;

        /** @brief Path to the certificate store */
        std::string                                 certStoreFileName;

        /** @brief Hex password for the certificate store (if any) */
        std::string                                 certStorePasswordHex;

        /** @brief The policy to be used for the underlying Engage Engine */
        EnginePolicy                                enginePolicy;

        /** @brief Name to use for signalling a configuration check */
        std::string                                 configurationCheckSignalName;

        /** @brief [Optional] Settings for the FIPS crypto. */
        FipsCryptoSettings                          fipsCrypto;

        /** @brief Address and port of the proxy */
        NetworkAddress                              proxy;

        /** @brief [Optional] Settings for NSM. */
        NsmConfiguration                            nsm;

        LingoServerConfiguration()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            serviceConfigurationFileCheckSecs = 60;
            lingoConfigurationFileName.clear();
            lingoConfigurationFileCommand.clear();
            lingoConfigurationFileCheckSecs = 60;
            statusReport.clear();
            externalHealthCheckResponder.clear();
            internals.clear();
            certStoreFileName.clear();
            certStorePasswordHex.clear();
            enginePolicy.clear();
            configurationCheckSignalName = "rts.22f4ec3.${id}";
            fipsCrypto.clear();
            proxy.clear();
            nsm.clear();
        }
    };

    static void to_json(nlohmann::json& j, const LingoServerConfiguration& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(serviceConfigurationFileCheckSecs),
            TOJSON_IMPL(lingoConfigurationFileName),
            TOJSON_IMPL(lingoConfigurationFileCommand),
            TOJSON_IMPL(lingoConfigurationFileCheckSecs),
            TOJSON_IMPL(statusReport),
            TOJSON_IMPL(externalHealthCheckResponder),
            TOJSON_IMPL(internals),
            TOJSON_IMPL(certStoreFileName),
            TOJSON_IMPL(certStorePasswordHex),
            TOJSON_IMPL(enginePolicy),
            TOJSON_IMPL(configurationCheckSignalName),
            TOJSON_IMPL(fipsCrypto),
            TOJSON_IMPL(proxy),
            TOJSON_IMPL(nsm)
        };
    }
    static void from_json(const nlohmann::json& j, LingoServerConfiguration& p)
    {
        p.clear();
        getOptional<std::string>("id", p.id, j);
        getOptional<int>("serviceConfigurationFileCheckSecs", p.serviceConfigurationFileCheckSecs, j, 60);
        getOptional<std::string>("lingoConfigurationFileName", p.lingoConfigurationFileName, j);
        getOptional<std::string>("lingoConfigurationFileCommand", p.lingoConfigurationFileCommand, j);
        getOptional<int>("lingoConfigurationFileCheckSecs", p.lingoConfigurationFileCheckSecs, j, 60);
        getOptional<LingoServerStatusReportConfiguration>("statusReport", p.statusReport, j);
        getOptional<ExternalHealthCheckResponder>("externalHealthCheckResponder", p.externalHealthCheckResponder, j);
        getOptional<LingoServerInternals>("internals", p.internals, j);
        getOptional<std::string>("certStoreFileName", p.certStoreFileName, j);
        getOptional<std::string>("certStorePasswordHex", p.certStorePasswordHex, j);
        j.at("enginePolicy").get_to(p.enginePolicy);
        getOptional<std::string>("configurationCheckSignalName", p.configurationCheckSignalName, j, "rts.22f4ec3.${id}");
        getOptional<FipsCryptoSettings>("fipsCrypo", p.fipsCrypto, j);
        getOptional<NetworkAddress>("proxy", p.proxy, j);
        getOptional<NsmConfiguration>("nsm", p.nsm, j);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(VoiceToVoiceSession)
    /**
    * @brief Voice to voice session settings
    *
    * Helper C++ class to serialize and de-serialize VoiceToVoiceSession JSON
    *
    * Example: @include[doc] examples/VoiceToVoiceSession.json
    *
    * @see TODO: ConfigurationObjects::VoiceToVoiceSession
    */
    class VoiceToVoiceSession : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(VoiceToVoiceSession)

    public:
        /** @brief ID */
        std::string                 id;

        /** @brief Name */
        std::string                 name;

        /** @brief List of group IDs to be included in the session */
        std::vector<std::string>    groups;

        /** @brief [Optional, Default: true] Enable the session */
        bool                        enabled;

        VoiceToVoiceSession()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            name.clear();
            groups.clear();
            enabled = true;
        }
    };

    static void to_json(nlohmann::json& j, const VoiceToVoiceSession& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(name),
            TOJSON_IMPL(groups),
            TOJSON_IMPL(enabled)
        };
    }
    static void from_json(const nlohmann::json& j, VoiceToVoiceSession& p)
    {
        p.clear();
        FROMJSON_IMPL(id, std::string, EMPTY_STRING);
        FROMJSON_IMPL(name, std::string, EMPTY_STRING);
        getOptional<std::vector<std::string>>("groups", p.groups, j);
        FROMJSON_IMPL(enabled, bool, true);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(LingoConfiguration)
    /**
    * @brief Lingo configuration
    *
    * Helper C++ class to serialize and de-serialize LingoConfiguration JSON
    *
    * Example: @include[doc] examples/LingoConfiguration.json
    *
    * @see TODO: ConfigurationObjects::LingoConfiguration
    */
    class LingoConfiguration : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(LingoConfiguration)

    public:
        /** @brief Array of voiceToVoice sessions in the configuration */
        std::vector<VoiceToVoiceSession>         voiceToVoiceSessions;

        /** @brief Array of groups in the configuration */
        std::vector<Group>                       groups;

        LingoConfiguration()
        {
            clear();
        }

        void clear()
        {
            voiceToVoiceSessions.clear();
            groups.clear();
        }
    };

    static void to_json(nlohmann::json& j, const LingoConfiguration& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(voiceToVoiceSessions),
            TOJSON_IMPL(groups)
        };
    }
    static void from_json(const nlohmann::json& j, LingoConfiguration& p)
    {
        p.clear();
        getOptional<std::vector<VoiceToVoiceSession>>("voiceToVoiceSessions", p.voiceToVoiceSessions, j);
        getOptional<std::vector<Group>>("groups", p.groups, j);
    }
    
    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(BridgingConfiguration)
    /**
    * @brief Bridging configuration
    *
    * Helper C++ class to serialize and de-serialize BridgingConfiguration JSON
    *
    * Example: @include[doc] examples/BridgingConfiguration.json
    *
    * @see TODO: ConfigurationObjects::BridgingConfiguration
    */
    class BridgingConfiguration : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(BridgingConfiguration)

    public:
        /** @brief Array of bridges in the configuration */
        std::vector<Bridge>         bridges;

        /** @brief Array of bridges in the configuration */
        std::vector<Group>          groups;

        BridgingConfiguration()
        {
            clear();
        }

        void clear()
        {
            bridges.clear();
            groups.clear();
        }
    };

    static void to_json(nlohmann::json& j, const BridgingConfiguration& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(bridges),
            TOJSON_IMPL(groups)
        };
    }
    static void from_json(const nlohmann::json& j, BridgingConfiguration& p)
    {
        p.clear();
        getOptional<std::vector<Bridge>>("bridges", p.bridges, j);
        getOptional<std::vector<Group>>("groups", p.groups, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(BridgingServerStatusReportConfiguration)
    /**
    * @brief TODO: Configuration for the bridging server status report file
    *
    * Helper C++ class to serialize and de-serialize BridgingServerStatusReportConfiguration JSON
    *
    * Example: @include[doc] examples/BridgingServerStatusReportConfiguration.json
    *
    * @see RallypointServer
    */
    class BridgingServerStatusReportConfiguration : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(BridgingServerStatusReportConfiguration)

    public:
        /** File name to use for the status report. */
        std::string                     fileName;

        /** [Optional, Default: 30] The interval at which to write out the status report to file. */
        int                             intervalSecs;

        /** [Optional, Default: false] Indicates if status reporting is enabled. */
        bool                            enabled;

        /** [Optional, Default: null] Command to be executed every time the status report is produced. */
        std::string                     runCmd;

        /** [Optional, Default: false] Indicates whether to include details of each group. */
        bool                            includeGroupDetail;

        /** [Optional, Default: false] Indicates whether to include details of each bridge. */
        bool                            includeBridgeDetail;

        /** [Optional, Default: false] Indicates whether to include details of each group in each bridge. */
        bool                            includeBridgeGroupDetail;

        BridgingServerStatusReportConfiguration()
        {
            clear();
        }

        void clear()
        {
            fileName.clear();
            intervalSecs = 60;
            enabled = false;
            includeGroupDetail = false;
            includeBridgeDetail = false;
            includeBridgeGroupDetail = false;
            runCmd.clear();
        }
    };

    static void to_json(nlohmann::json& j, const BridgingServerStatusReportConfiguration& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(fileName),
            TOJSON_IMPL(intervalSecs),
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(includeGroupDetail),
            TOJSON_IMPL(includeBridgeDetail),
            TOJSON_IMPL(includeBridgeGroupDetail),
            TOJSON_IMPL(runCmd)
        };
    }
    static void from_json(const nlohmann::json& j, BridgingServerStatusReportConfiguration& p)
    {
        p.clear();
        getOptional<std::string>("fileName", p.fileName, j);
        getOptional<int>("intervalSecs", p.intervalSecs, j, 60);
        getOptional<bool>("enabled", p.enabled, j, false);
        getOptional<std::string>("runCmd", p.runCmd, j);
        getOptional<bool>("includeGroupDetail", p.includeGroupDetail, j, false);
        getOptional<bool>("includeBridgeDetail", p.includeBridgeDetail, j, false);
        getOptional<bool>("includeBridgeGroupDetail", p.includeBridgeGroupDetail, j, false);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(BridgingServerInternals)
    /**
    * @brief Internal bridging server settings
    *
    * These settings are used to configure internal parameters.
    *
    * Helper C++ class to serialize and de-serialize BridgingServerInternals JSON
    *
    * Example: @include[doc] examples/BridgingServerInternals.json
    *
    * @see engageInitialize, ConfigurationObjects::BridgingServerConfiguration
    */
    class BridgingServerInternals : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(BridgingServerInternals)

    public:
        /** @brief [Optional] Settings for the watchdog. */
        WatchdogSettings    watchdog;

        /** @brief [Optional, Default: 1000] Interval at which to run the housekeeper thread. */
        int                 housekeeperIntervalMs;

        BridgingServerInternals()
        {
            clear();
        }

        void clear()
        {
            watchdog.clear();
            housekeeperIntervalMs = 1000;
        }
    };

    static void to_json(nlohmann::json& j, const BridgingServerInternals& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(watchdog),
            TOJSON_IMPL(housekeeperIntervalMs)
        };
    }
    static void from_json(const nlohmann::json& j, BridgingServerInternals& p)
    {
        p.clear();
        getOptional<WatchdogSettings>("watchdog", p.watchdog, j);
        getOptional<int>("housekeeperIntervalMs", p.housekeeperIntervalMs, j, 1000);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(BridgingServerConfiguration)
    /**
    * @brief Configuration for the bridging server
    *
    * Helper C++ class to serialize and de-serialize BridgingServerConfiguration JSON
    *
    * Example: @include[doc] examples/BridgingServerConfiguration.json
    *
    */
    class BridgingServerConfiguration : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(BridgingServerConfiguration)

    public:
        /** @brief Enum describing the modes the briging service runs in. */
        typedef enum
        {
            /** @brief Raw mode (default) - packet payloads are not accessed or modified and forwarded as raw packets */
            omRaw                       = 0,

            /** @brief Audio payloads are transformed, headers are preserved, multiple parallel output streams are possible/expected */
            omPayloadTransformation     = 1,

            /** @brief Audio payloads are mixed - output is anonymous (i.e. no metadata) if if the target group(s) allow header extensions */
            omAnonymousMixing           = 2,

            /** @brief Audio payloads are translated between group-specific languages */
            omLanguageTranslation       = 3
        } OpMode_t;

        /** @brief A unqiue identifier for the bridge server */
        std::string                                 id;

        /** @brief Specifies the operation mode (see @ref OpMode_t). */
        OpMode_t                                    mode;

        /** @brief Number of seconds between checks to see if the service configuration has been updated.  Default is 60.*/
        int                                         serviceConfigurationFileCheckSecs;
        
        /** @brief Name of a file containing the bridging configuration. */
        std::string                                 bridgingConfigurationFileName;

        /** @brief Command-line to execute that returns a bridging configuration */
        std::string                                 bridgingConfigurationFileCommand;

        /** @brief Number of seconds between checks to see if the bridging configuration has been updated.  Default is 60.*/
        int                                         bridgingConfigurationFileCheckSecs;

        /** @brief Details for producing a status report. @see BridgingServerStatusReportConfiguration */
        BridgingServerStatusReportConfiguration     statusReport;

        /** @brief Details concerning the server's interaction with an external health-checker such as a load-balancer.  @see ExternalHealthCheckResponder */
        ExternalHealthCheckResponder                 externalHealthCheckResponder;

        /** @brief Internal settings */
        BridgingServerInternals                     internals;

        /** @brief Path to the certificate store */
        std::string                                 certStoreFileName;

        /** @brief Hex password for the certificate store (if any) */
        std::string                                 certStorePasswordHex;

        /** @brief The policy to be used for the underlying Engage Engine */
        EnginePolicy                                enginePolicy;

        /** @brief Name to use for signalling a configuration check */
        std::string                                 configurationCheckSignalName;

        /** @brief [Optional] Settings for the FIPS crypto. */
        FipsCryptoSettings                          fipsCrypto;

        /** @brief [Optional] Settings for NSM. */
        NsmConfiguration                            nsm;

        BridgingServerConfiguration()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            mode = omRaw;
            serviceConfigurationFileCheckSecs = 60;
            bridgingConfigurationFileName.clear();
            bridgingConfigurationFileCommand.clear();
            bridgingConfigurationFileCheckSecs = 60;
            statusReport.clear();
            externalHealthCheckResponder.clear();
            internals.clear();
            certStoreFileName.clear();
            certStorePasswordHex.clear();
            enginePolicy.clear();
            configurationCheckSignalName = "rts.6cc0651.${id}";
            fipsCrypto.clear();
            nsm.clear();
        }
    };

    static void to_json(nlohmann::json& j, const BridgingServerConfiguration& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(mode),
            TOJSON_IMPL(serviceConfigurationFileCheckSecs),
            TOJSON_IMPL(bridgingConfigurationFileName),
            TOJSON_IMPL(bridgingConfigurationFileCommand),
            TOJSON_IMPL(bridgingConfigurationFileCheckSecs),
            TOJSON_IMPL(statusReport),
            TOJSON_IMPL(externalHealthCheckResponder),
            TOJSON_IMPL(internals),
            TOJSON_IMPL(certStoreFileName),
            TOJSON_IMPL(certStorePasswordHex),
            TOJSON_IMPL(enginePolicy),
            TOJSON_IMPL(configurationCheckSignalName),
            TOJSON_IMPL(fipsCrypto),
            TOJSON_IMPL(nsm)
        };
    }
    static void from_json(const nlohmann::json& j, BridgingServerConfiguration& p)
    {
        p.clear();
        getOptional<std::string>("id", p.id, j);
        getOptional<BridgingServerConfiguration::OpMode_t>("mode", p.mode, j, BridgingServerConfiguration::OpMode_t::omRaw);
        getOptional<int>("serviceConfigurationFileCheckSecs", p.serviceConfigurationFileCheckSecs, j, 60);
        getOptional<std::string>("bridgingConfigurationFileName", p.bridgingConfigurationFileName, j);
        getOptional<std::string>("bridgingConfigurationFileCommand", p.bridgingConfigurationFileCommand, j);
        getOptional<int>("bridgingConfigurationFileCheckSecs", p.bridgingConfigurationFileCheckSecs, j, 60);
        getOptional<BridgingServerStatusReportConfiguration>("statusReport", p.statusReport, j);
        getOptional<ExternalHealthCheckResponder>("externalHealthCheckResponder", p.externalHealthCheckResponder, j);
        getOptional<BridgingServerInternals>("internals", p.internals, j);
        getOptional<std::string>("certStoreFileName", p.certStoreFileName, j);
        getOptional<std::string>("certStorePasswordHex", p.certStorePasswordHex, j);
        j.at("enginePolicy").get_to(p.enginePolicy);
        getOptional<std::string>("configurationCheckSignalName", p.configurationCheckSignalName, j, "rts.6cc0651.${id}");
        getOptional<FipsCryptoSettings>("fipsCrypto", p.fipsCrypto, j);
        getOptional<NsmConfiguration>("nsm", p.nsm, j);
    }


    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(EarGroupsConfiguration)
    /**
    * @brief Ear configuration
    *
    * Helper C++ class to serialize and de-serialize EarGroupsConfiguration JSON
    *
    * Example: @include[doc] examples/EarGroupsConfiguration.json
    *
    * @see TODO: ConfigurationObjects::EarGroupsConfiguration
    */
    class EarGroupsConfiguration : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(EarGroupsConfiguration)

    public:
        /** @brief Array of groups in the configuration */
        std::vector<Group>          groups;

        EarGroupsConfiguration()
        {
            clear();
        }

        void clear()
        {
            groups.clear();
        }
    };

    static void to_json(nlohmann::json& j, const EarGroupsConfiguration& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(groups)
        };
    }
    static void from_json(const nlohmann::json& j, EarGroupsConfiguration& p)
    {
        p.clear();
        getOptional<std::vector<Group>>("groups", p.groups, j);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(EarServerStatusReportConfiguration)
    /**
    * @brief TODO: Configuration for the ear server status report file
    *
    * Helper C++ class to serialize and de-serialize EarServerStatusReportConfiguration JSON
    *
    * Example: @include[doc] examples/EarServerStatusReportConfiguration.json
    *
    * @see RallypointServer
    */
    class EarServerStatusReportConfiguration : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(EarServerStatusReportConfiguration)

    public:
        /** File name to use for the status report. */
        std::string                     fileName;

        /** [Optional, Default: 30] The interval at which to write out the status report to file. */
        int                             intervalSecs;

        /** [Optional, Default: false] Indicates if status reporting is enabled. */
        bool                            enabled;

        /** [Optional, Default: null] Command to be executed every time the status report is produced. */
        std::string                     runCmd;

        /** [Optional, Default: false] Indicates whether to include details of each group. */
        bool                            includeGroupDetail;

        EarServerStatusReportConfiguration()
        {
            clear();
        }

        void clear()
        {
            fileName.clear();
            intervalSecs = 60;
            enabled = false;
            includeGroupDetail = false;
            runCmd.clear();
        }
    };

    static void to_json(nlohmann::json& j, const EarServerStatusReportConfiguration& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(fileName),
            TOJSON_IMPL(intervalSecs),
            TOJSON_IMPL(enabled),
            TOJSON_IMPL(includeGroupDetail),
            TOJSON_IMPL(runCmd)
        };
    }
    static void from_json(const nlohmann::json& j, EarServerStatusReportConfiguration& p)
    {
        p.clear();
        getOptional<std::string>("fileName", p.fileName, j);
        getOptional<int>("intervalSecs", p.intervalSecs, j, 60);
        getOptional<bool>("enabled", p.enabled, j, false);
        getOptional<std::string>("runCmd", p.runCmd, j);
        getOptional<bool>("includeGroupDetail", p.includeGroupDetail, j, false);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(EarServerInternals)
    /**
    * @brief Internal ear server settings
    *
    * These settings are used to configure internal parameters.
    *
    * Helper C++ class to serialize and de-serialize EarServerInternals JSON
    *
    * Example: @include[doc] examples/EarServerInternals.json
    *
    * @see engageInitialize, ConfigurationObjects::EarServerConfiguration
    */
    class EarServerInternals : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(EarServerInternals)

    public:
        /** @brief [Optional] Settings for the EAR's watchdog. */
        WatchdogSettings    watchdog;

        /** @brief [Optional, Default: 1000] Interval at which to run the housekeeper thread. */
        int                 housekeeperIntervalMs;

        EarServerInternals()
        {
            clear();
        }

        void clear()
        {
            watchdog.clear();
            housekeeperIntervalMs = 1000;
        }
    };

    static void to_json(nlohmann::json& j, const EarServerInternals& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(watchdog),
            TOJSON_IMPL(housekeeperIntervalMs)
        };
    }
    static void from_json(const nlohmann::json& j, EarServerInternals& p)
    {
        p.clear();
        getOptional<WatchdogSettings>("watchdog", p.watchdog, j);
        getOptional<int>("housekeeperIntervalMs", p.housekeeperIntervalMs, j, 1000);
    }

    //-----------------------------------------------------------
    JSON_SERIALIZED_CLASS(EarServerConfiguration)
    /**
    * @brief Configuration for the ear server
    *
    * Helper C++ class to serialize and de-serialize EarServerConfiguration JSON
    *
    * Example: @include[doc] examples/EarServerConfiguration.json
    *
    */
    class EarServerConfiguration : public ConfigurationObjectBase
    {
        IMPLEMENT_JSON_SERIALIZATION()
        IMPLEMENT_JSON_DOCUMENTATION(EarServerConfiguration)

    public:

        /** @brief A unqiue identifier for the EAR server */
        std::string                                 id;

        /** @brief Number of seconds between checks to see if the service configuration has been updated.  Default is 60.*/
        int                                         serviceConfigurationFileCheckSecs;
        
        /** @brief Name of a file containing the ear configuration. */
        std::string                                 groupsConfigurationFileName;

        /** @brief Command-line to execute that returns a configuration */
        std::string                                 groupsConfigurationFileCommand;

        /** @brief Number of seconds between checks to see if the configuration has been updated.  Default is 60.*/
        int                                         groupsConfigurationFileCheckSecs;

        /** @brief Details for producing a status report. @see EarServerStatusReportConfiguration */
        EarServerStatusReportConfiguration          statusReport;

        /** @brief Details concerning the server's interaction with an external health-checker such as a load-balancer.  @see ExternalHealthCheckResponder */
        ExternalHealthCheckResponder                 externalHealthCheckResponder;

        /** @brief Internal settings */
        EarServerInternals                           internals;

        /** @brief Path to the certificate store */
        std::string                                  certStoreFileName;

        /** @brief Hex password for the certificate store (if any) */
        std::string                                  certStorePasswordHex;

        /** @brief The policy to be used for the underlying Engage Engine */
        EnginePolicy                                 enginePolicy;

        /** @brief Name to use for signalling a configuration check */
        std::string                                 configurationCheckSignalName;

        /** @brief [Optional] Settings for the FIPS crypto. */
        FipsCryptoSettings                          fipsCrypto;

        /** @brief [Optional] Settings for NSM. */
        NsmConfiguration                            nsm;

        EarServerConfiguration()
        {
            clear();
        }

        void clear()
        {
            id.clear();
            serviceConfigurationFileCheckSecs = 60;
            groupsConfigurationFileName.clear();
            groupsConfigurationFileCommand.clear();
            groupsConfigurationFileCheckSecs = 60;
            statusReport.clear();
            externalHealthCheckResponder.clear();
            internals.clear();
            certStoreFileName.clear();
            certStorePasswordHex.clear();
            enginePolicy.clear();
            configurationCheckSignalName = "rts.9a164fa.${id}";
            fipsCrypto.clear();
            nsm.clear();
        }
    };

    static void to_json(nlohmann::json& j, const EarServerConfiguration& p)
    {
        j = nlohmann::json{
            TOJSON_IMPL(id),
            TOJSON_IMPL(serviceConfigurationFileCheckSecs),
            TOJSON_IMPL(groupsConfigurationFileName),
            TOJSON_IMPL(groupsConfigurationFileCommand),
            TOJSON_IMPL(groupsConfigurationFileCheckSecs),
            TOJSON_IMPL(statusReport),
            TOJSON_IMPL(externalHealthCheckResponder),
            TOJSON_IMPL(internals),
            TOJSON_IMPL(certStoreFileName),
            TOJSON_IMPL(certStorePasswordHex),
            TOJSON_IMPL(enginePolicy),
            TOJSON_IMPL(configurationCheckSignalName),
            TOJSON_IMPL(fipsCrypto),
            TOJSON_IMPL(nsm)
        };
    }
    static void from_json(const nlohmann::json& j, EarServerConfiguration& p)
    {
        p.clear();
        getOptional<std::string>("id", p.id, j);
        getOptional<int>("serviceConfigurationFileCheckSecs", p.serviceConfigurationFileCheckSecs, j, 60);
        getOptional<std::string>("groupsConfigurationFileName", p.groupsConfigurationFileName, j);
        getOptional<std::string>("groupsConfigurationFileCommand", p.groupsConfigurationFileCommand, j);
        getOptional<int>("groupsConfigurationFileCheckSecs", p.groupsConfigurationFileCheckSecs, j, 60);
        getOptional<EarServerStatusReportConfiguration>("statusReport", p.statusReport, j);
        getOptional<ExternalHealthCheckResponder>("externalHealthCheckResponder", p.externalHealthCheckResponder, j);
        getOptional<EarServerInternals>("internals", p.internals, j);
        getOptional<std::string>("certStoreFileName", p.certStoreFileName, j);
        getOptional<std::string>("certStorePasswordHex", p.certStorePasswordHex, j);
        j.at("enginePolicy").get_to(p.enginePolicy);
        getOptional<std::string>("configurationCheckSignalName", p.configurationCheckSignalName, j, "rts.9a164fa.${id}");
        getOptional<FipsCryptoSettings>("fipsCrypto", p.fipsCrypto, j);
        getOptional<NsmConfiguration>("nsm", p.nsm, j);
    }

    //-----------------------------------------------------------
    static inline void dumpExampleConfigurations(const char *path)
    {
        WatchdogSettings::document();
        FileRecordingRequest::document();
        Feature::document();
        Featureset::document();
        Agc::document();
        RtpPayloadTypeTranslation::document();
        NetworkInterfaceDevice::document();
        ListOfNetworkInterfaceDevice::document();
        RtpHeader::document();
        BlobInfo::document();
        TxAudioUri::document();
        AdvancedTxParams::document();
        Identity::document();
        Location::document();
        Power::document();
        Connectivity::document();
        PresenceDescriptorGroupItem::document();
        PresenceDescriptor::document();
        NetworkTxOptions::document();
        TcpNetworkTxOptions::document();
        NetworkAddress::document();
        NetworkAddressRxTx::document();
        NetworkAddressRestrictionList::document();
        StringRestrictionList::document();
        Rallypoint::document();
        RallypointCluster::document();
        NetworkDeviceDescriptor::document();
        TxAudio::document();
        AudioDeviceDescriptor::document();
        ListOfAudioDeviceDescriptor::document();
        Audio::document();
        TalkerInformation::document();
        GroupTalkers::document();
        Presence::document();
        Advertising::document();
        GroupPriorityTranslation::document();
        GroupTimeline::document();
        GroupSatPaq::document();
        GroupLynQPro::document();
        RtpProfile::document();
        Group::document();
        Mission::document();
        LicenseDescriptor::document();
        EngineNetworkingRpUdpStreaming::document();
        EnginePolicyNetworking::document();
        Aec::document();
        Vad::document();
        Bridge::document();
        AndroidAudio::document();
        EnginePolicyAudio::document();
        SecurityCertificate::document();
        EnginePolicySecurity::document();
        EnginePolicyLogging::document();
        EnginePolicyDatabase::document();
        NamedAudioDevice::document();
        EnginePolicyNamedAudioDevices::document();
        Licensing::document();
        DiscoveryMagellan::document();
        DiscoverySsdp::document();
        DiscoverySap::document();
        DiscoveryCistech::document();
        DiscoveryTrellisware::document();
        DiscoveryConfiguration::document();
        EnginePolicyInternals::document();
        EnginePolicyTimelines::document();
        RtpMapEntry::document();
        ExternalModule::document();
        ExternalCodecDescriptor::document();
        EnginePolicy::document();
        TalkgroupAsset::document();
        EngageDiscoveredGroup::document();
        RallypointPeer::document();
        RallypointServerLimits::document();
        RallypointServerStatusReportConfiguration::document();
        RallypointServerLinkGraph::document();
        ExternalHealthCheckResponder::document();
        Tls::document();
        PeeringConfiguration::document();
        IgmpSnooping::document();
        RallypointReflector::document();
        RallypointUdpStreaming::document();
        RallypointServer::document();
        PlatformDiscoveredService::document();
        TimelineQueryParameters::document();
        CertStoreCertificate::document();
        CertStore::document();
        CertStoreCertificateElement::document();
        CertStoreDescriptor::document();
        CertificateDescriptor::document();
        BridgeCreationDetail::document();
        GroupConnectionDetail::document();
        GroupTxDetail::document();
        GroupCreationDetail::document();
        GroupReconfigurationDetail::document();
        GroupHealthReport::document();
        InboundProcessorStats::document();
        TrafficCounter::document();
        GroupStats::document();
        RallypointConnectionDetail::document();
        BridgingConfiguration::document();
        BridgingServerStatusReportConfiguration::document();
        BridgingServerInternals::document();
        BridgingServerConfiguration::document();
        EarGroupsConfiguration::document();
        EarServerStatusReportConfiguration::document();
        EarServerInternals::document();
        EarServerConfiguration::document();
        RangerPackets::document();
        TransportImpairment::document();
    }
}

#ifndef WIN32
    #pragma GCC diagnostic pop
#endif

#endif /* ConfigurationObjects_h */
