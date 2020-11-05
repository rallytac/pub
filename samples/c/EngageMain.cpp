//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//
//  PLEASE NOTE: This code is used internally at Rally Tactical Systems as a test harness
//               for Engine development.  As such, it is NOT meant for production use NOR
//               is it meant as a representation of best-practises on how to use the Engine.
//
//               However, we provide this code to our partners in the hope that it will serve
//               to demonstrate how to call the Engage Engine API.
//
//               This code is designed primarily to compile in our internal build environment
//               with access to internal source code.  If you do not have access to that internal
//               source code, simply make sure that HAVE_RTS_INTERNAL_SOURCE_CODE is *NOT* defined as
//               per below.
//

// Just in case
#undef HAVE_RTS_INTERNAL_SOURCE_CODE

// If you are not building this code in the RTS internal build environment, you 
// must comment out the following line
#define HAVE_RTS_INTERNAL_SOURCE_CODE

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <chrono>
#include <thread>
#include <list>
#include <vector>
#include <map>
#include <mutex>

#include "EngageInterface.h"
#include "ConfigurationObjects.h"
#include "EngageAudioDevice.h"

#if defined(HAVE_RTS_INTERNAL_SOURCE_CODE)
    #include "Utils.hpp"
    #include "Version.h"

    #if defined(__APPLE__)
        #include "License.hpp"
    #endif

    #if defined(RTS_HAVE_DBUS)
        #include "SimpleDbusSignalListener.hpp"
    #endif
#endif

class GroupInfo
{
public:

    int         type;
    std::string id;
    std::string name;
    std::string jsonConfiguration;
    bool        isEncrypted;
    bool        allowsFullDuplex;

    bool        created;
    bool        createFailed;
    bool        deleted;

    bool        joined;
    bool        joinFailed;
    bool        notJoined;

    bool        connected;
    bool        connectFailed;
    bool        notConnected;

    bool        rxStarted;
    bool        rxEnded;

    bool        txStarted;
    bool        txEnded;
    bool        txFailed;

    GroupInfo()
    {
        type = 0;
        id.clear();
        name.clear();
        jsonConfiguration.clear();
        isEncrypted = false;
        allowsFullDuplex = false;

        created = false;
        createFailed = false;
        deleted = false;

        joined = false;
        joinFailed = false;
        notJoined = true;

        connected = false;
        connectFailed = false;
        notConnected = true;

        rxStarted = false;
        rxEnded = false;

        txStarted = false;
        txEnded = false;
        txFailed = false;
    }
};

const size_t MAX_CMD_BUFF_SIZE = 4096;

std::vector<GroupInfo>    g_new_groups;
std::map<std::string, ConfigurationObjects::PresenceDescriptor>    g_nodes;

int g_txPriority = 0;
uint32_t g_txFlags = 0;
bool g_anonymous = false;
bool g_useadad = false;
const char *g_adadMicFile = nullptr;
const char *g_adadSpkFile = nullptr;
ConfigurationObjects::EnginePolicy g_enginePolicy;
ConfigurationObjects::Mission g_mission;
ConfigurationObjects::Rallypoint *g_rallypoint = nullptr;
char g_szUsepLdonfigAlias[16 +1] = {0};
char g_szUsepLdonfigUserId[64 +1] = {0};
char g_szUsepLdonfigDisplayName[64 + 1] = {0};
bool g_verboseScript = false;
const char *g_pszPresenceFile = "junk/pres1.json";
const char *g_desiredSpeakerName = nullptr;
const char *g_desiredMicrophoneName = nullptr;
bool g_engineStarted = false;
bool g_engineStopped = true;

int16_t                             g_speakerDeviceId = 0;
int16_t                             g_microphoneDeviceId = 0;
int16_t                             g_nextAudioDeviceInstanceId = 0;

int16_t                             g_desiredSpeakerDeviceId = 0;
int16_t                             g_desiredMicrophoneDeviceId = 0;

bool processCommandBuffer(char *buff);
void showUsage();
void showHelp();
void showGroups();
void showNodes();
bool registepLdallbacks();
bool loadScript(const char *pszFile);
bool runScript();
bool loadPolicy(const char *pszFn, ConfigurationObjects::EnginePolicy *pPolicy);
bool loadMission(const char *pszFn, ConfigurationObjects::Mission *pMission, ConfigurationObjects::Rallypoint *pRp);
bool loadRp(const char *pszFn, ConfigurationObjects::Rallypoint *pRp);


void doStartEngine();
void doStopEngine();
void doUpdatePresence(int index);
void doCreate(int index);
void doDelete(int index);
void doJoin(int index);
void doLeave(int index);
void doBeginTx(int index);
void doEndTx(int index);
void doMuteRx(int index);
void doUnmuteRx(int index);
void doMuteTx(int index);
void doUnmuteTx(int index);
void doSetGroupRxTag(int index, int tag);
void doSendBlob(int index, const uint8_t* blob, size_t size, const char *jsonParams);
void doSendRtp(int index, const uint8_t* payload, size_t size, const char *jsonParams);
void doSendRaw(int index, const uint8_t* raw, size_t size, const char *jsonParams);
void doRegisterGroupRtpHandler(int index, int payloadId);
void doUnregisterGroupRtpHandler(int index, int payloadId);
void generateMission();

void registerADAD();
void unregisterADAD();
void showAudioDevices();

int indexOfGroupId(const char *id);

#if defined(RTS_HAVE_DBUS)
class MyDbusNotifications : public SimpleDbusSignalListener::IDbusSignalNotification
{
public:
    virtual void onDbusError(const void *ctx, const char *nm, const char *msg) const
    {
        char logBuff[1024];
        sprintf_s(logBuff, sizeof(logBuff), "onDbusError [%s] [%s]", nm, msg);
        engageLogMsg(1, "MyDbusNotifications", logBuff);
    }

    virtual void onDbusConnected(const void *ctx) const
    {
        engageLogMsg(4, "MyDbusNotifications", "onDbusConnected");
    }

    virtual void onDbusDisconnected(const void *ctx) const
    {
        engageLogMsg(4, "MyDbusNotifications", "onDbusDisconnected");
    }

    virtual bool onDbusSignalReceived(const void *ctx, const char *signal) const
    {
        char logBuff[1024];
        sprintf_s(logBuff, sizeof(logBuff), "onDbusSignalReceived [%s]", signal);
        engageLogMsg(4, "MyDbusNotifications", logBuff);
        
        char buff[MAX_CMD_BUFF_SIZE];

        strcpy_s(buff, sizeof(buff), signal);
        processCommandBuffer(buff);

        return true;
    }
};

SimpleDbusSignalListener    *g_dbusReceiver = nullptr;
MyDbusNotifications         g_dbusNotificationHandler;
#endif

bool g_enableDbus = false;
std::string g_dbusConnectionName;
std::string g_dbusSignalName;

void setupDbusSignalListener()
{
    if(!g_enableDbus)
    {
        return;
    }

    #if defined(RTS_HAVE_DBUS)
        if(g_dbusConnectionName.empty())
        {
            std::cout << "******* ERROR *******: no DBUS connection name provided - dbus interface disabled" << std::endl;
            return;        
        }

        if(g_dbusSignalName.empty())
        {
            std::cout << "******* ERROR *******: no DBUS signal name provided - dbus interface disabled" << std::endl;
            return;        
        }

        std::string sinkConnectionName;
        std::string matchRule;
        std::string signalInterface;

        sinkConnectionName.assign(g_dbusConnectionName);
            sinkConnectionName.append(".sink");

        matchRule.assign("type='signal'");
        matchRule.append(",interface='");
            matchRule.append(g_dbusConnectionName);
            matchRule.append(".Type'");

        signalInterface.assign(g_dbusConnectionName);
            signalInterface.append(".Type");

        g_dbusReceiver = new SimpleDbusSignalListener(sinkConnectionName.c_str(),
                                                    matchRule.c_str(),
                                                    signalInterface.c_str(),
                                                    g_dbusSignalName.c_str(),
                                                    &g_dbusNotificationHandler,
                                                    nullptr);

        g_dbusReceiver->start();
    #else
        std::cout << "******* ERROR *******: no DBUS available on this platform - dbus interface disabled" << std::endl;
        return;        
    #endif
}

void shutdownDbusSignalListener()
{
    #if defined(RTS_HAVE_DBUS)
        if(g_dbusReceiver != nullptr)
        {
            g_dbusReceiver->stop();
            delete g_dbusReceiver;
            g_dbusReceiver = nullptr;
        }
    #endif
}

uint64_t getTickMs()
{
    return static_cast<uint64_t>(std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now())
                                    .time_since_epoch()
                                    .count());
}

bool readInput(char *buff, size_t maxSize)
{
    if( !fgets(buff, maxSize, stdin) )
    {
        return false;
    }

    char *p = (buff + strlen(buff) - 1);
    while((p >= buff) && (*p == '\n'))
    {
        *p = 0;
        p--;
    }

    return true;
}


#include "Crypto.hpp"
#include "ConfigurationObjects.h"

void devTest1()
{ 
    FILE *fp = fopen("/Global/github/shaunwork/pd.json", "rb");
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *json = new char[sz + 1];
    fread(json, 1, sz, fp);
    json[sz] = 0;
    fclose(fp);

    ConfigurationObjects::PresenceDescriptor pd;
    pd.deserialize(json);

    std::map<std::string, int>              aliasMap;
    std::map<std::string, int>::iterator    itrAlias;
    int                                     nextAliasIndex;
    
    nextAliasIndex = 0;
    for(std::vector<ConfigurationObjects::GroupAlias>::iterator itrGa = pd.groupAliases.begin();
        itrGa != pd.groupAliases.end();
        itrGa++)
    {
        itrAlias = aliasMap.find(itrGa->alias);
        if(itrAlias == aliasMap.end())
        {
            aliasMap[itrGa->alias] = nextAliasIndex;
            nextAliasIndex++;
        }
    }
}

void devTest2()
{         
    std::cout << "-------------- DATASTORE WITH NO APP PASSWORD" << std::endl;
    engageOpenCertStore("engage-default.certstore", "");

    std::string cert;
    std::string key;

    cert = Utils::readTextFile("/Global/github/pub/certificates/rtsCA.pem");
    engageSetCertStoreCertificatePem("rtsCA", cert.c_str(), nullptr);

    cert = Utils::readTextFile("/Global/github/pub/certificates/rtsFactoryDefaultEngage.pem");
    key = Utils::readTextFile("/Global/github/pub/certificates/rtsFactoryDefaultEngage.key");
    engageSetCertStoreCertificatePem("rtsFactoryDefaultEngage", cert.c_str(), key.c_str());

    std::cout << engageGetCertStoreDescriptor() << std::endl;
}

void devTest3()
{
}


int main(int argc, const char * argv[])
{
    //devTest2();
    //exit(0);

    std::cout << "---------------------------------------------------------------------------------" << std::endl;

    #if defined(HAVE_RTS_INTERNAL_SOURCE_CODE)
        std::cout << "Engage-Cmd version " << PRODUCT_VERSION << " for " << Utils::getPlatformTargetDescriptor();
        #if defined(RTS_DEBUG_BUILD)
            std::cout << " [*** DEBUG BUILD ***]";
        #endif
        std::cout << std::endl;
        
        std::cout << "Copyright (c) 2019 Rally Tactical Systems, Inc." << std::endl;
        std::cout << "Build time: " << __DATE__ << " @ " << __TIME__ << std::endl;
        std::cout << Utils::getPlatformTargetDescriptor().c_str() << std::endl;
        std::cout << "cpu:" << Utils::numberOfCpus() << std::endl;
        std::cout << "mfd:" << Utils::getMaxOpenFds() << std::endl;
    #else
        std::cout << "Engage-Cmd version";

        #if defined(RTS_DEBUG_BUILD)
            std::cout << " [*** DEBUG BUILD ***]";
        #else
            std::cout << " [*** RELEASE BUILD ***]";
        #endif

        std::cout << std::endl;
    #endif

    std::cout << "---------------------------------------------------------------------------------" << std::endl;

    int pLd;
    const char *missionFile = nullptr;
    const char *epFile = nullptr;
    const char *rpFile = nullptr;
    const char *pszScript = nullptr;
    bool haveScript = false;
    bool continueWithCmdLine = true;
    std::string nicName;    
    std::string certStore;
    std::string certStorePwd;

    #if defined(HAVE_RTS_INTERNAL_SOURCE_CODE)
        Utils::NetworkInterfaceInfo nic;
    #endif

    std::string enginePolicyJson;
    std::string userIdentityJson;

    // Check out the command line
    for(int x = 1; x < argc; x++)
    {        
        if(strncmp(argv[x], "-jsonobjects", 12) == 0)
        {
            const char *path = nullptr;
            const char *p = strchr(argv[x], ':');
            if(p != nullptr)
            {
                path = (p + 1);
            }

            ConfigurationObjects::dumpExampleConfigurations(path);
            exit(0);
        }
        else if(strncmp(argv[x], "-mission:", 9) == 0)
        {
            missionFile = argv[x] + 9;
        }        
        else if(strncmp(argv[x], "-cs:", 4) == 0)
        {
            certStore = argv[x] + 4;
        }
        else if(strncmp(argv[x], "-csp:", 5) == 0)
        {
            certStorePwd = argv[x] + 5;
        }
        else if(strncmp(argv[x], "-nic:", 5) == 0)
        {
            nicName = argv[x] + 5;
        }
        else if(strncmp(argv[x], "-ep:", 4) == 0)
        {
            epFile = argv[x] + 4;
        }        
        else if(strncmp(argv[x], "-rp:", 4) == 0)
        {
            rpFile = argv[x] + 4;
        }        
        else if(strcmp(argv[x], "-anon") == 0)
        {
            g_anonymous = true;
        }
        else if(strcmp(argv[x], "-useadad") == 0)
        {
            g_useadad = true;
        }
        else if(strncmp(argv[x], "-adadmic:", 9) == 0)
        {
            g_adadMicFile = (argv[x] + 9);
        }
        else if(strncmp(argv[x], "-adadspk:", 9) == 0)
        {
            g_adadSpkFile = (argv[x] + 9);
        }
        else if(strncmp(argv[x], "-script:", 8) == 0)
        {
            pszScript = (argv[x] + 8);
        }
        else if(strcmp(argv[x], "-verbose") == 0)
        {
            g_verboseScript = true;
        }        
        else if(strncmp(argv[x], "-ua:", 4) == 0)
        {
            memset(g_szUsepLdonfigAlias, 0, sizeof(g_szUsepLdonfigAlias));
            strncpy_s(g_szUsepLdonfigAlias, sizeof(g_szUsepLdonfigAlias), argv[x] + 4, sizeof(g_szUsepLdonfigAlias)-1);
        }        
        else if(strncmp(argv[x], "-ui:", 4) == 0)
        {
            memset(g_szUsepLdonfigUserId, 0, sizeof(g_szUsepLdonfigUserId));
			strncpy_s(g_szUsepLdonfigUserId, sizeof(g_szUsepLdonfigUserId), argv[x] + 4, sizeof(g_szUsepLdonfigUserId)-1);
        }
        else if(strncmp(argv[x], "-ud:", 4) == 0)
        {
            memset(g_szUsepLdonfigDisplayName, 0, sizeof(g_szUsepLdonfigDisplayName));
			strncpy_s(g_szUsepLdonfigDisplayName, sizeof(g_szUsepLdonfigDisplayName), argv[x] + 4, sizeof(g_szUsepLdonfigDisplayName)-1);
        }
        else if(strncmp(argv[x], "-ut:", 4) == 0)
        {
            g_txPriority = atoi(argv[x] + 4);
        }
        else if(strncmp(argv[x], "-uf:", 4) == 0)
        {
            g_txFlags = atoi(argv[x] + 4);
        }
        else if(strncmp(argv[x], "-putenv:", 8) == 0)
        {
            putenv((char*)(argv[x] + 8));
        }
        else if(strncmp(argv[x], "-sn:", 4) == 0)
        {
            g_desiredSpeakerName = (argv[x] + 4);
        }
        else if(strncmp(argv[x], "-mn:", 4) == 0)
        {
            g_desiredMicrophoneName = (argv[x] + 4);
        }
        else if(strncmp(argv[x], "-dbusconn:", 10) == 0)
        {
            g_enableDbus = true;
            g_dbusConnectionName = (argv[x] + 10);
        }
        else if(strncmp(argv[x], "-dbussig:", 9) == 0)
        {
            g_enableDbus = true;
            g_dbusSignalName = (argv[x] + 9);
        }
        else
        {
            std::cout << "unknown option '" << argv[x] << "'" << std::endl;
            showUsage();
            goto end_function;
        }
    }

    // Open a cert store if we have one
    if(!certStore.empty())
    {
        engageOpenCertStore(certStore.c_str(), certStorePwd.c_str());
    }

    // We're just going to use a simple random number generator for this app
    srand((unsigned int)time(NULL));

    if(pszScript != nullptr && pszScript[0])
    {
        haveScript = loadScript(pszScript);
        if(!haveScript)
        {
            goto end_function;
        }
    }

    // If we're not going to be anonymous then create our user configuration json
    if(!g_anonymous)
    {
        // Make a random alias if we don't have one
        if(!g_szUsepLdonfigAlias[0])
        {
            #if defined(__APPLE__)
                const char *prefix = "APL-";
            #elif defined(__ANDROID__)
                const char *prefix = "AND-";
            #elif defined(__linux__)
                const char *prefix = "LNX-";
            #elif defined(WIN32)
                const char *prefix = "WIN-";
            #else
                const char *prefix = "UNK-";    
            #endif

            sprintf_s(g_szUsepLdonfigAlias, sizeof(g_szUsepLdonfigAlias), "%s%012X", prefix, rand());
        }
        
        // Cook up a user id if we don't have one
        if(!g_szUsepLdonfigUserId[0])
        {
            sprintf_s(g_szUsepLdonfigUserId, sizeof(g_szUsepLdonfigUserId), "%s@engagedev.rallytac.com", g_szUsepLdonfigAlias);
        }

        // Cook up a display name if we don't have one
        if(!g_szUsepLdonfigDisplayName[0])
        {
            sprintf_s(g_szUsepLdonfigDisplayName, sizeof(g_szUsepLdonfigDisplayName), "User %s", g_szUsepLdonfigAlias);
        }

        // Make sure our transmit priority is a valid value
        if(g_txPriority < 0 || g_txPriority > 255)
        {
            g_txPriority = 0;
        }
    }

    if(missionFile == nullptr || missionFile[0] == 0)
    {
        std::cerr << "no mission file specified" << std::endl;
        showUsage();
        goto end_function;
    }

    // Register our callbacks
    if(!registepLdallbacks())
    {
        std::cerr << "callback registration failed" << std::endl;
        goto end_function;
    }

    // Load our engine policy (if any)
    if(epFile != nullptr)
    {
        if(!loadPolicy(epFile, &g_enginePolicy))
        {
            std::cerr << "could not load engine policy '" << epFile << "'" << std::endl;
            goto end_function;
        }        
    }

    // Load our mission
    g_rallypoint = new ConfigurationObjects::Rallypoint();
    if(!loadMission(missionFile, &g_mission, g_rallypoint))
    {
        std::cerr << "could not load mission '" << missionFile << "'" << std::endl;
        goto end_function;
    }

    // Load our RP (if any)
    if(rpFile != nullptr)
    {
        if(!loadRp(rpFile, g_rallypoint))
        {
            std::cerr << "could not load rallypoint '" << rpFile << "'" << std::endl;
            goto end_function;
        }        
    }

    if(!nicName.empty())
    {
        g_enginePolicy.networking.defaultNic = nicName;
    }

#ifdef WIN32
	if (g_enginePolicy.networking.defaultNic.empty())
	{
		std::cerr << "WARNING : The Windows version of engage-cmd does not pass the network interface name to the Engage Engine!" << std::endl;
	}
#else
    #if defined(HAVE_RTS_INTERNAL_SOURCE_CODE)
        if(g_enginePolicy.networking.defaultNic.empty())
        {
            if(!Utils::getFirstViableNic(nic, AF_INET))
            {
                std::cerr << "no viable nics found - following nics are present:" << std::endl;

                std::vector<Utils::NetworkInterfaceInfo> presentNics = Utils::getNics();
                for(size_t x = 0; x < presentNics.size(); x++)
                {
                    std::cerr << "name='" << presentNics[x]._name << "'"
                            << ", friendlyName='" << presentNics[x]._friendlyName << "'"
                            << ", description='" << presentNics[x]._description << "'"                            
                            << ", family=" << presentNics[x]._family
                            << ", address='" << presentNics[x]._address << "'"
                            << ", available=" << presentNics[x]._isAvailable
                            << ", loopBack=" << presentNics[x]._isLoopback
                            << ", supportsMulticast=" << presentNics[x]._supportsMulticast << std::endl;
                }

                return -1;
            }        
        }    
        else
        {
            if(!Utils::getNicByName(g_enginePolicy.networking.defaultNic.c_str(), AF_INET, nic))
            {
                std::cerr << "WARNING: '" << nicName.c_str() << "' not found found - following nics are present:" << std::endl;

                std::vector<Utils::NetworkInterfaceInfo> presentNics = Utils::getNics();
                for(size_t x = 0; x < presentNics.size(); x++)
                {
                    std::cerr << "name='" << presentNics[x]._name << "'"
                            << ", friendlyName='" << presentNics[x]._friendlyName << "'"
                            << ", description='" << presentNics[x]._description << "'"                            
                            << ", family=" << presentNics[x]._family
                            << ", address='" << presentNics[x]._address << "'"
                            << ", available=" << presentNics[x]._isAvailable
                            << ", loopBack=" << presentNics[x]._isLoopback
                            << ", supportsMulticast=" << presentNics[x]._supportsMulticast << std::endl;
                }

                strcpy_s(nic._name, sizeof(nic._name), nicName.c_str());
            }
        }

        std::cout << "name='" << nic._name << "'"
                    << ", friendlyName='" << nic._friendlyName << "'"
                    << ", description='" << nic._description << "'"                    
                    << ", family=" << nic._family
                    << ", address='" << nic._address << "'"
                    << ", available=" << nic._isAvailable
                    << ", loopBack=" << nic._isLoopback
                    << ", supportsMulticast=" << nic._supportsMulticast << std::endl;
    #endif
#endif

    // Build the information we'll need for our UI
    for(std::vector<ConfigurationObjects::Group>::iterator itr = g_mission.groups.begin();
        itr != g_mission.groups.end();
        itr++)
    {
        GroupInfo gi;

        gi.id = (int)itr->type;
        gi.id = itr->id;
        gi.name = itr->name;
        gi.type = itr->type;
        gi.isEncrypted = (!itr->cryptoPassword.empty());
        gi.allowsFullDuplex = itr->txAudio.fdx;

        // Get the JSON representation - it'll be needed when we create the group
        gi.jsonConfiguration = itr->serialize();

        g_new_groups.push_back(gi);
    }

    // At this point we have our group list, show them
    showGroups();

    // Create a user identity
    userIdentityJson = "{";
    if(!g_anonymous)
    {
        userIdentityJson += "\"userId\":\"";
        userIdentityJson += g_szUsepLdonfigUserId;
        userIdentityJson += "\"";

        userIdentityJson += ",\"displayName\":\"";
        userIdentityJson += g_szUsepLdonfigDisplayName;
        userIdentityJson += "\"";
    }
    userIdentityJson += "}";

    // Build our engine policy
    enginePolicyJson = g_enginePolicy.serialize();

    // Initialize the library
    pLd = engageInitialize(enginePolicyJson.c_str(), userIdentityJson.c_str(), nullptr);
    if(pLd != ENGAGE_RESULT_OK)
    {
        std::cerr << "engageInitialize failed" << std::endl;
        goto end_function;
    }

    // Fire up the dbus interface if any.  Do this here because the dbus goodies uses the Engage logger
    setupDbusSignalListener();

    // Start the Engine
    pLd = engageStart();
    if(pLd != ENGAGE_RESULT_OK)
    {
        std::cerr << "engageStart failed" << std::endl;
        goto end_function;
    }

    if(g_useadad)
    {
        registerADAD();
    }

    if(haveScript)
    {
        if( !runScript() )
        {
            continueWithCmdLine = false;
        }
    }

    // Go round and round getting commands from our console user
    while( continueWithCmdLine )
    {
        char buff[MAX_CMD_BUFF_SIZE];
        std::cout << "............running >";

        memset(buff, 0, sizeof(buff));
        if( !readInput(buff, sizeof(buff)) )
        {
            continue;
        }

        if( !processCommandBuffer(buff) )
        {
            break;
        }
    }

    unregisterADAD();

end_function:
    // Shutdown the dbus interface if any
    shutdownDbusSignalListener();

    // Stop the Engine
    engageStop();

    // Shut down the library
    pLd = engageShutdown();

    // Clean up
    g_new_groups.clear();

    if(g_rallypoint != nullptr)
    {
        delete g_rallypoint;
    }

    return 0;
}

bool processCommandBuffer(char *buff)
{
    if(buff[0] == 0)
    {
        return true;
    }

    else if(strcmp(buff, "q") == 0)
    {
        return false;
    }

    else if(strcmp(buff, "starteng") == 0)
    {
        doStartEngine();
    }

    else if(strcmp(buff, "stopeng") == 0)
    {
        doStopEngine();
    }

    else if(buff[0] == '!')
    {
        if(system(buff + 1) < 0)
        {
            std::cout << "Error executing '" << (buff + 1) << "'" << std::endl;
        }
    }

    else if(strncmp(buff, "tl", 2) == 0)
    {
        const char *p = (buff + 2);
        while( *p == ' ')
        {
            p++;
        }

        if( *p == 0 )
        {
            p = "filters/default-timeline-query.json";
        }

        if(*p != 0)
        {
            std::string filter;
            filter = Utils::readTextFile(p);
            if(!filter.empty())
            {
                engageQueryGroupTimeline(g_new_groups[1].id.c_str(), filter.c_str());
            }
            else
            {
                std::cout << "******* ERROR *******: Cannot load specified filter file" << std::endl;
            }                
        }
        else
        {
            std::cout << "NOTE: No filter file specified" << std::endl;
            engageQueryGroupTimeline(g_new_groups[1].id.c_str(), "{}");
        }
    }

    else if(strcmp(buff, "dev1") == 0)
    {
        devTest1();
    }
    else if(strcmp(buff, "dev2") == 0)
    {
        devTest2();
    }
    else if(strcmp(buff, "dev3") == 0)
    {
        devTest3();
    }

    /*
    else if(buff[0] == 'l')
    {            
        const char *msg = "hello world blob";
        size_t size = strlen(msg + 1);

        uint8_t *blob = new uint8_t[size];
        memcpy(blob, msg, size);
        std::string jsonParams;

        ConfigurationObjects::BlobInfo bi;
        bi.rtpHeader.pt = 114;
        jsonParams = bi.serialize();

        doSendBlob(buff[1] == 'a' ? -1 : atoi(buff + 1), blob, size, jsonParams.c_str());

        delete[] blob;
    }
    else if(buff[0] == 'r')
    {
        const char *msg = "hello world rtp";
        size_t size = strlen(msg + 1);

        uint8_t *payload = new uint8_t[size];
        memcpy(payload, msg, size);
        std::string jsonParams;

        ConfigurationObjects::RtpHeader rtpHeader;
        rtpHeader.pt = 109;
        jsonParams = rtpHeader.serialize();

        doSendRtp(buff[1] == 'a' ? -1 : atoi(buff + 1), payload, size, jsonParams.c_str());

        delete[] payload;
    }
    else if(buff[0] == 'w')
    {
        const char *msg = "hello world raw";
        size_t size = strlen(msg + 1);

        uint8_t *payload = new uint8_t[size];
        memcpy(payload, msg, size);
        std::string jsonParams;

        doSendRaw(buff[1] == 'a' ? -1 : atoi(buff + 1), payload, size, jsonParams.c_str());

        delete[] payload;
    }
    */
    
    else if(strcmp(buff, "?") == 0)
    {
        showHelp();
    }
    else if(strcmp(buff, "sg") == 0)
    {
        showGroups();
    }
    else if(strcmp(buff, "sn") == 0)
    {
        showNodes();
    }
    else if(strcmp(buff, "gm") == 0)
    {
        generateMission();
    }
    else if(buff[0] == 'z')
    {
        doUpdatePresence(buff[1] == 'a' ? -1 : atoi(buff + 1));
    }
    else if(buff[0] == 'c')
    {
        doCreate(buff[1] == 'a' ? -1 : atoi(buff + 1));
    }
    else if(buff[0] == 'd')
    {
        doDelete(buff[1] == 'a' ? -1 : atoi(buff + 1));
    }
    else if(buff[0] == 'j')
    {
        doJoin(buff[1] == 'a' ? -1 : atoi(buff + 1));
    }
    else if(buff[0] == 'l')
    {
        doLeave(buff[1] == 'a' ? -1 : atoi(buff + 1));
    }
    else if(buff[0] == 'p')
    {
        g_txPriority = atoi(buff + 1);
        std::cout << "tx priority set to " << g_txPriority << std::endl;
    }
    else if(buff[0] == 'f')
    {
        g_txFlags = atoi(buff + 1);
        std::cout << "tx flags set to " << g_txFlags << std::endl;
    }
    else if(buff[0] == 't')
    {
        char *val = strchr(buff, ' ');
        if(val != nullptr)
        {
            *val = 0;
            int tag = atoi(val + 1);
            doSetGroupRxTag(buff[1] == 'a' ? -1 : atoi(buff + 1), tag);
        }
    }
    else if(buff[0] == 'b')
    {
        doBeginTx(buff[1] == 'a' ? -1 : atoi(buff + 1));
    }
    else if(buff[0] == 'e')
    {
        doEndTx(buff[1] == 'a' ? -1 : atoi(buff + 1));
    }
    else if(buff[0] == 'm')
    {
        doMuteRx(buff[1] == 'a' ? -1 : atoi(buff + 1));
    }
    else if(buff[0] == 'u')
    {
        doUnmuteRx(buff[1] == 'a' ? -1 : atoi(buff + 1));
    }
    /* TODO
    else if(buff[0] == 'todo for mutetx')
    {
        doMuteTx(buff[1] == 'a' ? -1 : atoi(buff + 1));
    }
    else if(buff[0] == 'todo for unmutetx')
    {
        doUnmuteTx(buff[1] == 'a' ? -1 : atoi(buff + 1));
    }
    */
    else if(buff[0] == 'x')
    {
        char *val = strchr(buff, ' ');
        if(val != nullptr)
        {
            *val = 0;
            int tag = atoi(val + 1);
            doRegisterGroupRtpHandler(buff[1] == 'a' ? -1 : atoi(buff + 1), tag);
        }
    }
    else if(buff[0] == 'y')
    {
        char *val = strchr(buff, ' ');
        if(val != nullptr)
        {
            *val = 0;
            int tag = atoi(val + 1);
            doUnregisterGroupRtpHandler(buff[1] == 'a' ? -1 : atoi(buff + 1), tag);
        }
    }
    else
    {
        std::cout << "'" << buff << "' not recognized" << std::endl;
        showHelp();
    }

    return true;
}

int getIdOfNamedAudioDevice(const char *nm, bool forInput)
{
    int rc = 0;

    try
    {
        const char *jsonString = engageGetAudioDevices();

        ConfigurationObjects::ListOfAudioDeviceDescriptor obj;

        if(!obj.deserialize(jsonString))
        {
            throw "";
        }

        for(std::vector<ConfigurationObjects::AudioDeviceDescriptor>::iterator itr = obj.list.begin();
            itr != obj.list.end();
            itr++)
        {
            if(itr->name.compare(nm) == 0 && 
               ((forInput && itr->direction == 1) || (!forInput && itr->direction == 2)) )
            {
                rc = itr->deviceId;
                break;
            }
        }
    }
    catch(...)
    {
        rc = 0;
    }
    
    return rc;
}

void showAudioDevices()
{
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "Available audio devices" << std::endl;

    try
    {
        const char *jsonString = engageGetAudioDevices();

        ConfigurationObjects::ListOfAudioDeviceDescriptor obj;

        if(!obj.deserialize(jsonString))
        {
            throw "";
        }

        for(std::vector<ConfigurationObjects::AudioDeviceDescriptor>::iterator itr = obj.list.begin();
            itr != obj.list.end();
            itr++)
        {
            std::cout << "deviceId.............: " << itr->deviceId << std::endl <<
                         "   name..............: " << itr->name << std::endl <<
                         "   manufacturer......: " << itr->manufacturer << std::endl <<
                         "   model.............: " << itr->model << std::endl <<
                         "   hardwareId........: " << itr->hardwareId << std::endl <<
                         "   serialNumber......: " << itr->serialNumber << std::endl <<
                         "   isDefault.........: " << itr->isDefault << std::endl <<
                         "   isAdad............: " << itr->isAdad << std::endl <<
                         "   samplingRate......: " << itr->samplingRate << std::endl <<
                         "   channels..........: "<< itr->channels << std::endl <<
                         "   direction.........: " << itr->direction << std::endl <<
                         "   boostPercentage...: " << itr->boostPercentage << std::endl <<
            std::endl;
        }
    }
    catch(...)
    {
        std::cout << "******* ERROR *******: exception while processing audio devices list" << std::endl;
    }

    std::cout << "-------------------------------------------------------" << std::endl;
}

std::string buildGroupCreationJson(int index)
{
    // Parse the baseline configuration into "groupConfig"
    ConfigurationObjects::Group groupConfig;
    groupConfig.deserialize(g_new_groups[index].jsonConfiguration.c_str());

    if(!g_anonymous)
    {
        groupConfig.alias = g_szUsepLdonfigAlias;
    }
    else
    {
        groupConfig.alias.clear();
    }

    if(g_rallypoint != nullptr && g_rallypoint->host.port > 0)
    {
        groupConfig.rallypoints.push_back(*g_rallypoint);
    }

    if(g_desiredSpeakerName != nullptr)
    {
        groupConfig.audio.outputId = getIdOfNamedAudioDevice(g_desiredSpeakerName, false);
        if(groupConfig.audio.outputId == 0)
        {
            std::cout << "******* ERROR *******: cannot find requested speaker device '" << g_desiredSpeakerName << "'" << std::endl;
            showAudioDevices();
        }
    }    
    else
    {
        // We'll use our application-defined speaker audio device if we have one
        if(g_speakerDeviceId != 0)
        {
            groupConfig.audio.outputId = g_speakerDeviceId;
        }
    }

    if(g_desiredMicrophoneName != nullptr)
    {
        groupConfig.audio.inputId = getIdOfNamedAudioDevice(g_desiredMicrophoneName, true);
        if(groupConfig.audio.inputId == 0)
        {
            std::cout << "******* ERROR *******: cannot find requested microphone device '" << g_desiredMicrophoneName << "'" << std::endl;
            showAudioDevices();
        }
    }    
    else
    {
        // We'll use our application-defined microphone audio device if we have one
        if(g_microphoneDeviceId != 0)
        {
            groupConfig.audio.inputId = g_microphoneDeviceId;
        }
    }

    // Serialize to a string
    std::string rc = groupConfig.serialize();

    return rc;
}

void doStartEngine()
{
    if(g_verboseScript) std::cout << "doStartEngine" << std::endl;
    engageStart();
}

void doStopEngine()
{
    if(g_verboseScript) std::cout << "doStopEngine" << std::endl;
    engageStop();
}

void doUpdatePresence(int index)
{
    if(g_verboseScript) std::cout << "doUpdatePresence: " << index << std::endl;
    std::string jsonText;
    jsonText = Utils::readTextFile(g_pszPresenceFile);

    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            engageUpdatePresenceDescriptor(g_new_groups[x].id.c_str(), jsonText.c_str(), true);
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            engageUpdatePresenceDescriptor(g_new_groups[index].id.c_str(), jsonText.c_str(), true);
        }
        else
        {
            std::cerr << "invalid index" << std::endl;
        }                
    }
}

void doCreate(int index)
{
    if(g_verboseScript) std::cout << "doCreate: " << index << std::endl;
    std::string json;

    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            json = buildGroupCreationJson((int)x);
            engageCreateGroup(json.c_str());
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            json = buildGroupCreationJson(index);
            engageCreateGroup(json.c_str());
        }
        else
        {
            std::cerr << "invalid index" << std::endl;
        }                
    }
}

void doDelete(int index)
{
    if(g_verboseScript) std::cout << "doDelete: " << index << std::endl;
    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            engageDeleteGroup(g_new_groups[x].id.c_str());
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            engageDeleteGroup(g_new_groups[index].id.c_str());
        }
        else
        {
            std::cerr << "invalid index" << std::endl;
        }                
    }
}

void doJoin(int index)
{
    if(g_verboseScript) std::cout << "doJoin: " << index << std::endl;
    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            engageJoinGroup(g_new_groups[x].id.c_str());
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            engageJoinGroup(g_new_groups[index].id.c_str());
        }
        else
        {
            std::cerr << "invalid index" << std::endl;
        }                
    }
}

void doLeave(int index)
{
    if(g_verboseScript) std::cout << "doLeave: " << index << std::endl;
    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            engageLeaveGroup(g_new_groups[x].id.c_str());
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            engageLeaveGroup(g_new_groups[index].id.c_str());
        }
        else
        {
            std::cerr << "invalid index" << std::endl;
        }                
    }
}

void doBeginTx(int index)
{
    ConfigurationObjects::AdvancedTxParams params;
    params.flags = (uint16_t)0;
    params.priority = (uint8_t)0;
    //params.grantAudioUri = "./tx_on.wav";

    if(!g_anonymous)
    {
        params.alias = g_szUsepLdonfigUserId;
    }

    params.subchannelTag = 0;
    params.includeNodeId = true;
    params.muted = false;
    std::string json = params.serialize();

    if(g_verboseScript) std::cout << "doBeginTx: " << index << std::endl;
    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            //engageBeginGroupTx(g_new_groups[x].id.c_str(), g_txPriority, g_txFlags);
            engageBeginGroupTxAdvanced(g_new_groups[x].id.c_str(), json.c_str());
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            //engageBeginGroupTx(g_new_groups[index].id.c_str(), g_txPriority, g_txFlags);
            engageBeginGroupTxAdvanced(g_new_groups[index].id.c_str(), json.c_str());
        }
        else
        {
            std::cerr << "invalid index" << std::endl;
        }                
    }
}

void doEndTx(int index)
{
    if(g_verboseScript) std::cout << "doEndTx: " << index << std::endl;
    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            engageEndGroupTx(g_new_groups[x].id.c_str());
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            engageEndGroupTx(g_new_groups[index].id.c_str());
        }
        else
        {
            std::cerr << "invalid index" << std::endl;
        }                
    }
}

void doMuteRx(int index)
{
    if(g_verboseScript) std::cout << "doMuteRx: " << index << std::endl;
    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            engageMuteGroupRx(g_new_groups[x].id.c_str());
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            engageMuteGroupRx(g_new_groups[index].id.c_str());
        }
        else
        {
            std::cerr << "invalid index" << std::endl;
        }                
    }
}

void doUnmuteRx(int index)
{
    if(g_verboseScript) std::cout << "doUnmuteRx: " << index << std::endl;
    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            engageUnmuteGroupRx(g_new_groups[x].id.c_str());
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            engageUnmuteGroupRx(g_new_groups[index].id.c_str());
        }
        else
        {
            std::cerr << "invalid index" << std::endl;
        }                
    }
}

void doMuteTx(int index)
{
    if(g_verboseScript) std::cout << "doMuteTx: " << index << std::endl;
    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            engageMuteGroupTx(g_new_groups[x].id.c_str());
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            engageMuteGroupTx(g_new_groups[index].id.c_str());
        }
        else
        {
            std::cerr << "invalid index" << std::endl;
        }                
    }
}

void doUnmuteTx(int index)
{
    if(g_verboseScript) std::cout << "doUnmuteTx: " << index << std::endl;
    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            engageUnmuteGroupTx(g_new_groups[x].id.c_str());
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            engageUnmuteGroupTx(g_new_groups[index].id.c_str());
        }
        else
        {
            std::cerr << "invalid index" << std::endl;
        }                
    }
}

void doSetGroupRxTag(int index, int tag)
{
    if(g_verboseScript) std::cout << "doSetGroupRxTag: " << index << std::endl;
    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            engageSetGroupRxTag(g_new_groups[x].id.c_str(), tag);
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            engageSetGroupRxTag(g_new_groups[index].id.c_str(), tag);
        }
        else
        {
            std::cerr << "invalid index" << std::endl;
        }                
    }
}

void doRegisterGroupRtpHandler(int index, int payloadId)
{
    if(g_verboseScript) std::cout << "doRegisterGroupRtpHandler: " << index << std::endl;
    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            engageRegisterGroupRtpHandler(g_new_groups[x].id.c_str(), payloadId);
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            engageRegisterGroupRtpHandler(g_new_groups[index].id.c_str(), payloadId);
        }
        else
        {
            std::cerr << "invalid index" << std::endl;
        }                
    }
}

void doUnregisterGroupRtpHandler(int index, int payloadId)
{
    if(g_verboseScript) std::cout << "doUnregisterGroupRtpHandler: " << index << std::endl;
    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            engageUnregisterGroupRtpHandler(g_new_groups[x].id.c_str(), payloadId);
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            engageUnregisterGroupRtpHandler(g_new_groups[index].id.c_str(), payloadId);
        }
        else
        {
            std::cerr << "invalid index" << std::endl;
        }                
    }
}

void doSendBlob(int index, const uint8_t* blob, size_t size, const char *jsonParams)
{
    if(g_verboseScript) std::cout << "doSendBlob: " << index << std::endl;
    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            engageSendGroupBlob(g_new_groups[x].id.c_str(), blob, size, jsonParams);
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            engageSendGroupBlob(g_new_groups[index].id.c_str(), blob, size, jsonParams);
        }
        else
        {
            std::cerr << "invalid index" << std::endl;
        }                
    }
}

void doSendRtp(int index, const uint8_t* payload, size_t size, const char *jsonParams)
{
    if(g_verboseScript) std::cout << "doSendRtp: " << index << std::endl;
    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            engageSendGroupRtp(g_new_groups[x].id.c_str(), payload, size, jsonParams);
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            engageSendGroupRtp(g_new_groups[index].id.c_str(), payload, size, jsonParams);
        }
        else
        {
            std::cerr << "invalid index" << std::endl;
        }                
    }
}

void doSendRaw(int index, const uint8_t* raw, size_t size, const char *jsonParams)
{
    if(g_verboseScript) std::cout << "doSendRaw: " << index << std::endl;
    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            engageSendGroupRaw(g_new_groups[x].id.c_str(), raw, size, jsonParams);
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            engageSendGroupRaw(g_new_groups[index].id.c_str(), raw, size, jsonParams);
        }
        else
        {
            std::cerr << "invalid index" << std::endl;
        }                
    }
}

int indexOfGroupId(const char *id)
{
    for(size_t x = 0; x < g_new_groups.size(); x++)
    {
        if(g_new_groups[x].id.compare(id) == 0)
        {
            return (int) x;
        }
    }

    return -1;
}

void generateMission()
{
    char buff[MAX_CMD_BUFF_SIZE];
    int channelCount;
    std::string rallypoint;
    std::string passphrase;
    std::string fn;
    std::string name;

    std::cout << "passphrase: ";
    memset(buff, 0, sizeof(buff));
    if( !readInput(buff, sizeof(buff)) )
    {
        return;
    }
    passphrase.assign(buff);
    if(passphrase.empty())
    {
        return;
    }

    std::cout << "mission name (optional): ";
    memset(buff, 0, sizeof(buff));
    if( !readInput(buff, sizeof(buff)) )
    {
        return;
    }
    name.assign(buff);

    std::cout << "number of channels: ";
    memset(buff, 0, sizeof(buff));
    if( !readInput(buff, sizeof(buff)) )
    {
        return;
    }
    channelCount = atoi(buff);
    if(channelCount <= 0)
    {
        return;
    }


    std::cout << "rallypoint (optional): ";
    memset(buff, 0, sizeof(buff));
    if( !readInput(buff, sizeof(buff)) )
    {
        return;
    }
    rallypoint.assign(buff);
        
    std::cout << "file name (optional): ";
    memset(buff, 0, sizeof(buff));
    if( !readInput(buff, sizeof(buff)) )
    {
        return;
    }
    fn.assign(buff);

    std::string ms;

    ms = engageGenerateMission(passphrase.c_str(), channelCount, rallypoint.c_str(), name.c_str());
    if(fn.empty())
    {
        std::cout << ms << std::endl; 
    }
    else
    {
        FILE *fp;

        #ifndef WIN32
            fp = fopen(fn.c_str(), "wt");
        #else
            if (fopen_s(&fp, fn.c_str(), "wt") != 0)
            {
                fp = nullptr;
            }
        #endif

        if(fp == nullptr)
        {
            std::cerr << "cannot save to '" << fn.c_str() << "'" << std::endl;
            return;
        }

        fputs(ms.c_str(), fp);

        fclose(fp);

        std::cout << "saved to '" << fn.c_str() << "'" << std::endl; 
    }
    
}


// Application-defined audio device
class ADADInstance
{
public:
    ADADInstance(int16_t deviceId, 
                 int16_t instanceId, 
                 ConfigurationObjects::AudioDeviceDescriptor::Direction_t direction)
    {
        _deviceId = deviceId;
        _instanceId = instanceId;
        _direction = direction;
        _running = false;
        _paused = false;
    }

    int start()
    {
        if(!_running)
        {
            _running = true;
            _paused = false;
            _threadHandle = std::thread(&ADADInstance::thread, this);
        }

        return ENGAGE_AUDIO_DEVICE_RESULT_OK;
    }

    int stop()
    {
        _running = false;
        if(_threadHandle.joinable())
        {
            _threadHandle.join();
        }

        return ENGAGE_AUDIO_DEVICE_RESULT_OK;
    }

    int pause()
    {
        _paused = true;
        return ENGAGE_AUDIO_DEVICE_RESULT_OK;
    }

    int resume()
    {
        _paused = false;
        return ENGAGE_AUDIO_DEVICE_RESULT_OK;
    }

    int reset()
    {
        return ENGAGE_AUDIO_DEVICE_RESULT_OK;
    }

    int restart()
    {
        return ENGAGE_AUDIO_DEVICE_RESULT_OK;
    }

private:
    ADADInstance()
    {
        // Not to be used
    }

    void thread()
    {
        SET_THREAD_NAME("adad");

        // Our "device" will work in 60ms intervals
        const size_t  MY_AUDIO_DEVICE_INTERVAL_MS = 60;

        // The number of samples we produce/consume is 16 samples per millisecond - i.e. this is a wideband device
        const size_t  MY_AUDIO_DEVICE_BUFFER_SAMPLE_COUNT = (MY_AUDIO_DEVICE_INTERVAL_MS * 8);

        int16_t buffer[MY_AUDIO_DEVICE_BUFFER_SAMPLE_COUNT];
        int     rc;
        FILE    *fp = nullptr;
        //int     x;

        // These are used to generate the sine wave for our "microphone"
        /*
        float amplitute = 1000;
        float pi = 22/7;
        float freq = 1024;
        float tm = 0.0;
        float phaseShift = 0.0;
        */

        #if defined(HAVE_RTS_INTERNAL_SOURCE_CODE)
        uint64_t    now;
        uint64_t    started = 0;
        uint64_t    loops = 0;
        #endif

        size_t      msToSleep;

        struct timeval  tv;
        int             ret;
        uint64_t        rxCount = 0;


        if(_direction == ConfigurationObjects::AudioDeviceDescriptor::Direction_t::dirInput)
        {
            if(g_adadMicFile != nullptr)
            {
                fp = fopen(g_adadMicFile, "rb");
                if(fp == nullptr)
                {
                    std::cout << "WARNING: specified adad microphone file '" << g_adadMicFile << "' cannot be opened" << std::endl;
                }
            }            
        }
        else if(_direction == ConfigurationObjects::AudioDeviceDescriptor::Direction_t::dirOutput)
        {
            if(g_adadSpkFile != nullptr)
            {
                fp = fopen(g_adadSpkFile, "wb");
                if(fp == nullptr)
                {
                    std::cout << "WARNING: specified adad speaker file '" << g_adadSpkFile << "' cannot be opened" << std::endl;
                }
            }            
        }

        memset(buffer, 0, sizeof(buffer));

        while( _running )
        {
            msToSleep = MY_AUDIO_DEVICE_INTERVAL_MS;

            now = getTickMs();

            if(started == 0)
            {
                started = now;
            }
            else
            {
                uint64_t avg = ((now - started) / loops);

                int64_t potentialTimeToSleep = 0;

                // Maybe catchup or slow down
                if(avg > MY_AUDIO_DEVICE_INTERVAL_MS)
                {
                    potentialTimeToSleep = (MY_AUDIO_DEVICE_INTERVAL_MS - ((avg - MY_AUDIO_DEVICE_INTERVAL_MS) * 32));
                }
                else if( avg < MY_AUDIO_DEVICE_INTERVAL_MS )
                {
                    potentialTimeToSleep = (MY_AUDIO_DEVICE_INTERVAL_MS + ((MY_AUDIO_DEVICE_INTERVAL_MS - avg) * 32));
                }
                else
                {
                    potentialTimeToSleep = MY_AUDIO_DEVICE_INTERVAL_MS;
                }

                if(potentialTimeToSleep < 0)
                {
                    potentialTimeToSleep = 0;
                }

                msToSleep = (size_t) potentialTimeToSleep;

                //std::cout << "avg=" << avg << ", msToSleep=" << msToSleep << std::endl;
            }

            loops++;

            if(!_paused)
            {
                if(_direction == ConfigurationObjects::AudioDeviceDescriptor::Direction_t::dirOutput)
                {
                    rc = engageAudioDeviceReadBuffer(_deviceId, _instanceId, buffer, MY_AUDIO_DEVICE_BUFFER_SAMPLE_COUNT);

                    if(rc > 0)
                    {
                        // At this point we have rc number of audio samples from the Engine.  These now need to be sent 
                        // onward to where they're needed.  For tis demo, we'll write the samples to a file or, if that's
                        // not available, we'll display the power level of the received buffer
                        if(fp != nullptr)
                        {
                            fwrite(buffer, 1, (2 * rc), fp);
                        }
                        else
                        {
                            float total = 0.0;

                            for(int x = 0; x < rc; x++)
                            {
                                total += buffer[x];
                            }

                            rxCount++;
                            if(rxCount % 100 == 0)
                            {
                                std::cout << "ADADInstance readReadBuffer received " << rxCount << " buffers so far" << std::endl;
                            }
                            //std::cout << "ADADInstance readReadBuffer received " << rc << " samples with an average sample level of " << (total / rc) << std::endl;
                        }
                    }
                }
                else if(_direction == ConfigurationObjects::AudioDeviceDescriptor::Direction_t::dirInput)
                {
                    // Read audio from the file, if we can't or don't have one, just generate noise
                    if(fp != nullptr)
                    {
                        size_t amountRead = fread(buffer, 1, MY_AUDIO_DEVICE_BUFFER_SAMPLE_COUNT * 2, fp);
                        if(amountRead != MY_AUDIO_DEVICE_BUFFER_SAMPLE_COUNT * 2)
                        {
                            fseek(fp, 0, SEEK_SET);
                            amountRead = fread(buffer, 1, MY_AUDIO_DEVICE_BUFFER_SAMPLE_COUNT * 2, fp);
                            if(amountRead != MY_AUDIO_DEVICE_BUFFER_SAMPLE_COUNT * 2)
                            {
                                std::cout << "******* ERROR *******: file read error from adad microphone file" << std::endl;
                                abort();
                            }
                        }
                    }
                    else
                    {
                        for(size_t x = 0; x < MY_AUDIO_DEVICE_BUFFER_SAMPLE_COUNT; x++)
                        {
                            buffer[x] = (rand() % 32767);
                            if(rand() % 100 < 50)
                            {
                                buffer[x] *= -1;
                            }
                        }
                    }

                    rc = engageAudioDeviceWriteBuffer(_deviceId, _instanceId, buffer, MY_AUDIO_DEVICE_BUFFER_SAMPLE_COUNT);
                }
                else
                {
                    assert(0);
                }
            }

            // Sleep for our device's "interval"
            tv.tv_sec = 0;
            tv.tv_usec = (msToSleep * 1000);
            do
            {
                ret = select(1, NULL, NULL, NULL, &tv);
            }
            while((ret == -1)&&(errno == EINTR));
            
            //std::this_thread::sleep_for(std::chrono::milliseconds(msToSleep));
        }

        if(fp != nullptr)
        {
            fclose(fp);
        }
    }

    int16_t                                                     _deviceId;
    int16_t                                                     _instanceId;
    ConfigurationObjects::AudioDeviceDescriptor::Direction_t    _direction;
    std::thread                                                 _threadHandle;
    bool                                                        _running;
    bool                                                        _paused;
};

std::map<int16_t, ADADInstance*>    g_audioDeviceInstances;

int MyAudioDeviceCallback(int16_t deviceId, int16_t instanceId, EngageAudioDeviceCtlOp_t op, uintptr_t p1)
{   
    int rc = ENGAGE_AUDIO_DEVICE_RESULT_OK;

    ADADInstance *instance = nullptr;
    std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> MyAudioDeviceCallback: deviceId=" << deviceId << ", ";

    // Instance creation is a little different from other operations
    if( op == EngageAudioDeviceCtlOp_t::eadCreateInstance)
    {
        g_nextAudioDeviceInstanceId++;

        if(deviceId == g_speakerDeviceId)
        {
            // Create an instance of a speaker
            instance = new ADADInstance(deviceId, g_nextAudioDeviceInstanceId, ConfigurationObjects::AudioDeviceDescriptor::Direction_t::dirOutput);
        }
        else if(deviceId == g_microphoneDeviceId)
        {
            // Create an instance of a speaker
            instance = new ADADInstance(deviceId, g_nextAudioDeviceInstanceId, ConfigurationObjects::AudioDeviceDescriptor::Direction_t::dirInput);
        }
        else
        {
            assert(0);
        }     

        g_audioDeviceInstances[g_nextAudioDeviceInstanceId] = instance;

        rc = g_nextAudioDeviceInstanceId;
    }
    else
    {
        // Track down the instance object
        std::map<int16_t, ADADInstance*>::iterator itr = g_audioDeviceInstances.find(instanceId);
        if(itr != g_audioDeviceInstances.end())
        {
            instance = itr->second;

            // The Engine wants us to ...
            switch( op )
            {
                // We should never fall into this case because the "if" above catered for it.  But, some compilers
                // will warn about the switch not catering for all enum values from EngageAudioDeviceCtlOp_t.  So we'll
                // put in this case to keep them happy.
                case EngageAudioDeviceCtlOp_t::eadCreateInstance:
                    assert(0);
                    break;

                // ... destroy an instance of a speaker identified by "instanceId"
                case EngageAudioDeviceCtlOp_t::eadDestroyInstance:
                    std::cout << "destroy instance instanceId=" << instanceId << std::endl;
                    instance->stop();
                    delete instance;
                    g_audioDeviceInstances.erase(itr);
                    break;

                // ... start an instance of a device identified by "instanceId"
                case EngageAudioDeviceCtlOp_t::eadStart:
                    std::cout << "start" << std::endl;
                    instance->start();
                    break;

                // ... stop an instance of a device identified by "instanceId"
                case EngageAudioDeviceCtlOp_t::eadStop:
                    std::cout << "stop" << std::endl;
                    instance->stop();
                    break;

                // ... pause an instance of a device identified by "instanceId"
                case EngageAudioDeviceCtlOp_t::eadPause:
                    std::cout << "pause" << std::endl;
                    instance->pause();
                    break;

                // ... resume an instance of a device identified by "instanceId"
                case EngageAudioDeviceCtlOp_t::eadResume:
                    std::cout << "resume" << std::endl;
                    instance->resume();
                    break;

                // ... reset an instance of a device identified by "instanceId"
                case EngageAudioDeviceCtlOp_t::eadReset:
                    std::cout << "reset" << std::endl;
                    instance->reset();
                    break;

                // ... restart an instance of a device identified by "instanceId"
                case EngageAudioDeviceCtlOp_t::eadRestart:
                    std::cout << "restart" << std::endl;
                    instance->restart();
                    break;

                // The compiler should catch this.  But, just in case ...
                default:
                    assert(false);
                    rc = ENGAGE_AUDIO_DEVICE_INVALID_OPERATION;
                    break;
            }            
        }
        else
        {
            std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> MyAudioDeviceCallback for an unknown instance id of " << instanceId << std::endl;
            rc = ENGAGE_AUDIO_DEVICE_INVALID_INSTANCE_ID;
        }        
    }

    return rc;
}

void registerADAD()
{
    // Setup the speaker
    {
        ConfigurationObjects::AudioDeviceDescriptor speakerDevice;
        speakerDevice.direction = ConfigurationObjects::AudioDeviceDescriptor::Direction_t::dirOutput;
        speakerDevice.deviceId = 0;
        speakerDevice.samplingRate = 8000;
        speakerDevice.channels = 2;
        speakerDevice.boostPercentage = 0;
        std::string json = speakerDevice.serialize();
        g_speakerDeviceId = engageAudioDeviceRegister(json.c_str(), MyAudioDeviceCallback);
        if(g_speakerDeviceId < 0)
        {
            g_speakerDeviceId = 0;
        }
    }

    // Setup the microphone
    {
        ConfigurationObjects::AudioDeviceDescriptor microphoneDevice;
        microphoneDevice.direction = ConfigurationObjects::AudioDeviceDescriptor::Direction_t::dirInput;
        microphoneDevice.deviceId = 0;
        microphoneDevice.samplingRate = 8000;
        microphoneDevice.channels = 1;
        microphoneDevice.boostPercentage = 0;
        std::string json = microphoneDevice.serialize();
        g_microphoneDeviceId = engageAudioDeviceRegister(json.c_str(), MyAudioDeviceCallback);
        if(g_microphoneDeviceId < 0)
        {
            g_microphoneDeviceId = 0;
        }
    }
}

void unregisterADAD()
{
    if(g_speakerDeviceId > 0)
    {
        engageAudioDeviceUnregister(g_speakerDeviceId);
        g_speakerDeviceId = 0;
    }
    
    if(g_microphoneDeviceId > 0)
    {
        engageAudioDeviceUnregister(g_microphoneDeviceId);
        g_microphoneDeviceId = 0;
    }
}

void cleanupADAInstances()
{
    for(std::map<int16_t, ADADInstance*>::iterator itr = g_audioDeviceInstances.begin();
        itr != g_audioDeviceInstances.end();
        itr++)
    {
        itr->second->stop();
        delete itr->second;
    }

    g_audioDeviceInstances.clear();
}


void showUsage()
{
    std::cout << "usage: engage-cmd -mission:<mission_file> [options]" << std::endl << std::endl
              << "\twhere [options] are:" << std::endl << std::endl
              << "\t-cs:<certificate_store_file> .......... specify a certificate store file" << std::endl
              << "\t-csp:<certificate_store_pwd> .......... password (if any) for the certificate store in hexstring format" << std::endl
              << "\t-ep:<engine_policy_file> .............. specify an engine policy" << std::endl
              << "\t-rp:<rallypoint_file> ................. specify a rallypoint configuration" << std::endl
              << "\t-nic:<nic_name> ....................... specify the name of the default nic" << std::endl
              << "\t-ua:<user_alias> ...................... set the user's alias" << std::endl
              << "\t-ui:<user_id> ......................... set the user's id" << std::endl
              << "\t-ud:<user_display> .................... set the user's display name" << std::endl
              << "\t-ut:<user_tx_priority> ................ set audio transmit priority" << std::endl
              << "\t-uf:<user_tx_flags> ................... set audio transmit flags" << std::endl
              << "\t-putenv:<key>=<value> ................. set an environment variable" << std::endl
              << "\t-verbose .............................. enable verbose script mode" << std::endl
              << "\t-anon ................................. operate in anonymous identity mode" << std::endl
              << "\t-useadad .............................. use an application-defined audio device" << std::endl
              << "\t-adadmic:<file_name>................... name of microphone file to use for the ADAD" << std::endl
              << "\t-adadspk:<file_name>................... name of speaker file to use for the ADAD" << std::endl              
              << "\t-jsonobjects .......................... display json object configuration" << std::endl
              << "\t-sn:<speaker_name> .................... set name of audio device to use for speaker" << std::endl
              << "\t-mn:<microphone_name> ................. set name of audio device to use for microphone" << std::endl
              << "\t-dbusconn:<dbus_connection_name> ...... set the DBUS connection name" << std::endl
              << "\t-dbussig:<dbus_signal_name> ........... set the DBUS signal name" << std::endl;
}

void showHelp()
{
    std::cout << "q.............quit" << std::endl;
    std::cout << "!<command>....execute a shell command" << std::endl;
    std::cout << "-.............draw a line" << std::endl;
    std::cout << "starteng......start engine" << std::endl;
    std::cout << "stopeng.......stop engine" << std::endl;
    std::cout << "sg............show group list" << std::endl;
    std::cout << "sn............show node list" << std::endl;
    std::cout << "gm............generate mission json" << std::endl;
    std::cout << "y.............request sync" << std::endl;
    std::cout << "p<N>..........set tx priority to N" << std::endl;
    std::cout << "f<N>..........set tx flags to N" << std::endl;
    std::cout << "c<N>..........create group index N" << std::endl;
    std::cout << "d<N>..........delete group index N" << std::endl;
    std::cout << "ca............create all groups" << std::endl;
    std::cout << "da............delete all groups" << std::endl;
    std::cout << "j<N>..........join group index N" << std::endl;
    std::cout << "ja............join all groups" << std::endl;
    std::cout << "l<N>..........leave group index N" << std::endl;
    std::cout << "la............leave all groups" << std::endl;
    std::cout << "b<N>..........begin tx on group index N" << std::endl;
    std::cout << "ba............begin tx on all groups" << std::endl;
    std::cout << "e<N>..........end tx on group index N" << std::endl;
    std::cout << "ea............end tx on all groups" << std::endl;
    std::cout << "m<N>..........mute rx on group index N" << std::endl;
    std::cout << "ma............mute rx on all groups" << std::endl;
    std::cout << "u<N>..........unmute rx on group index N" << std::endl;
    std::cout << "ua............unmute rx on all groups" << std::endl;
    std::cout << "z<N>..........send a presence update on presence group index N" << std::endl;
    std::cout << "za............send a presence update on all presence groups" << std::endl;
    std::cout << "z<N>..........send a presence update on presence group index N" << std::endl;
    std::cout << "ta <V>........set rx tag on all groups groups to V" << std::endl;
    std::cout << "t<N>> <V>.....set rx tag on group index N to V" << std::endl;
    std::cout << "la............send a blob on all groups" << std::endl;
    std::cout << "l<N>..........send a blob on group index N" << std::endl;
    std::cout << "ra............send a rtp payload on all groups" << std::endl;
    std::cout << "r<N>..........send a rtp payload on group index N" << std::endl;
    std::cout << "wa............send a raw payload on all groups" << std::endl;
    std::cout << "w<N>..........send a raw payload on group index N" << std::endl;
    std::cout << "xa <V>........register rtp payload handler <V> on all groups" << std::endl;
    std::cout << "x<N>..........register rtp payload handler <V> on group index N" << std::endl;
    std::cout << "ya <V>........unregister rtp payload handler <V> on all groups" << std::endl;
    std::cout << "y<N>..........unregister rtp payload handler <V> on group index N" << std::endl;
}

void showGroups()
{
    std::cout << "Groups:" << std::endl;
    for(size_t x = 0; x < g_new_groups.size(); x++)
    {
        std::cout << "index=" << x
                  << ", type=" << g_new_groups[x].type
                  << ", id=" << g_new_groups[x].id
                  << ", name=" << g_new_groups[x].name
                  << ", encrypted=" << (g_new_groups[x].isEncrypted ? "yes" : "no")
                  << ", fullDuplex=" << (g_new_groups[x].allowsFullDuplex ? "yes" : "no")
                  << std::endl;
    }
}

void showGroupAliases(ConfigurationObjects::PresenceDescriptor& pd)
{
    for(std::vector<ConfigurationObjects::GroupAlias>::iterator itr = pd.groupAliases.begin();
        itr != pd.groupAliases.end();
        itr++)
    {
        std::cout << "     " << itr->groupId << " = " << itr->alias << std::endl;
    }
}

void showNodes()
{
    std::cout << "\n\nNodes:" << std::endl;

    std::map<std::string, ConfigurationObjects::PresenceDescriptor>::iterator itr;
    for(itr = g_nodes.begin();
        itr != g_nodes.end();
        itr++)
    {
        ConfigurationObjects::PresenceDescriptor *pd = &(itr->second);

        std::cout << (pd->self ? "(*SELF*) " : "")
                  << pd->identity.nodeId 
                  << ", " << pd->identity.userId 
                  << ", " << pd->identity.displayName                  
                  << std::endl;

        showGroupAliases(*pd);

        std::cout << std::endl;
    }

    std::cout << "\n\n" << std::endl;    
}

bool loadPolicy(const char *pszFn, ConfigurationObjects::EnginePolicy *pPolicy)
{
    bool pLd = false;

    try
    {
        std::string jsonText;
        jsonText = Utils::readTextFile(pszFn);

        nlohmann::json j = nlohmann::json::parse(jsonText);
        ConfigurationObjects::from_json(j, *pPolicy);
        pLd = true;
    }
    catch (...)
    {
    }                
    
    return pLd;
}

bool loadMission(const char *pszFn, ConfigurationObjects::Mission *pMission, ConfigurationObjects::Rallypoint *pRp)
{
    bool pLd = false;

    try
    {
        std::string jsonText;
        jsonText = Utils::readTextFile(pszFn);

        nlohmann::json j = nlohmann::json::parse(jsonText);
        ConfigurationObjects::from_json(j, *pMission);
        pLd = true;

        // We may have a global rallypoint defined in this JSON
        try
        {
            nlohmann::json rp = j.at("rallypoint");
            std::string address = rp.at("address");
            int port = rp.at("port");

            pRp->host.address = address;
            pRp->host.port = port;
            pRp->certificate = "@default_cert.pem";
            pRp->certificateKey = "@default_key.pem";
        }
        catch(...)
        {
            pRp->clear();
        }
    }
    catch (...)
    {
    }                

    return pLd;
}

bool loadRp(const char *pszFn, ConfigurationObjects::Rallypoint *pRp)
{
    bool pLd = false;

    try
    {
        std::string jsonText;
        jsonText = Utils::readTextFile(pszFn);

        nlohmann::json j = nlohmann::json::parse(jsonText);
        ConfigurationObjects::from_json(j, *pRp);
        pLd = true;
    }
    catch (...)
    {
    }                
    return pLd;
}

void on_ENGAGE_ENGINE_STARTED(const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_ENGINE_STARTED" << std::endl;
    g_engineStarted = true;
    g_engineStopped = false;
}

void on_ENGAGE_ENGINE_STOPPED(const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_ENGINE_STOPPED" << std::endl;
    g_engineStarted = false;
    g_engineStopped = true;
}

void on_ENGAGE_RP_PAUSING_CONNECTION_ATTEMPT(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_RP_PAUSING_CONNECTION_ATTEMPT: " << pId << std::endl;
}

void on_ENGAGE_RP_CONNECTING(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_RP_CONNECTING: " << pId << std::endl;
}

void on_ENGAGE_RP_CONNECTED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_RP_CONNECTED: " << pId << std::endl;
}

void on_ENGAGE_RP_DISCONNECTED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_RP_DISCONNECTED: " << pId << std::endl;
}

void on_ENGAGE_RP_ROUNDTRIP_REPORT(const char *pId, uint32_t rtMs, uint32_t rtRating, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_RP_ROUNDTRIP_REPORT: " << pId << ", ms=" << rtMs << ", rating=" << rtRating << std::endl;
}

void on_ENGAGE_GROUP_CREATED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_CREATED: " << pId << std::endl;
    int index = indexOfGroupId(pId);
    if(index >= 0)
    {
        g_new_groups[index].created = true;
        g_new_groups[index].createFailed = false;
        g_new_groups[index].deleted = false;
    }
}

void on_ENGAGE_GROUP_CREATE_FAILED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_CREATE_FAILED: " << pId << std::endl;
    int index = indexOfGroupId(pId);
    if(index >= 0)
    {
        g_new_groups[index].created = false;
        g_new_groups[index].createFailed = true;
    }
}

void on_ENGAGE_GROUP_DELETED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_DELETED: " << pId << std::endl;
    int index = indexOfGroupId(pId);
    if(index >= 0)
    {
        g_new_groups[index].deleted = true;
    }
}

void on_ENGAGE_GROUP_INSTANTIATED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_INSTANTIATED: " << pId << std::endl;
}

void on_ENGAGE_GROUP_INSTANTIATE_FAILED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_INSTANTIATE_FAILED: " << pId << std::endl;
}

void on_ENGAGE_GROUP_DEINSTANTIATED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_DEINSTANTIATED: " << pId << std::endl;
}

void on_ENGAGE_GROUP_CONNECTED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_CONNECTED: " << pId << ", " << eventExtraJson << std::endl;
    int index = indexOfGroupId(pId);
    if(index >= 0)
    {
        g_new_groups[index].connected = true;
        g_new_groups[index].connectFailed = false;
        g_new_groups[index].notConnected = false;
    }
}

void on_ENGAGE_GROUP_CONNECT_FAILED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_CONNECT_FAILED: " << pId << ", " << eventExtraJson << std::endl;
    int index = indexOfGroupId(pId);
    if(index >= 0)
    {
        g_new_groups[index].connected = false;
        g_new_groups[index].connectFailed = true;
        g_new_groups[index].notConnected = true;
    }
}

void on_ENGAGE_GROUP_DISCONNECTED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_DISCONNECTED: " << pId << ", " << eventExtraJson << std::endl;
    int index = indexOfGroupId(pId);
    if(index >= 0)
    {
        g_new_groups[index].connected = false;
        g_new_groups[index].connectFailed = false;
        g_new_groups[index].notConnected = true;
    }
}

void on_ENGAGE_GROUP_JOINED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_JOINED: " << pId << std::endl;
    int index = indexOfGroupId(pId);
    if(index >= 0)
    {
        g_new_groups[index].joined = true;
        g_new_groups[index].joinFailed = false;
        g_new_groups[index].notJoined = false;
    }
}

void on_ENGAGE_GROUP_JOIN_FAILED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_JOIN_FAILED: " << pId << std::endl;
    int index = indexOfGroupId(pId);
    if(index >= 0)
    {
        g_new_groups[index].joined = false;
        g_new_groups[index].joinFailed = true;
        g_new_groups[index].notJoined = true;
    }
}

void on_ENGAGE_GROUP_LEFT(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_LEFT: " << pId << std::endl;
    int index = indexOfGroupId(pId);
    if(index >= 0)
    {
        g_new_groups[index].joined = false;
        g_new_groups[index].joinFailed = false;
        g_new_groups[index].notJoined = true;
    }
}

void on_ENGAGE_GROUP_MEMBER_COUNT_CHANGED(const char *pId, size_t newCount, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_MEMBER_COUNT_CHANGED: " 
              << pId 
              << ", " 
              << newCount << std::endl;
}

void on_ENGAGE_GROUP_RX_STARTED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_RX_STARTED: " << pId << std::endl;
    int index = indexOfGroupId(pId);
    if(index >= 0)
    {
        g_new_groups[index].rxStarted = true;
        g_new_groups[index].rxEnded = false;
    }
}

void on_ENGAGE_GROUP_RX_ENDED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_RX_ENDED: " << pId << std::endl;
    int index = indexOfGroupId(pId);
    if(index >= 0)
    {
        g_new_groups[index].rxStarted = false;
        g_new_groups[index].rxEnded = true;
    }
}

void on_ENGAGE_GROUP_RX_MUTED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_RX_MUTED: " << pId << std::endl;
}

void on_ENGAGE_GROUP_RX_UNMUTED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_RX_UNMUTED: " << pId << std::endl;
}

void on_ENGAGE_GROUP_RX_SPEAKERS_CHANGED(const char *pId, const char *groupTalkerJson, const char *eventExtraJson)
{
    std::string listOfNames;
    ConfigurationObjects::GroupTalkers gt;

    if(!gt.deserialize(groupTalkerJson))
    {
        listOfNames = "(none)";
    }
    else
    {
        for(std::vector<ConfigurationObjects::TalkerInformation>::iterator itr = gt.list.begin();
            itr != gt.list.end();
            itr++)
        {
            if(!listOfNames.empty())
            {
                listOfNames += ", ";
            }

            std::string info;

            info = itr->alias;
            info.append(" nodeId='");
            info.append(itr->nodeId);
            info.append("'");
            listOfNames += info;
        }
    }

    std::cout << "D/EngageMain: on_ENGAGE_GROUP_RX_SPEAKERS_CHANGED: " << pId << " [" << listOfNames << "]" << std::endl;
}

void on_ENGAGE_GROUP_TX_STARTED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_TX_STARTED: " << pId << std::endl;
    int index = indexOfGroupId(pId);
    if(index >= 0)
    {
        g_new_groups[index].txStarted = true;
        g_new_groups[index].txEnded = false;
        g_new_groups[index].txFailed = false;
    }
}

void on_ENGAGE_GROUP_TX_ENDED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_TX_ENDED: " << pId << std::endl;
    int index = indexOfGroupId(pId);
    if(index >= 0)
    {
        g_new_groups[index].txStarted = false;
        g_new_groups[index].txEnded = true;
        g_new_groups[index].txFailed = false;
    }
}

void on_ENGAGE_GROUP_TX_FAILED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_TX_FAILED: " << pId << std::endl;
    int index = indexOfGroupId(pId);
    if(index >= 0)
    {
        g_new_groups[index].txStarted = false;
        g_new_groups[index].txEnded = false;
        g_new_groups[index].txFailed = true;
    }
}

void on_ENGAGE_GROUP_TX_USURPED_BY_PRIORITY(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_TX_USURPED_BY_PRIORITY: " << pId << std::endl;
    int index = indexOfGroupId(pId);
    if(index >= 0)
    {
        g_new_groups[index].txStarted = false;
        g_new_groups[index].txEnded = true;
        g_new_groups[index].txFailed = false;
    }
}

void on_ENGAGE_GROUP_MAX_TX_TIME_EXCEEDED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_MAX_TX_TIME_EXCEEDED: " << pId << std::endl;
}

void on_ENGAGE_GROUP_TX_MUTED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_TX_MUTED: " << pId << std::endl;
}

void on_ENGAGE_GROUP_TX_UNMUTED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_TX_UNMUTED: " << pId << std::endl;
}

void addOrUpdatePd(ConfigurationObjects::PresenceDescriptor& pd)
{
    g_nodes.erase(pd.identity.nodeId);
    g_nodes[pd.identity.nodeId] = pd;
}

void removePd(ConfigurationObjects::PresenceDescriptor& pd)
{
    g_nodes.erase(pd.identity.nodeId);
}

void on_ENGAGE_GROUP_NODE_DISCOVERED(const char *pId, const char *pszNodeJson, const char *eventExtraJson)
{
    ConfigurationObjects::PresenceDescriptor pd;
    if(pd.deserialize(pszNodeJson))
    {
        std::cout << "D/EngageMain: on_ENGAGE_GROUP_NODE_DISCOVERED: " << pId << ", " << pd.identity.nodeId 
                  << " : " << pszNodeJson
                  << std::endl;
        addOrUpdatePd(pd);
    }
    else
    {
       std::cout << "D/EngageMain: on_ENGAGE_GROUP_NODE_DISCOVERED: " << pId << ", COULD NOT PARSE JSON" << std::endl; 
    }    
}

void on_ENGAGE_GROUP_NODE_REDISCOVERED(const char *pId, const char *pszNodeJson, const char *eventExtraJson)
{
    ConfigurationObjects::PresenceDescriptor pd;
    if(pd.deserialize(pszNodeJson))
    {
        std::cout << "D/EngageMain: on_ENGAGE_GROUP_NODE_REDISCOVERED: " << pId << ", " << pd.identity.nodeId 
                  << " : " << pszNodeJson
                  << std::endl;
        addOrUpdatePd(pd);
    }
    else
    {
       std::cout << "D/EngageMain: on_ENGAGE_GROUP_NODE_REDISCOVERED: " << pId << ", COULD NOT PARSE JSON" << std::endl; 
    }    
}

void on_ENGAGE_GROUP_NODE_UNDISCOVERED(const char *pId, const char *pszNodeJson, const char *eventExtraJson)
{
    ConfigurationObjects::PresenceDescriptor pd;
    if(pd.deserialize(pszNodeJson))
    {
        std::cout << "D/EngageMain: on_ENGAGE_GROUP_NODE_UNDISCOVERED: " << pId << ", " << pd.identity.nodeId << std::endl;
        removePd(pd);
    }
    else
    {
       std::cout << "D/EngageMain: on_ENGAGE_GROUP_NODE_UNDISCOVERED: " << pId << ", COULD NOT PARSE JSON" << std::endl; 
    }    
}

void on_ENGAGE_GROUP_ASSET_DISCOVERED(const char *pId, const char *pszNodeJson, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_ASSET_DISCOVERED: " << pId << ", " << pszNodeJson << std::endl;

    engageJoinGroup(pId);
}

void on_ENGAGE_GROUP_ASSET_REDISCOVERED(const char *pId, const char *pszNodeJson, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_ASSET_REDISCOVERED: " << pId << ", " << pszNodeJson << std::endl;
}

void on_ENGAGE_GROUP_ASSET_UNDISCOVERED(const char *pId, const char *pszNodeJson, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_ASSET_UNDISCOVERED: " << pId << ", " << pszNodeJson << std::endl;
}

void on_ENGAGE_LICENSE_CHANGED(const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_LICENSE_CHANGED" << std::endl;
    const char *p = engageGetActiveLicenseDescriptor();
    std::cout << p << std::endl;
}

void on_ENGAGE_LICENSE_EXPIRED(const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_LICENSE_EXPIRED" << std::endl;
}

void on_ENGAGE_LICENSE_EXPIRING(const char *pSecsLeft, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_LICENSE_EXPIRING in " << pSecsLeft << " seconds" << std::endl;
}

void on_ENGAGE_GROUP_BLOB_SENT(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_BLOB_SENT: " << pId << std::endl;
}

void on_ENGAGE_GROUP_BLOB_SEND_FAILED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_BLOB_SEND_FAILED: " << pId << std::endl;
}

void on_ENGAGE_GROUP_BLOB_RECEIVED(const char *pId, const char *pszBlobJson, const uint8_t *blob, size_t blobSize, const char *eventExtraJson)
{
    ConfigurationObjects::BlobInfo bi;

    if(!bi.deserialize(pszBlobJson))
    {
        std::cout << "D/EngageMain: on_ENGAGE_GROUP_BLOB_RECEIVED ERROR: Cannot parse blob info JSON" << std::endl;
        return;
    }

    // We'll make a copy to work with
    uint8_t *blobCopy = new uint8_t[blobSize];
    memcpy(blobCopy, blob, blobSize);

    if(bi.payloadType == ConfigurationObjects::BlobInfo::bptUndefined)
    {
        std::cout << "D/EngageMain: on_ENGAGE_GROUP_BLOB_RECEIVED [UNDEFINED]: " << pId << ", blobSize=" << blobSize << ", blobJson=" << pszBlobJson << std::endl;
    }
    else if(bi.payloadType == ConfigurationObjects::BlobInfo::bptAppTextUtf8)
    {
        std::cout << "D/EngageMain: on_ENGAGE_GROUP_BLOB_RECEIVED [APP TEXT UTF8]: " << pId << ", blobSize=" << blobSize << ", blobJson=" << pszBlobJson << std::endl;
    }
    else if(bi.payloadType == ConfigurationObjects::BlobInfo::bptJsonTextUtf8)
    {
        std::cout << "D/EngageMain: on_ENGAGE_GROUP_BLOB_RECEIVED [JSON TEXT UTF8]: " << pId << ", blobSize=" << blobSize << ", blobJson=" << pszBlobJson << std::endl;
    }
    else if(bi.payloadType == ConfigurationObjects::BlobInfo::bptAppBinary)
    {
        std::cout << "D/EngageMain: on_ENGAGE_GROUP_BLOB_RECEIVED [APP BINARY]: " << pId << ", blobSize=" << blobSize << ", blobJson=" << pszBlobJson << std::endl;
    }
    else if(bi.payloadType == ConfigurationObjects::BlobInfo::bptEngageBinaryHumanBiometrics)
    {
        std::cout << "D/EngageMain: on_ENGAGE_GROUP_BLOB_RECEIVED [ENGAGE HUMAN BIOMETRICS : HEART RATE]: " << pId << ", blobSize=" << blobSize << ", blobJson=" << pszBlobJson << std::endl;

        uint8_t *src = blobCopy;
        size_t bytesLeft = blobSize;

        while( bytesLeft > 0 )
        {
            ConfigurationObjects::DataSeriesHeader_t *hdr = (ConfigurationObjects::DataSeriesHeader_t*)src;        
            hdr->ts = ntohl(hdr->ts);

            src += sizeof(ConfigurationObjects::DataSeriesHeader_t);
            bytesLeft -= sizeof(ConfigurationObjects::DataSeriesHeader_t);

            if(hdr->t == ConfigurationObjects::HumanBiometricsTypes_t::heartRate)
            {
                std::cout << "\tHEART RATE: " << (int)hdr->ss << " samples" << std::endl;
            }
            else if(hdr->t == ConfigurationObjects::HumanBiometricsTypes_t::skinTemp)
            {
                std::cout << "\tSKIN TEMP: " << (int)hdr->ss << " samples" << std::endl;
            }
            else if(hdr->t == ConfigurationObjects::HumanBiometricsTypes_t::coreTemp)
            {
                std::cout << "\tCORE TEMP: " << (int)hdr->ss << " samples" << std::endl;
            }
            else if(hdr->t == ConfigurationObjects::HumanBiometricsTypes_t::hydration)
            {
                std::cout << "\tHYDRATION: " << (int)hdr->ss << " samples" << std::endl;
            }
            else if(hdr->t == ConfigurationObjects::HumanBiometricsTypes_t::bloodOxygenation)
            {
                std::cout << "\tBLOOD OXYGENATION: " << (int)hdr->ss << " samples" << std::endl;
            }
            else if(hdr->t == ConfigurationObjects::HumanBiometricsTypes_t::fatigueLevel)
            {
                std::cout << "\tFATIGUE LEVEL: " << (int)hdr->ss << " samples" << std::endl;
            }
            else if(hdr->t == ConfigurationObjects::HumanBiometricsTypes_t::taskEffectiveness)
            {
                std::cout << "\tTASK EFFECTIVENESS: " << (int)hdr->ss << " samples" << std::endl;
            }
            else
            {
                std::cout << "\tUNKNOWN BIOMETRIC: " << (int)hdr->ss << " samples" << std::endl;
            }  

            std::cout << "\t\t";

            if(hdr->vt == ConfigurationObjects::DataSeriesValueType_t::uint8)
            {
                ConfigurationObjects::DataElementUint8_t *element = (ConfigurationObjects::DataElementUint8_t*) src;

                for(uint8_t x = 0; x < hdr->ss; x++)
                {
                    std::cout << (int) element->ofs << ", " << (int) element->val << " | ";
                    element++;                    
                }

                src += (sizeof(ConfigurationObjects::DataElementUint8_t) * hdr->ss);
                bytesLeft -= (sizeof(ConfigurationObjects::DataElementUint8_t) * hdr->ss);
            }
            else if(hdr->vt == ConfigurationObjects::DataSeriesValueType_t::uint16)
            {
                // TODO : display 16-bit numbers
            }
            else if(hdr->vt == ConfigurationObjects::DataSeriesValueType_t::uint32)
            {
                // TODO : display 32-bit numbers
            }
            else if(hdr->vt == ConfigurationObjects::DataSeriesValueType_t::uint64)
            {
                // TODO : display 64-bit numbers
            }

            std::cout << std::endl;                      
        }        
    }

    delete[] blobCopy;
}

void on_ENGAGE_GROUP_RTP_SENT(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_RTP_SENT: " << pId << std::endl;
}

void on_ENGAGE_GROUP_RTP_SEND_FAILED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_RTP_SEND_FAILED: " << pId << std::endl;
}

void on_ENGAGE_GROUP_RTP_RECEIVED(const char *pId, const char *pszRtpHeaderJson, const uint8_t *payload, size_t payloadSize, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_RTP_RECEIVED: " << pId << ", payloadSize=" << payloadSize << ", rtpHeaderJson=" << pszRtpHeaderJson << std::endl;
}

void on_ENGAGE_GROUP_RAW_SENT(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_RAW_SENT: " << pId << std::endl;
}

void on_ENGAGE_GROUP_RAW_SEND_FAILED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_RAW_SEND_FAILED: " << pId << std::endl;
}

void on_ENGAGE_GROUP_RAW_RECEIVED(const char *pId, const uint8_t *raw, size_t rawSize, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_RAW_RECEIVED: " << pId << std::endl;
}


void on_ENGAGE_GROUP_TIMELINE_EVENT_STARTED(const char *pId, const char *eventJson, const char *eventExtraJson)
{
    /*
    nlohmann::json j = nlohmann::json::parse(eventJson);
    std::string alias = j.at("alias");
    std::string nodeId = j.at("nodeId");
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_TIMELINE_EVENT_STARTED: alias='" << alias << "', nodeIde='" << nodeId << "'" << std::endl;
    */

    std::cout << "D/EngageMain: on_ENGAGE_GROUP_TIMELINE_EVENT_STARTED: " << pId << ":" << eventJson << std::endl;
}

void on_ENGAGE_GROUP_TIMELINE_EVENT_UPDATED(const char *pId, const char *eventJson, const char *eventExtraJson)
{
    /*
    nlohmann::json j = nlohmann::json::parse(eventJson);
    std::string alias = j.at("alias");
    std::string nodeId = j.at("nodeId");
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_TIMELINE_EVENT_UPDATED: alias='" << alias << "', nodeIde='" << nodeId << "'" << std::endl;
    */

    std::cout << "D/EngageMain: on_ENGAGE_GROUP_TIMELINE_EVENT_UPDATED: " << pId << ":" << eventJson << std::endl;
}

void on_ENGAGE_GROUP_TIMELINE_EVENT_ENDED(const char *pId, const char *eventJson, const char *eventExtraJson)
{
    /*
    nlohmann::json j = nlohmann::json::parse(eventJson);
    std::string alias = j.at("alias");
    std::string nodeId = j.at("nodeId");
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_TIMELINE_EVENT_ENDED: alias='" << alias << "', nodeIde='" << nodeId << "'" << std::endl;
    */

    std::cout << "D/EngageMain: on_ENGAGE_GROUP_TIMELINE_EVENT_ENDED: " << pId << ":" << eventJson << std::endl;
}

void on_ENGAGE_GROUP_TIMELINE_REPORT(const char *pId, const char *reportJson, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_TIMELINE_REPORT: " << pId << ":" << std::endl << reportJson << std::endl;
}

void on_ENGAGE_GROUP_TIMELINE_REPORT_FAILED(const char *pId, const char *eventExtraJson)
{
    std::cout << "D/EngageMain: on_ENGAGE_GROUP_TIMELINE_REPORT_FAILED: " << pId << std::endl;
}

std::string expandString(const char *s);

class Instruction
{
public:
    typedef enum {iUnknown, 
                  iGoto, 
                  iLabel, 
                  iFatalMessage,
                  iErrorMessage,
                  iWarningMessage,
                  iInfoMessage,
                  iDebugMessage,
                  iSleep,
                  iCreate, 
                  iDelete,
                  iJoin,
                  iLeave,
                  iBeginTx,
                  iEndTx,
                  iMuteRx,
                  iUnmuteRx,
                  iMuteTx,
                  iUnmuteTx,
                  iEndScript,
                  iSet,
                  iAdd,
                  iSub,
                  iCompare,
                  iOnGoto,
                  iCls,
                  iStartEngine,
                  iStopEngine,
                  iIfCondition,
                  iWaitForGroup,
                  iWaitForEngine,
                  } Type_t;

    Type_t type;
    
    int intParam;
    int intParam2;

    bool randomizeInt;
    bool randomizeInt2;

    std::string stringParam;
    std::string stringParam2;

    bool intParamFromPlaceholder;
    std::string intPlaceholder;

    bool intParamFromPlaceholder2;
    std::string intPlaceholder2;

    Instruction()
    {
        clear();
    }

    void clear()
    {
        type = iUnknown;
        intParam = 0;
        intParam2 = 0;
        randomizeInt = false;
        randomizeInt2 = false;
        intParamFromPlaceholder = false;
        intParamFromPlaceholder2 = false;
        stringParam.clear();
        stringParam2.clear();
        intPlaceholder.clear();
        intPlaceholder2.clear();
    }

    int getExpandedIntParam(int rndRange)
    {
        if(randomizeInt)
        {
            if( rndRange > 0)
            {
                return (rand() % rndRange);
            }
            else
            {
                return rand();
            }
        }
        else if(intParamFromPlaceholder)
        {
            std::string tmp = expandString(intPlaceholder.c_str());                    
            return atoi(tmp.c_str());
        }
        else
        {
            return intParam;
        }
    }

    int getExpandedIntParam2(int rndRange)
    {
        if(randomizeInt2)
        {
            if( rndRange > 0)
            {
                return (rand() % rndRange);
            }
            else
            {
                return rand();
            }
        }
        else if(intParamFromPlaceholder2)
        {
            std::string tmp = expandString(intPlaceholder2.c_str());                    
            return atoi(tmp.c_str());
        }
        else
        {
            return intParam2;
        }
    }
};

std::list<Instruction> g_scriptInstructions;
std::map<std::string, int>  g_intVars;

void processIntParameter(Instruction *pIns, char *tok)
{
    printf("[%s]\n", tok);
    if(strcmp(tok, "(allgroups)") == 0)
    {
        pIns->intParam = -1;
    }
    else if(tok[0] == 'r')
    {
        tok++;
        pIns->intParam = atoi(tok);
        pIns->randomizeInt = true;
    }
    else if(tok[0] == '$')
    {
        pIns->intPlaceholder.assign(tok);
        pIns->intParamFromPlaceholder = true;
    }
    else
    {
        pIns->intParam = atoi(tok);
    }
}

void processIntParameter2(Instruction *pIns, char *tok)
{
    printf("[%s]\n", tok);
    if(strcmp(tok, "(allgroups)") == 0)
    {
        pIns->intParam2 = -1;
    }
    else if(tok[0] == 'r')
    {
        tok++;
        pIns->intParam2 = atoi(tok);
        pIns->randomizeInt = true;
    }
    else if(tok[0] == '$')
    {
        pIns->intPlaceholder2.assign(tok);
        pIns->intParamFromPlaceholder2 = true;
    }
    else
    {
        pIns->intParam2 = atoi(tok);
    }
}

bool interpret(char *line, Instruction *pIns)
{
    pIns->clear();

    // Labels are special
    if(line[0] == ':')
    {
        pIns->type = Instruction::iLabel;
        pIns->stringParam = line+1;
        return true;
    }

    static const char *SEPS = " ";
	char *tokenCtx = nullptr;
    char *tok = strtok_s(line, SEPS, &tokenCtx);

    if( strcmp(tok, "message") == 0 )
    {
        pIns->type = Instruction::iDebugMessage;
        pIns->stringParam = tok + strlen(tok) + 1;
    }
    else if( strcmp(tok, "message.debug") == 0 )
    {
        pIns->type = Instruction::iDebugMessage;
        pIns->stringParam = tok + strlen(tok) + 1;
    }
    else if( strcmp(tok, "message.info") == 0 )
    {
        pIns->type = Instruction::iInfoMessage;
        pIns->stringParam = tok + strlen(tok) + 1;
    }
    else if( strcmp(tok, "message.warn") == 0 )
    {
        pIns->type = Instruction::iWarningMessage;
        pIns->stringParam = tok + strlen(tok) + 1;
    }
    else if( strcmp(tok, "message.error") == 0 )
    {
        pIns->type = Instruction::iErrorMessage;
        pIns->stringParam = tok + strlen(tok) + 1;
    }
    else if( strcmp(tok, "message.fatal") == 0 )
    {
        pIns->type = Instruction::iFatalMessage;
        pIns->stringParam = tok + strlen(tok) + 1;
    }
    else if( strcmp(tok, "cls") == 0 )
    {
        pIns->type = Instruction::iCls;
    }
    else if( strcmp(tok, "endscript") == 0 )
    {
        pIns->type = Instruction::iEndScript;
    }
    else if( strcmp(tok, "goto") == 0 )
    {
        pIns->type = Instruction::iGoto;
        pIns->stringParam = tok + strlen(tok) + 1;
    }
    else if( strcmp(tok, "sleep") == 0 )
    {
        pIns->type = Instruction::iSleep;
        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        processIntParameter(pIns, tok);
    }
    else if( strcmp(tok, "create") == 0 )
    {
        pIns->type = Instruction::iCreate;
        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        processIntParameter(pIns, tok);
    }
    else if( strcmp(tok, "delete") == 0 )
    {
        pIns->type = Instruction::iDelete;
        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        processIntParameter(pIns, tok);
    }
    else if( strcmp(tok, "join") == 0 )
    {
        pIns->type = Instruction::iJoin;
        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        processIntParameter(pIns, tok);
    }
    else if( strcmp(tok, "leave") == 0 )
    {
        pIns->type = Instruction::iLeave;
        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        processIntParameter(pIns, tok);
    }
    else if( strcmp(tok, "begintx") == 0 )
    {
        pIns->type = Instruction::iBeginTx;
        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        processIntParameter(pIns, tok);
    }
    else if( strcmp(tok, "endtx") == 0 )
    {
        pIns->type = Instruction::iEndTx;
        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        processIntParameter(pIns, tok);
    }
    else if( strcmp(tok, "muterx") == 0 )
    {
        pIns->type = Instruction::iMuteRx;
        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        processIntParameter(pIns, tok);
    }
    else if( strcmp(tok, "unmuterx") == 0 )
    {
        pIns->type = Instruction::iUnmuteRx;
        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        processIntParameter(pIns, tok);
    }
    else if( strcmp(tok, "mutetx") == 0 )
    {
        pIns->type = Instruction::iMuteTx;
        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        processIntParameter(pIns, tok);
    }
    else if( strcmp(tok, "unmutetx") == 0 )
    {
        pIns->type = Instruction::iUnmuteTx;
        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        processIntParameter(pIns, tok);
    }
    else if( strcmp(tok, "set") == 0 )
    {
        pIns->type = Instruction::iSet;

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        pIns->stringParam = tok;

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        processIntParameter(pIns, tok);
    }
    else if( strcmp(tok, "add") == 0 )
    {
        pIns->type = Instruction::iAdd;

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        pIns->stringParam = tok;

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        processIntParameter(pIns, tok);
    }
    else if( strcmp(tok, "sub") == 0 )
    {
        pIns->type = Instruction::iSub;

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        pIns->stringParam = tok;

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        processIntParameter(pIns, tok);
    }
    else if( strcmp(tok, "on") == 0 )
    {
        pIns->type = Instruction::iOnGoto;

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        pIns->stringParam = tok;

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        processIntParameter(pIns, tok);

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        if(strcmp(tok, "goto") != 0)
        {
            return false;
        }

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        pIns->stringParam2 = tok;
    }
    else if( strcmp(tok, "ifcondition") == 0 )
    {
        pIns->type = Instruction::iIfCondition;

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        pIns->stringParam = tok;

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        if(strcmp(tok, "goto") != 0)
        {
            return false;
        }

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        pIns->stringParam2 = tok;
    }    
    else if( strcmp(tok, "waitforengine") == 0 )
    {
        pIns->type = Instruction::iWaitForEngine;

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        pIns->stringParam = tok;

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        if(strcmp(tok, "goto") != 0)
        {
            std::cerr << "waitforengine: instruction invalid - no 'goto'" << std::endl;
            return false;
        }

        tok = strtok_s(nullptr, SEPS, &tokenCtx);


        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        pIns->stringParam2 = tok;

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        if(strcmp(tok, "after") != 0)
        {
            std::cerr << "waitforengine: instruction invalid - no 'after'" << std::endl;
            return false;
        }

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        processIntParameter(pIns, tok);
    }    
    
    else if( strcmp(tok, "waitforgroup") == 0 )
    {
        pIns->type = Instruction::iWaitForGroup;

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        processIntParameter(pIns, tok);

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        pIns->stringParam = tok;

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        if(strcmp(tok, "goto") != 0)
        {
            std::cerr << "waitforgroup: instruction invalid - no 'goto'" << std::endl;
            return false;
        }

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        pIns->stringParam2 = tok;

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        if(strcmp(tok, "after") != 0)
        {
            std::cerr << "waitforgroup: instruction invalid - no 'after'" << std::endl;
            return false;
        }

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        processIntParameter2(pIns, tok);
    }    
    else if( strcmp(tok, "enginestart") == 0 )
    {
        pIns->type = Instruction::iStartEngine;
    }
    else if( strcmp(tok, "enginestop") == 0 )
    {
        pIns->type = Instruction::iStopEngine;
    }
    else
    {
        std::cerr << "unknown instruction '" << tok << "'" << std::endl;
        return false;
    }

    return true;
}

bool loadScript(const char *pszFile)
{
	FILE *fp;

	#ifndef WIN32
		fp = fopen(pszFile, "rt");
	#else
		if (fopen_s(&fp, pszFile, "rt") != 0)
		{
			fp = nullptr;
		}
	#endif

    if(fp == nullptr)
    {
        std::cerr << "cannot load script file '" << pszFile << "'" << std::endl;
        return false;
    }

    char tmp[1024];
    char *lineStart;
    char *lineEnd;
    bool pLd = true;

    while( fgets(tmp, sizeof(tmp), fp) )
    {
        // Strip leading space
        lineStart = tmp;
        while(*lineStart != 0 && (*lineStart == '\n' || *lineStart == '\r' || *lineStart == '\t' || *lineStart == ' '))
        {
            lineStart++;
        }        

        // Skip empty lines and comments
        if(*lineStart == 0 || *lineStart == '#')
        {
            continue;
        }

        // Strip trailing junk
        lineEnd = lineStart + strlen(lineStart) - 1;
        while(lineEnd >= lineStart && (*lineEnd == '\n' || *lineEnd == '\r' || *lineEnd == '\t' || *lineEnd == ' '))
        {
            *lineEnd = 0;
            lineEnd--;
        }

        if(lineEnd <= lineStart)
        {
            continue;
        }

        if(strlen(lineStart) == 0)
        {
            continue;
        }

        // OK, we have something valid, let's see what it is
        Instruction ins;

        ins.clear();
        if(!interpret(lineStart, &ins))
        {
            pLd = false;
            break;
        }

        // Add the instruction
        g_scriptInstructions.push_back(ins);
    }

    fclose(fp);

    return pLd;
}

std::string replaceAll(std::string str, const std::string& from, const std::string& to) 
{
    size_t start_pos = 0;

    while((start_pos = str.find(from, start_pos)) != std::string::npos) 
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }

    return str;
}

std::string expandString(const char *s)
{
    size_t tokenStart = 0;
    std::string pLd;

    pLd = s;

    while( true )
    {
        tokenStart = pLd.find("${", tokenStart);
        if(tokenStart == std::string::npos)
        {
            break;
        }
        size_t tokenEnd = pLd.find("}", tokenStart);
        if(tokenEnd == std::string::npos)
        {
            break;
        }

        std::string varName = pLd.substr(tokenStart + 2, (tokenEnd - tokenStart - 2));
        std::string replSeapLdh = "${" + varName + "}";
        std::string replReplace;
        if(!varName.empty())
        {
            std::map<std::string, int>::iterator    itr = g_intVars.find(varName);
            if(itr != g_intVars.end())
            {
                char tmp[64];
                sprintf_s(tmp, sizeof(tmp), "%d", itr->second);
                replReplace = tmp;
            }
        }

        pLd = replaceAll(pLd, replSeapLdh, replReplace);
    }

    return pLd;
}

bool setVar(const char *nm, int val)
{
    if(g_verboseScript) std::cout << "setVar: " << nm << ", " << val << std::endl;
    std::map<std::string, int>::iterator    itr = g_intVars.find(nm);
    if(itr != g_intVars.end())
    {
        itr->second = val;
    }
    else
    {
        g_intVars[nm] = val;
    }    

    return true;
}

bool addVar(const char *nm, int val)
{
    if(g_verboseScript) std::cout << "addVar: " << nm << ", " << val << std::endl;
    std::map<std::string, int>::iterator    itr = g_intVars.find(nm);
    if(itr != g_intVars.end())
    {
        itr->second += val;
    }
    else
    {
        g_intVars[nm] = val;
    }    

    return true;
}

bool subVar(const char *nm, int val)
{
    if(g_verboseScript) std::cout << "subVar: " << nm << ", " << val << std::endl;
    std::map<std::string, int>::iterator    itr = g_intVars.find(nm);
    if(itr != g_intVars.end())
    {
        itr->second -= val;
    }
    else
    {
        g_intVars[nm] = val;
    }    

    return true;
}

bool compVar(const char *nm, int comp, int *pResult)
{
    std::map<std::string, int>::iterator    itr = g_intVars.find(nm);
    if(itr != g_intVars.end())
    {
        if(itr->second == comp)
        {
            *pResult = 0;
        }
        else if(itr->second < comp)
        {
            *pResult = -1;
        }
        else
        {
            *pResult = 1;
        }        
    }
    else
    {
        return false;
    }    

    return true;
}

bool runScript()
{
    bool endScript = false;
    bool pLd = false;
    bool error = false;
    std::list<Instruction>::iterator itrIns;

    itrIns = g_scriptInstructions.begin();
    while( !error && !endScript && itrIns != g_scriptInstructions.end() )
    {
        Instruction ins = *itrIns;
        switch( ins.type )
        {
            case Instruction::iUnknown:
            {
                std::cerr << "unknown instruction" << std::endl;
            }
            break;

            case Instruction::iEndScript:            
            {
                if(g_verboseScript) std::cout << "endscript" << std::endl;
                endScript = true;
                pLd = true;
            }
            break;

            case Instruction::iCls:            
            {
                if(g_verboseScript) std::cout << "cls" << std::endl;
                #ifdef WIN32
                    system("cls");
                #else
                    if(system("clear") != 0)
                    {
                        std::cout << "system('clear') failed" << std::endl;
                    }
                #endif
                itrIns++;
            }
            break;

            case Instruction::iLabel:
            {
                if(g_verboseScript) std::cout << "label: " << ins.stringParam << std::endl;
                itrIns++;
            }
            break;

            case Instruction::iGoto:
            {
                if(g_verboseScript) std::cout << "goto: " << ins.stringParam << std::endl;
                std::list<Instruction>::iterator itrFind;
                for(itrFind = g_scriptInstructions.begin();
                    itrFind != g_scriptInstructions.end();
                    itrFind++)
                {
                    if(itrFind->type == Instruction::iLabel && itrFind->stringParam.compare(ins.stringParam) == 0)
                    {
                        break;
                    }
                }

                if(itrFind == g_scriptInstructions.end())
                {
                    std::cerr << "goto: label '" << ins.stringParam << "' not found!" << std::endl;
                }

                itrIns = itrFind;
            }
            break;

            case Instruction::iFatalMessage:
            {                
                if(g_verboseScript) std::cout << "iFatalMessage: " << ins.stringParam << std::endl;
                std::string expanded = expandString(ins.stringParam.c_str());
                engageLogMsg(0, "Script", expanded.c_str());
                itrIns++;
            }
            break;

            case Instruction::iErrorMessage:
            {                
                if(g_verboseScript) std::cout << "iErrorMessage: " << ins.stringParam << std::endl;
                std::string expanded = expandString(ins.stringParam.c_str());
                engageLogMsg(1, "Script", expanded.c_str());
                itrIns++;
            }
            break;

            case Instruction::iWarningMessage:
            {                
                if(g_verboseScript) std::cout << "iWarningMessage: " << ins.stringParam << std::endl;
                std::string expanded = expandString(ins.stringParam.c_str());
                engageLogMsg(2, "Script", expanded.c_str());
                itrIns++;
            }
            break;

            case Instruction::iInfoMessage:
            {                
                if(g_verboseScript) std::cout << "iInfoMessage: " << ins.stringParam << std::endl;
                std::string expanded = expandString(ins.stringParam.c_str());
                engageLogMsg(3, "Script", expanded.c_str());
                itrIns++;
            }
            break;

            case Instruction::iDebugMessage:
            {                
                if(g_verboseScript) std::cout << "iDebugMessage: " << ins.stringParam << std::endl;
                std::string expanded = expandString(ins.stringParam.c_str());
                engageLogMsg(4, "Script", expanded.c_str());
                itrIns++;
            }
            break;

            case Instruction::iSleep:
            {
                int ms = ins.getExpandedIntParam(ins.intParam);
                if(g_verboseScript) std::cout << "sleep: " << ms << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                itrIns++;
            }
            break;

            case Instruction::iCreate:
            {
                doCreate(ins.getExpandedIntParam(g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iDelete:
            {
                doDelete(ins.getExpandedIntParam(g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iJoin:
            {
                doJoin(ins.getExpandedIntParam(g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iLeave:
            {
                doLeave(ins.getExpandedIntParam(g_new_groups.size()));
                itrIns++;
            }
            break;
     
            case Instruction::iBeginTx:
            {
                doBeginTx(ins.getExpandedIntParam(g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iEndTx:
            {
                doEndTx(ins.getExpandedIntParam(g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iMuteRx:
            {
                doMuteRx(ins.getExpandedIntParam(g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iUnmuteRx:
            {
                doUnmuteRx(ins.getExpandedIntParam(g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iMuteTx:
            {
                doMuteTx(ins.getExpandedIntParam(g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iUnmuteTx:
            {
                doUnmuteTx(ins.getExpandedIntParam(g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iSet:
            {
                error = !setVar(ins.stringParam.c_str(), ins.intParam);
                itrIns++;                    
            }
            break;

            case Instruction::iAdd:
            {
                error = !addVar(ins.stringParam.c_str(), ins.intParam);
                itrIns++;                    
            }
            break;

            case Instruction::iSub:
            {
                error = !subVar(ins.stringParam.c_str(), ins.intParam * -1);
                itrIns++;                    
            }
            break;

            case Instruction::iCompare:
            {
                int result = 0;
                error = !compVar(ins.stringParam.c_str(), ins.intParam, &result);
                if(!error)
                {
                    // what do with the comparison result
                }
                itrIns++;
            }
            break;        

            case Instruction::iOnGoto:
            {
                if(g_verboseScript) std::cout << "ongoto: " << ins.stringParam << std::endl;

                ins.intParam = ins.getExpandedIntParam(0);
                ins.intParam2 = ins.getExpandedIntParam2(0);

                std::map<std::string, int>::iterator    itr = g_intVars.find(ins.stringParam);
                if(itr == g_intVars.end())
                {
                    std::cerr << "var: '" << ins.stringParam << "' not found!" << std::endl;
                    error = true;
                }
                else
                {
                    if(itr->second == ins.intParam)
                    {
                        std::list<Instruction>::iterator itrFind;
                        for(itrFind = g_scriptInstructions.begin();
                            itrFind != g_scriptInstructions.end();
                            itrFind++)
                        {
                            if(itrFind->type == Instruction::iLabel && itrFind->stringParam.compare(ins.stringParam2) == 0)
                            {
                                break;
                            }
                        }

                        if(itrFind == g_scriptInstructions.end())
                        {
                            std::cerr << "ongoto: label '" << ins.stringParam2 << "' not found!" << std::endl;
                            error = true;
                        }

                        itrIns = itrFind;
                    }
                    else
                    {
                        itrIns++;
                    }                    
                }                
            }
            break;

            case Instruction::iStartEngine:
            {
                doStartEngine();
                itrIns++;
            }
            break;

            case Instruction::iStopEngine:
            {
                doStopEngine();
                itrIns++;
            }
            break;

            case Instruction::iIfCondition:
            {
                /*
                if(g_verboseScript) std::cout << "ifcondition: " << ins.stringParam << std::endl;

                bool foundIt = false;
                bool conditionSatisfied = false;
                for(size_t x = 0; g_ifConditions[x].nm; x++)
                {
                    if(ins.stringParam.compare(g_ifConditions[x].nm) == 0)
                    {
                        foundIt = true;
                        conditionSatisfied = *g_ifConditions[x].bl;
                    }
                }

                if(!foundIt)
                {
                    std::cerr << "ifcondition: condition '" << ins.stringParam << "' not found!" << std::endl;
                    error = true;
                }
                else
                {
                    if(conditionSatisfied)
                    {
                        std::list<Instruction>::iterator itrFind;
                        for(itrFind = g_scriptInstructions.begin();
                            itrFind != g_scriptInstructions.end();
                            itrFind++)
                        {
                            if(itrFind->type == Instruction::iLabel && itrFind->stringParam.compare(ins.stringParam2) == 0)
                            {
                                break;
                            }
                        }

                        if(itrFind == g_scriptInstructions.end())
                        {
                            std::cerr << "ifcondition: label '" << ins.stringParam2 << "' not found!" << std::endl;
                            abort();
                        }

                        itrIns = itrFind;
                    }
                    else
                    {
                        itrIns++;
                    }
                }
                */
            }
            break;
            
            case Instruction::iWaitForEngine:
            {
                if(g_verboseScript) std::cout << "waitforengine: " << ins.stringParam << std::endl;

                // intParam1 is the timeout
                // stringParam is the condition name
                // stringParam2 is the label to goto upon timeout

                ins.intParam = ins.getExpandedIntParam(0);
                ins.intParam2 = ins.getExpandedIntParam2(0);

                if(ins.intParam < 0)
                {
                    std::cerr << "waitforengine: timeout '" << ins.intParam << " invalid!" << std::endl;
                    error = true;
                    break;
                }

                uint64_t started;
                uint64_t now;
                bool timedOut = false;

                started = getTickMs();
                while( true )
                {
                    // Have we timed out?
                    now = getTickMs();
                    if( now - started >= (uint64_t) ins.intParam)
                    {
                        timedOut = true;
                        break;
                    }

                    if( 
                            ((ins.stringParam.compare("started") == 0) && (g_engineStarted)) ||
                            ((ins.stringParam.compare("stopped") == 0) && (g_engineStopped)) 
                        )
                    {
                        break;
                    }

                    // Sleep for the smallest (reasonable) time possible
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }

                if( !error && timedOut )
                {
                    std::list<Instruction>::iterator itrFind;
                    for(itrFind = g_scriptInstructions.begin();
                        itrFind != g_scriptInstructions.end();
                        itrFind++)
                    {
                        if(itrFind->type == Instruction::iLabel && itrFind->stringParam.compare(ins.stringParam2) == 0)
                        {
                            std::cout << "waitforengine '" << ins.stringParam << "' timeout" << std::endl;
                            break;
                        }
                    }

                    if(itrFind == g_scriptInstructions.end())
                    {
                        std::cerr << "waitforengine: label '" << ins.stringParam2 << "' not found!" << std::endl;
                        abort();
                    }

                    itrIns = itrFind;
                }
                else
                {
                    itrIns++;
                }
            }
            break;

            case Instruction::iWaitForGroup:
            {
                if(g_verboseScript) std::cout << "waitforgroup: " << ins.stringParam << std::endl;

                ins.intParam = ins.getExpandedIntParam(0);
                ins.intParam2 = ins.getExpandedIntParam2(0);

                // intParam is the index
                // intParam2 is the timeout
                // stringParam is the condition name
                // stringParam2 is the label to goto upon timeout

                if(ins.intParam < 0 || ins.intParam >= (int)g_new_groups.size())
                {
                    std::cerr << "waitforgroup: index '" << ins.intParam << " invalid!" << std::endl;
                    error = true;
                    break;
                }

                if(ins.intParam2 < 0)
                {
                    std::cerr << "waitforgroup: timeout '" << ins.intParam2 << " invalid!" << std::endl;
                    error = true;
                    break;
                }

                uint64_t started;
                uint64_t now;
                bool timedOut = false;

                started = getTickMs();
                while( true )
                {
                    // Have we timed out?
                    now = getTickMs();
                    if( now - started >= (uint64_t) ins.intParam2)
                    {
                        timedOut = true;
                        break;
                    }

                    if( 
                            ((ins.stringParam.compare("created") == 0) && (g_new_groups[ins.intParam].created)) ||
                            ((ins.stringParam.compare("createfailed") == 0) && (g_new_groups[ins.intParam].createFailed)) ||
                            ((ins.stringParam.compare("deleted") == 0) && (g_new_groups[ins.intParam].deleted)) ||

                            ((ins.stringParam.compare("joined") == 0) && (g_new_groups[ins.intParam].joined)) ||
                            ((ins.stringParam.compare("joinfailed") == 0) && (g_new_groups[ins.intParam].joinFailed)) ||
                            ((ins.stringParam.compare("notjoined") == 0) && (g_new_groups[ins.intParam].notJoined)) ||
                                                    
                            ((ins.stringParam.compare("connected") == 0) && (g_new_groups[ins.intParam].connected)) ||
                            ((ins.stringParam.compare("connectfailed") == 0) && (g_new_groups[ins.intParam].connectFailed)) ||
                            ((ins.stringParam.compare("notconnected") == 0) && (g_new_groups[ins.intParam].notConnected)) ||
                                                    
                            ((ins.stringParam.compare("rxstarted") == 0) && (g_new_groups[ins.intParam].rxStarted)) ||
                            ((ins.stringParam.compare("rxended") == 0) && (g_new_groups[ins.intParam].rxEnded)) ||
                                                    
                            ((ins.stringParam.compare("txstarted") == 0) && (g_new_groups[ins.intParam].txStarted)) ||
                            ((ins.stringParam.compare("txended") == 0) && (g_new_groups[ins.intParam].txEnded)) ||
                            ((ins.stringParam.compare("txfailed") == 0) && (g_new_groups[ins.intParam].txFailed))
                        )
                    {
                        break;
                    }

                    // Sleep for the smallest (reasonable) time possible
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }

                if( !error && timedOut )
                {
                    std::list<Instruction>::iterator itrFind;
                    for(itrFind = g_scriptInstructions.begin();
                        itrFind != g_scriptInstructions.end();
                        itrFind++)
                    {
                        if(itrFind->type == Instruction::iLabel && itrFind->stringParam.compare(ins.stringParam2) == 0)
                        {
                            std::cout << "waitforgroup '" << ins.stringParam << "' timeout" << std::endl;
                            break;
                        }
                    }

                    if(itrFind == g_scriptInstructions.end())
                    {
                        std::cerr << "waitforgroup: label '" << ins.stringParam2 << "' not found!" << std::endl;
                        abort();
                    }

                    itrIns = itrFind;
                }
                else
                {
                    itrIns++;
                }
            }
            break;
        }
    }

    return pLd;
}

bool registepLdallbacks()
{
    EngageEvents_t cb;

    memset(&cb, 0, sizeof(cb));

    cb.PFN_ENGAGE_ENGINE_STARTED = on_ENGAGE_ENGINE_STARTED;
    cb.PFN_ENGAGE_ENGINE_STOPPED = on_ENGAGE_ENGINE_STOPPED;

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

    cb.PFN_ENGAGE_GROUP_MEMBER_COUNT_CHANGED = on_ENGAGE_GROUP_MEMBER_COUNT_CHANGED;

    cb.PFN_ENGAGE_GROUP_RX_STARTED = on_ENGAGE_GROUP_RX_STARTED;
    cb.PFN_ENGAGE_GROUP_RX_ENDED = on_ENGAGE_GROUP_RX_ENDED;
    cb.PFN_ENGAGE_GROUP_RX_SPEAKERS_CHANGED = on_ENGAGE_GROUP_RX_SPEAKERS_CHANGED;
    cb.PFN_ENGAGE_GROUP_RX_MUTED = on_ENGAGE_GROUP_RX_MUTED;
    cb.PFN_ENGAGE_GROUP_RX_UNMUTED = on_ENGAGE_GROUP_RX_UNMUTED;

    cb.PFN_ENGAGE_GROUP_TX_STARTED = on_ENGAGE_GROUP_TX_STARTED;
    cb.PFN_ENGAGE_GROUP_TX_ENDED = on_ENGAGE_GROUP_TX_ENDED;
    cb.PFN_ENGAGE_GROUP_TX_FAILED = on_ENGAGE_GROUP_TX_FAILED;
    cb.PFN_ENGAGE_GROUP_TX_USURPED_BY_PRIORITY = on_ENGAGE_GROUP_TX_USURPED_BY_PRIORITY;
    cb.PFN_ENGAGE_GROUP_MAX_TX_TIME_EXCEEDED = on_ENGAGE_GROUP_MAX_TX_TIME_EXCEEDED;
    cb.PFN_ENGAGE_GROUP_TX_MUTED = on_ENGAGE_GROUP_TX_MUTED;
    cb.PFN_ENGAGE_GROUP_TX_UNMUTED = on_ENGAGE_GROUP_TX_UNMUTED;

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

    return (engageRegisterEventCallbacks(&cb) == ENGAGE_RESULT_OK);
}
