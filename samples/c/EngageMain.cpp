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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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
#include "EngageNetworkDevice.h"
#include "EngagePlatformNotifications.h"
#include "lua.hpp"

#include "Crypto.hpp"
#include "ConfigurationObjects.h"
#include "Utils.hpp"


//===================================================================
#include <string>
#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <functional>
#include <map>
#include <list>
#include <forward_list>
#include <memory.h>
#include <condition_variable>


#if defined(RTS_SUPPORT_NCURSES)
    #include <ncurses.h>

    WINDOW *ncRxActivityWin = nullptr;
    WINDOW *ncNodeWin = nullptr;
    WINDOW *ncLogWin = nullptr;
    std::mutex ncLock;
    std::atomic<bool> ncRefreshRequested;
    uint64_t ncRefreshUpdates = 0;
    static int CLR_RX_ON = 100;
    static int CLR_RX_OFF = 101;

    WINDOW *ncCreateNewWin(const char *title, int height, int width, int starty, int startx);
#endif

void ncInit();
void ncLogThis(int level, const char *s);
void ncShutdown();
void ncRefreshUi();
void ncLoop();
void ncRefreshScreen();

#if defined(WIN32)
#include <conio.h>

bool myKbHit(void)
{
    return (_kbhit() != 0);
}

#else
#include <termios.h>
#include <sys/ioctl.h>

bool myKbHit(void)
{
    static bool initflag = false;
    static const int STDIN = 0;

    if(!initflag)
    {
        struct termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initflag = true;
    }

    int nbbytes;
    ioctl(STDIN, FIONREAD, &nbbytes);
    return (nbbytes > 0);
}
#endif

uint32_t myRand32()
{
    return (((uint32_t)rand() << 1) | (rand()));
}

uint64_t myRand64()
{
    uint64_t r1 = (((uint64_t) myRand32()) << 32);
    uint64_t r2 = (((uint64_t) myRand32()));
    return (r1 | r2);
}

std::string getWaitingKeyboardInput()
{
    std::string rc;

    if(myKbHit())
    {
        std::cin >> rc;
    }

    return rc;
}

class WorkQueue
{
public:   
    static const size_t DEFAULT_MAX_DEPTH = 512;
    
    WorkQueue();
    virtual ~WorkQueue();
    
    void start();
    void stop();
    void restart();

    bool submit(std::function<void()> op);
    bool submitAndWait(std::function<void()> op);
    
    inline void setMaxDepth(size_t d)
    {
        _maxDepth = d;
    }
    
    inline size_t getMaxDepth()
    {
        return _maxDepth;
    }
    
    inline void enableSubmissions()
    {
        _executorLock.lock();
        _allowSubmissions = true;
        _executorLock.unlock();
    }

    inline void disableSubmissions()
    {
        _executorLock.lock();
        _allowSubmissions = false;
        _executorLock.unlock();
    }

    void reset();

    inline void setName(const char *nm)
    {
        _name = (nm == nullptr ? "" : nm);
    }
    
private:
    class Sem
    {
    public:
        Sem()
        {
            sig = false;
        }
        
        virtual ~Sem()
        {
        }
        
        inline void notify()
        {
            {
                std::lock_guard<std::mutex> lck(mutex_);
                sig = true;
            }
            
            condVar.notify_one();
        }
        
        inline void wait()
        {
            std::unique_lock<std::mutex> lck(mutex_);
            condVar.wait(lck, [this]{ return sig; });
            sig = false;
        }
        
        inline void waitFor(int ms)
        {
            std::unique_lock<std::mutex> lck(mutex_);
            condVar.wait_for(lck, std::chrono::milliseconds(ms), [this]{ return sig; });
            sig = false;
        }
        
        inline void reset()
        {
            sig = false;
        }
        
    private:
        std::mutex mutex_;
        std::condition_variable condVar;
        bool sig;
    };

    class Lambda
    {
    public:
        Lambda(std::function<void()> r)
        {
            _r = r;
            _blockSemToSignal = nullptr;
        }

        std::function<void()>   _r;
        Sem                     *_blockSemToSignal;
    };

    bool                            _running;
    bool                            _fatalError;
    std::mutex                      _executorLock;
    std::list<Lambda*>              _queue;
    std::forward_list<Lambda*>      _pool;
    size_t                          _poolSize;
    std::map<uint32_t, int64_t>     _counters;
    std::thread                     _dispatchThread;
    Sem                             _semSignalAction;
    Sem                             _semReady;
    Sem                             _semDone;
    size_t                          _maxDepth;
    bool                            _allowSubmissions;
    std::string                     _name;

    Lambda *createLambda(std::function<void()> r);

    void dispatcher();
    void clear();
    void returnToPool(Lambda *l);
};

WorkQueue::WorkQueue()
{
    _running = false;
    _fatalError = false;
    _maxDepth = WorkQueue::DEFAULT_MAX_DEPTH;

    _poolSize = 0;
    _allowSubmissions = true;
}

WorkQueue::~WorkQueue()
{
    stop();
}

void WorkQueue::start()
{
    _running = true;
    _fatalError = false;

    _semSignalAction.reset();
    _semReady.reset();
    _semDone.reset();
    _dispatchThread = std::thread(&WorkQueue::dispatcher, this);
    _semReady.wait();
}

void WorkQueue::stop()
{
    if(_running && _dispatchThread.joinable())
    {
        _running = false;
        _semSignalAction.notify();
        _semDone.wait();
        _dispatchThread.join();
    }
    
    clear();
}

void WorkQueue::restart()
{
    stop();
    start();
}

void WorkQueue::reset()
{
    Lambda *op;

    _executorLock.lock();
    
    while( !_queue.empty() )
    {
        op = _queue.front();
        _queue.pop_front();
        returnToPool(op);
    }

    _executorLock.unlock();
}

bool WorkQueue::submit(std::function<void()> op)
{
    if(_running && !_fatalError)
    {
        _executorLock.lock();
        
        if(_allowSubmissions)
        {
            if(_maxDepth > 0)
            {
                if(_queue.size() >= _maxDepth)
                {
                    _executorLock.unlock();
                    
                    return false;
                }
            }
            
            _queue.push_back(createLambda(op));
            _executorLock.unlock();
            
            _semSignalAction.notify();
        }
        else
        {
            _executorLock.unlock();
        }
        
        return (!_fatalError);
    }
    else
    {
        return false;
    }
}

bool WorkQueue::submitAndWait(std::function<void()> op)
{
    if(_running && !_fatalError)
    {
        _executorLock.lock();

        if(_allowSubmissions)
        {
            Sem *semBlock = new Sem();

            if(_maxDepth > 0)
            {
                if((_queue.size() + 1) >= _maxDepth)
                {
                    delete semBlock;
                    _executorLock.unlock();
                    return false;
                }
            }
            
            _queue.push_back(createLambda(op));

            std::string theName;

            Lambda *sig = createLambda([semBlock]()
            {
                semBlock->notify();
            });

            sig->_blockSemToSignal = semBlock;
            
            _queue.push_back(sig);

            _executorLock.unlock();
            
            _semSignalAction.notify();
            
            semBlock->wait();
            delete semBlock;
        }
        else
        {
            _executorLock.unlock();
        }        

        return (!_fatalError);
    }
    else
    {
        return false;
    }
}

void WorkQueue::dispatcher()
{
    PlatformSetThreadName(_name.c_str());

    _semReady.notify();
    
    Lambda *op;

    while( _running && !_fatalError )
    {
        _semSignalAction.wait();
        
        while( _running && !_fatalError )
        {
            _executorLock.lock();
            {
                if(_queue.empty())
                {
                    _executorLock.unlock();
                    break;
                }
                else
                {
                    op = _queue.front();
                    _queue.pop_front();
                }
            }
            _executorLock.unlock();
            
            // Execute
            (op->_r)();

            _executorLock.lock();
            {
                // We're done with it
                returnToPool(op);
            }
            _executorLock.unlock();
        }
    }
    
    _semDone.notify();
}

// NOTE: Not called under lock
void WorkQueue::clear()
{
    _executorLock.lock();
    {
        while(!_queue.empty())
        {
            Lambda *op = _queue.front();
            _queue.pop_front();
            delete op;
        }
        
        while(!_pool.empty())
        {
            Lambda *op = _pool.front();
            _pool.pop_front();
            delete op;
        }

        _poolSize = 0;
    }
    _executorLock.unlock();        
}

// NOTE: Called under lock
WorkQueue::Lambda *WorkQueue::createLambda(std::function<void()> r)
{
    Lambda *rc = nullptr;
    
    if(_poolSize == 0)
    {
        rc = new Lambda(r);
    }
    else
    {
        rc = _pool.front();
        _pool.pop_front();
        rc->_r = r;
        rc->_blockSemToSignal = nullptr;

        _poolSize--;
    }
    
    return rc;
}

// NOTE: Called under lock
void WorkQueue::returnToPool(Lambda *l)
{
    if(_poolSize < 100)
    {
        l->_blockSemToSignal = nullptr;
        _pool.push_front(l);
        _poolSize++;
    }
    else
    {
        delete l;
    }
}

WorkQueue   g_wq;
//===================================================================

std::string readTextFile(const char *pszFn)
{
    FILE *fp;
    std::string rc;

    #ifndef WIN32
        fp = fopen(pszFn, "rb");
    #else
        if(fopen_s(&fp, pszFn, "rb") != 0)
        {
            fp = nullptr;
        }
    #endif

    if(fp != nullptr)
    {
        fseek(fp, 0, SEEK_END);
        size_t sz = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        char *buff = new char[sz + 1];
        if( fread(buff, 1, sz, fp) == sz )
        {
            buff[sz] = 0;
            rc = buff;
        }
        delete[] buff;

        fclose(fp);
    }

    return rc;
}

class MiniLogger
{
private:
    static const size_t MAX_LOG_BUFFER_SIZE = (8192 * 4);

public:
    MiniLogger()
    {
        _pszBuffer = new char[MAX_LOG_BUFFER_SIZE + 1];
    }

    virtual ~MiniLogger()
    {
        delete[] _pszBuffer;
    }

    void doOutput(int level, const char *pszTag, const char *msg)
    {
        //engageLogMsg(level, pszTag, msg);
        printf("[%08" PRIu64 "][%d]:[%s]:%s\n", Utils::getTickMs(), level, pszTag, msg);
    }

    void f(const char *pszTag, const char *pszFmt, ...)
    {
        std::lock_guard<std::mutex>   scopedLock(_lock);

        va_list args;
        va_start(args, pszFmt);
        vsnprintf(_pszBuffer, MAX_LOG_BUFFER_SIZE, pszFmt, args);

        doOutput(0, pszTag, _pszBuffer);

        va_end(args);
    }

    void e(const char *pszTag, const char *pszFmt, ...)
    {
        std::lock_guard<std::mutex>   scopedLock(_lock);

        va_list args;
        va_start(args, pszFmt);
        vsnprintf(_pszBuffer, MAX_LOG_BUFFER_SIZE, pszFmt, args);

        doOutput(1, pszTag, _pszBuffer);

        va_end(args);
    }

    void w(const char *pszTag, const char *pszFmt, ...)
    {
        std::lock_guard<std::mutex>   scopedLock(_lock);

        va_list args;
        va_start(args, pszFmt);
        vsnprintf(_pszBuffer, MAX_LOG_BUFFER_SIZE, pszFmt, args);

        doOutput(2, pszTag, _pszBuffer);

        va_end(args);
    }

    void i(const char *pszTag, const char *pszFmt, ...)
    {
        std::lock_guard<std::mutex>   scopedLock(_lock);

        va_list args;
        va_start(args, pszFmt);
        vsnprintf(_pszBuffer, MAX_LOG_BUFFER_SIZE, pszFmt, args);

        doOutput(3, pszTag, _pszBuffer);

        va_end(args);
    }

    void d(const char *pszTag, const char *pszFmt, ...)
    {
        std::lock_guard<std::mutex>   scopedLock(_lock);

        va_list args;
        va_start(args, pszFmt);
        vsnprintf(_pszBuffer, MAX_LOG_BUFFER_SIZE, pszFmt, args);

        doOutput(4, pszTag, _pszBuffer);

        va_end(args);
    }

private:
    std::mutex       _lock;
    char            *_pszBuffer;
};

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
    bool        txUsurped;

    bool        isDynamic;

    std::list<std::string> talkers;

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
        txUsurped = false;

        isDynamic = false;
    }
};

static const char *TAG = "EngageCmd";

const size_t MAX_CMD_BUFF_SIZE = 4096;

std::vector<GroupInfo>    g_new_groups;
std::map<std::string, ConfigurationObjects::PresenceDescriptor>    g_nodes;

int g_txPriority = 0;
uint32_t g_txFlags = 0;
bool g_anonymous = false;
bool g_useadad = false;
const char *g_adadMicFile = nullptr;
const char *g_adadSpkFile = nullptr;
int g_adadIntervalMs = 20;
int g_adadSampleRate = 16000;
int g_adadChannels = 2;
ConfigurationObjects::EnginePolicy g_enginePolicy;
ConfigurationObjects::Mission g_mission;
ConfigurationObjects::Rallypoint *g_singleRallypoint = nullptr;
ConfigurationObjects::RallypointCluster *g_rallypointCluster = nullptr;
int g_multicastFailoverPolicy = -1;
char g_szUserConfigAlias[16 +1] = {0};
char g_szUseConfigUserId[64 +1] = {0};
char g_szUserConfigDisplayName[64 + 1] = {0};
char g_szNodeId[64 + 1] = {0};
uint16_t g_uiUserConfigAliasSpecializer = 0;
bool g_verboseScript = false;
bool g_quiet = false;
const char *g_pszPresenceFile = nullptr;
const char *g_desiredSpeakerName = nullptr;
const char *g_desiredMicrophoneName = nullptr;
bool g_engineStarted = false;
bool g_engineStarting = false;
bool g_engineStopped = true;
bool g_shutdownEngineOnStopped = false;
bool g_restartEngineOnStopped = false;
bool g_reinitEngineOnStopped = false;
bool g_reinitInProgress = false;

MiniLogger g_miniLogger;
const char *g_missionFile = nullptr;
const char *g_epFile = nullptr;
const char *g_rpFile = nullptr;
const char *g_rpcFile = nullptr;
const char *g_pszScript = nullptr;

int16_t g_speakerDeviceId = 0;
int16_t g_microphoneDeviceId = 0;
int16_t g_nextAudioDeviceInstanceId = 0;
int16_t g_networkDeviceId = 0;

int16_t g_desiredSpeakerDeviceId = 0;
int16_t g_desiredMicrophoneDeviceId = 0;
std::string g_nicName;
std::string g_certStore;
std::string g_certStorePwd;
std::string g_enginePolicyJson;
std::string g_userIdentityJson;
std::string g_txUriName;
int g_txUriRepeatCount = 0;
bool g_transmitFileAndExit = false;
bool g_autoCreateAndJoin = false;
bool g_proceedWithExit = false;
int g_exitCode = 0;
bool g_ncurses = false;
const char *g_netDeviceName = nullptr;
const char *g_fipsCryptoEnginePath = nullptr;
bool g_fipsCryptoDebug = false;

bool processCommandBuffer(char *buff);
void showUsage();
void showHelp();
void showGroups();
void showNodes();
bool registerCallbacks();
bool loadScript(const char *pszFile);
bool runScript();
bool loadPolicy(const char *pszFn, ConfigurationObjects::EnginePolicy *pPolicy);
bool loadMission(const char *pszFn, ConfigurationObjects::Mission *pMission, 
                                    ConfigurationObjects::Rallypoint *pSingleRp, 
                                    ConfigurationObjects::RallypointCluster *pRpCluster,
                                    int *pMulticastFailoverPolicy);
bool loadSingleRp(const char *pszFn, ConfigurationObjects::Rallypoint *pRp);
bool loadRpCluster(const char *pszFn, ConfigurationObjects::RallypointCluster *pRpCluster);


void doStartEngine();
void doStopEngine();
void doRestartEngine();
void doReinitEngine();
void doUpdatePresence(int index);
void doCreate(int index);
void doCreateUpTo(int index);
void doDelete(int index);
void doJoin(int index);
void doJoinUpTo(int index);
void doLeave(int index);
void doBeginTx(int index);
void doEndTx(int index);
void doMuteRx(int index);
void doGetStats(int index);
void doGetHealth(int index);
void doUnmuteRx(int index);
void doMuteTx(int index);
void doUnmuteTx(int index);
void doSetGroupRxTag(int index, int tag);
void doSendBlob(int index, const uint8_t* blob, size_t size, const char *jsonParams);
void doSendRtp(int index, const uint8_t* payload, size_t size, const char *jsonParams);
void doSendRaw(int index, const uint8_t* raw, size_t size, const char *jsonParams);
void doRegisterGroupRtpHandler(int index, int payloadId);
void doUnregisterGroupRtpHandler(int index, int payloadId);
void doGenerateMission(const char *passphrase, int channelCount, const char *rallypoint, const char *name, const char *fn);
void generateMission();
void doSetGroupAudioIds(int index, int inputId, int outputId);

void registerADAD();
void unregisterADAD();
void showAudioDevices();
void showNetworkDevices();

void registerNETD();
void unregisterNETD();

int indexOfGroupId(const char *id);

int runLuaScript(const char *fn);

#if defined(RTS_HAVE_DBUS)
class MyDbusNotifications : public SimpleDbusSignalListener::IDbusSignalNotification
{
public:
    virtual void onDbusError(const void *ctx, const char *nm, const char *msg) const
    {
        char logBuff[1024];
        snprintf(logBuff, sizeof(logBuff), "onDbusError [%s] [%s]", nm, msg);
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
        snprintf(logBuff, sizeof(logBuff), "onDbusSignalReceived [%s]", signal);
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
    if( !fgets(buff, (int)maxSize, stdin) )
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


void devTest1()
{
    for(size_t x = 0; x < g_new_groups.size(); x++)
    {
        engageSetGroupRxVolume(g_new_groups[x].id.c_str(), 100, 100);
    }
}

void devTest2()
{
    for(size_t x = 0; x < g_new_groups.size(); x++)
    {
        engageSetGroupRxVolume(g_new_groups[x].id.c_str(), 200, 200);
    }
}

void devTest3()
{
    for(size_t x = 0; x < g_new_groups.size(); x++)
    {
        engageSetGroupRxVolume(g_new_groups[x].id.c_str(), 300, 300);
    }
}

void devTest4()
{
    for(size_t x = 0; x < g_new_groups.size(); x++)
    {
        engageSetGroupRxVolume(g_new_groups[x].id.c_str(), 400, 400);
    }
}

void devTest5()
{
    for(size_t x = 0; x < g_new_groups.size(); x++)
    {
        engageSetGroupRxVolume(g_new_groups[x].id.c_str(), 500, 500);
    }
}

void devTest6()
{
    for(size_t x = 0; x < g_new_groups.size(); x++)
    {
        engageSetGroupRxVolume(g_new_groups[x].id.c_str(), 600, 600);
    }
}

void devTest7()
{
    for(size_t x = 0; x < g_new_groups.size(); x++)
    {
        engageSetGroupRxVolume(g_new_groups[x].id.c_str(), 700, 700);
    }
}

void devTest8()
{
}

void processInit()
{
#if defined(WIN32)
	engageWin32LibraryInit();
#endif
}

void processDeinit()
{
#if defined(WIN32)
	engageWin32LibraryDeinit();
#endif
}

bool doAppInitialization()
{
    int rc;
    bool ok = false;

    // If we're not going to be anonymous then create our user configuration json
    if(!g_anonymous)
    {
        // Make a random alias if we don't have one
        if(!g_szUserConfigAlias[0])
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

            snprintf(g_szUserConfigAlias, sizeof(g_szUserConfigAlias), "%s%012X", prefix, myRand32());
        }

        // Cook up a user id if we don't have one
        if(!g_szUseConfigUserId[0])
        {
            snprintf(g_szUseConfigUserId, sizeof(g_szUseConfigUserId), "%s@engagedev.rallytac.com", g_szUserConfigAlias);
        }

        // Cook up a display name if we don't have one
        if(!g_szUserConfigDisplayName[0])
        {
            snprintf(g_szUserConfigDisplayName, sizeof(g_szUserConfigDisplayName), "User %s", g_szUserConfigAlias);
        }
    }

    // Make sure our transmit priority is a valid value
    if(g_txPriority < 0 || g_txPriority > 255)
    {
        g_txPriority = 0;
    }

    // Load our engine policy (if any)
    if(g_epFile != nullptr)
    {
        if(!loadPolicy(g_epFile, &g_enginePolicy))
        {
            std::cerr << "could not load engine policy '" << g_epFile << "'" << std::endl;
            goto end_function;
        }
    }

    // Override the policy NIC if provided
    if(!g_nicName.empty())
    {
        g_enginePolicy.networking.defaultNic = g_nicName;
    }

    // Load our mission
    delete g_singleRallypoint;
    g_singleRallypoint = new ConfigurationObjects::Rallypoint();

    delete g_rallypointCluster;
    g_rallypointCluster = new ConfigurationObjects::RallypointCluster();

    if(!loadMission(g_missionFile, &g_mission, g_singleRallypoint, g_rallypointCluster, &g_multicastFailoverPolicy))
    {
        std::cerr << "could not load mission '" << g_missionFile << "'" << std::endl;
        goto end_function;
    }

    // Load our single RP (if any)
    if(g_rpFile != nullptr)
    {
        if(!loadSingleRp(g_rpFile, g_singleRallypoint))
        {
            std::cerr << "could not load rallypoint '" << g_rpFile << "'" << std::endl;
            goto end_function;
        }
    }
    // Otherwise, load the Rallypoint cluster if we have that
    else if(g_rpcFile != nullptr)
    {
        if(!loadRpCluster(g_rpcFile, g_rallypointCluster))
        {
            std::cerr << "could not load rallypoint cluster '" << g_rpcFile << "'" << std::endl;
            goto end_function;
        }
    }        

    // Create a user identity
    g_userIdentityJson = "{";
    if(!g_anonymous)
    {
        g_userIdentityJson += "\"userId\":\"";
        g_userIdentityJson += g_szUseConfigUserId;
        g_userIdentityJson += "\"";

        g_userIdentityJson += ",\"displayName\":\"";
        g_userIdentityJson += g_szUserConfigDisplayName;
        g_userIdentityJson += "\"";

        if(g_szNodeId[0] != 0)
        {
            g_userIdentityJson += ",\"nodeId\":\"";
            g_userIdentityJson += g_szNodeId;
            g_userIdentityJson += "\"";
        }
    }
    g_userIdentityJson += "}";

    // Build the information we'll need for our UI
    g_new_groups.clear();
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

    if(g_ncurses)
    {
        ncInit();
    }

    // Build our engine policy
    g_enginePolicyJson = g_enginePolicy.serialize();

    // We want to use a crypto engine
    if(g_fipsCryptoEnginePath != nullptr)
    {
        ConfigurationObjects::FipsCryptoSettings fces;

        fces.enabled = true;
        fces.path = g_fipsCryptoEnginePath;
        fces.debug = g_fipsCryptoDebug;

        if(engageSetFipsCrypto(fces.serialize().c_str()) != ENGAGE_RESULT_OK)
        {
            std::cerr << "engageSetFipsCrypto failed" << std::endl;
            goto end_function;
        }
    }

    if(engageIsCryptoFipsValidated() == 1)
    {
        g_miniLogger.i(TAG, "crypto engine is FIPS validated");
    }
    else
    {
        g_miniLogger.i(TAG, "crypto engine is NOT FIPS validated");
    }

    // Open a cert store if we have one
    if(!g_certStore.empty())
    {
        engageOpenCertStore(g_certStore.c_str(), g_certStorePwd.c_str());
    }

    // Initialize the library
    rc = engageInitialize(g_enginePolicyJson.c_str(), g_userIdentityJson.c_str(), nullptr);
    if(rc != ENGAGE_RESULT_OK)
    {
        std::cerr << "engageInitialize failed" << std::endl;
        goto end_function;
    }

    ok = true;

end_function:
    return ok;
}

void on_ENGAGE_LOG_MSG(int level, const char *tag, const char *message)
{
    if(g_ncurses)
    {
        ncLogThis(level, message);
    }
    else
    {
        puts(message);
    }
}

int main(int argc, const char * argv[])
{    
	// In Windows environments, when statically linking with the Engage library, we need
	// to manually do what the DLL loader would normally do for us.
	atexit(processDeinit);
	processInit();

    PlatformSetThreadName("ecMain");

    g_wq.setName("ecQ");
    g_wq.start();

    int rc;
    bool haveScript = false;
    bool continueWithCmdLine = true;

    // Check out the command line
    for(int x = 1; x < argc; x++)
    {
        if(strcmp(argv[x], "--version") == 0)
        {
            std::cout << RTS_VERSION << std::endl;
            return 0;
        }
        else if(strncmp(argv[x], "-jsonobjects", 12) == 0)
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
            g_missionFile = argv[x] + 9;
        }
        else if(strncmp(argv[x], "-mi:", 4) == 0)
        {
            g_missionFile = argv[x] + 4;
        }
        else if(strncmp(argv[x], "-loglevel:", 10) == 0)
        {
            engageSetLogLevel(atoi(argv[x] + 10));
        }        
        else if(strncmp(argv[x], "-ll:", 4) == 0)
        {
            engageSetLogLevel(atoi(argv[x] + 4));
        }
        else if(strncmp(argv[x], "-tx:", 4) == 0)
        {
            engageSetLogTagExtension(argv[x] + 4);
        }
        else if(strncmp(argv[x], "-cs:", 4) == 0)
        {
            g_certStore = argv[x] + 4;
        }
        else if(strncmp(argv[x], "-csp:", 5) == 0)
        {
            g_certStorePwd = argv[x] + 5;
        }
        else if(strncmp(argv[x], "-nic:", 5) == 0)
        {
            g_nicName = argv[x] + 5;
        }
        else if(strncmp(argv[x], "-ep:", 4) == 0)
        {
            g_epFile = argv[x] + 4;
        }
        else if(strncmp(argv[x], "-rp:", 4) == 0)
        {
            g_rpFile = argv[x] + 4;
        }
        else if(strncmp(argv[x], "-rpc:", 5) == 0)
        {
            g_rpcFile = argv[x] + 5;
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
        else if(strncmp(argv[x], "-adadms:", 8) == 0)
        {
            g_adadIntervalMs = atoi(argv[x] + 8);
        }
        else if(strncmp(argv[x], "-adadsr:", 8) == 0)
        {
            g_adadSampleRate = atoi(argv[x] + 8);
        }
        else if(strncmp(argv[x], "-adadch:", 8) == 0)
        {
            g_adadChannels = atoi(argv[x] + 8);
        }        
        else if(strncmp(argv[x], "-script:", 8) == 0)
        {
            g_pszScript = (argv[x] + 8);
        }
        else if(strcmp(argv[x], "-verbose") == 0)
        {
            g_verboseScript = true;
        }
        else if(strcmp(argv[x], "-quiet") == 0)
        {
            g_quiet = true;
        }        
        else if(strncmp(argv[x], "-pdf:", 5) == 0)
        {
            g_pszPresenceFile = (argv[x] + 5);
        }        
        else if(strncmp(argv[x], "-nid:", 5) == 0)
        {
            memset(g_szNodeId, 0, sizeof(g_szNodeId));
            strncpy_s(g_szNodeId, sizeof(g_szNodeId), argv[x] + 5, sizeof(g_szNodeId)-1);
        }        
        else if(strncmp(argv[x], "-ua:", 4) == 0)
        {
            memset(g_szUserConfigAlias, 0, sizeof(g_szUserConfigAlias));
            Utils::utf8cpy(g_szUserConfigAlias, (argv[x] + 4), sizeof(g_szUserConfigAlias));
        }
        else if(strncmp(argv[x], "-ui:", 4) == 0)
        {
            memset(g_szUseConfigUserId, 0, sizeof(g_szUseConfigUserId));
			Utils::utf8cpy(g_szUseConfigUserId, (argv[x] + 4), sizeof(g_szUseConfigUserId));
        }
        else if(strncmp(argv[x], "-ud:", 4) == 0)
        {
            memset(g_szUserConfigDisplayName, 0, sizeof(g_szUserConfigDisplayName));
			Utils::utf8cpy(g_szUserConfigDisplayName, (argv[x] + 4), sizeof(g_szUserConfigDisplayName));
        }
        else if(strncmp(argv[x], "-ut:", 4) == 0)
        {
            g_txPriority = atoi(argv[x] + 4);
        }
        else if(strncmp(argv[x], "-uf:", 4) == 0)
        {
            g_txFlags = atoi(argv[x] + 4);
        }
        else if(strncmp(argv[x], "-uas:", 5) == 0)
        {
            g_uiUserConfigAliasSpecializer = atoi(argv[x] + 5);
        }
        else if(strncmp(argv[x], "-putenv:", 8) == 0)
        {
            #if defined(WIN32)
                _putenv((char*)(argv[x] + 8));
            #else
                putenv((char*)(argv[x] + 8));
            #endif
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
        else if(strcmp(argv[x], "-getdeviceid") == 0)
        {
            // We don't want logging to mess up the output
            engageSetLogLevel(0);

            const char *deviceId = engageGetDeviceId();
            std::cout << (deviceId == nullptr ? "-" : deviceId) << std::endl;
            return 0;
        }        
        else if(strcmp(argv[x], "-acj") == 0)
        {
            g_autoCreateAndJoin = true;
        }
        else if(strncmp(argv[x], "-tf:", 4) == 0)
        {
            g_transmitFileAndExit = true;
            g_autoCreateAndJoin = true;
            g_txUriName = (argv[x] + 4);
        }
        else if(strncmp(argv[x], "-tfr:", 5) == 0)
        {
            g_txUriRepeatCount = atoi(argv[x] + 5);
        }
        else if(strcmp(argv[x], "-ncm") == 0)
        {
            #if defined(RTS_SUPPORT_NCURSES)
            {
                g_ncurses = true;
            }
            #else
            {
                std::cout << "ncurses is not supported on this platform" << std::endl;
                showUsage();
                goto end_function;
            }
            #endif
        }
        else if(strncmp(argv[x], "-netd:", 6) == 0)
        {
            g_netDeviceName = (argv[x] + 6);
        }
        else if(strcmp(argv[x], "-fcdebug") == 0)
        {
            g_fipsCryptoDebug = true;
        }        
        else if(strncmp(argv[x], "-fcpath:", 8) == 0)
        {
            g_fipsCryptoEnginePath = (argv[x] + 8);
        }
        else
        {
            std::cout << "unknown option '" << argv[x] << "'" << std::endl;
            showUsage();
            goto end_function;
        }
    }

    if(g_ncurses)
    {
        g_quiet = true;
        engageSetLoggingOutputOverride(on_ENGAGE_LOG_MSG);
    }

    if(!g_quiet)
    {
        std::cout << "---------------------------------------------------------------------------------" << std::endl;

        const char *buildType;

        #if defined(RTS_DEBUG_BUILD)
            buildType = "[DEBUG]";
        #else
            buildType = "[RELEASE]";
        #endif

        std::cout << "Engage-Cmd version " << RTS_VERSION << " " << buildType << " for " << RTS_TARGET_PLATFORM << std::endl;
        std::cout << "Copyright (c) 2019 Rally Tactical Systems, Inc." << std::endl;
        std::cout << "Build time: " << __DATE__ << " @ " << __TIME__ << std::endl;

        std::cout << "---------------------------------------------------------------------------------" << std::endl;
    }

    // We're just going to use a simple random number generator for this app
    srand((unsigned int)time(NULL));

    if(g_rpFile != nullptr && g_rpcFile != nullptr)
    {
        std::cerr << "'-rp' and '-rpc' are mutually exclusive" << std::endl;
        goto end_function;
    }

    // We need a mission file
    if(g_missionFile == nullptr || g_missionFile[0] == 0)
    {
        std::cerr << "no mission file specified" << std::endl;
        showUsage();
        goto end_function;
    }

    // See if we have a script
    if(g_pszScript != nullptr && g_pszScript[0])
    {
        if(strstr(g_pszScript, ".lua") == 0)
        {
            haveScript = loadScript(g_pszScript);
            if(!haveScript)
            {
                goto end_function;
            }
        }
    }

    // Register our callbacks
    if(!registerCallbacks())
    {
        std::cerr << "callback registration failed" << std::endl;
        goto end_function;
    }

    // Perform application-level initialization
    if(!doAppInitialization())
    {
        goto end_function;
    }

    // At this point we have our group list, show them
    if(!g_quiet)
    {
        showGroups();
    }

    // Fire up the dbus interface if any.  Do this here because the dbus goodies uses the Engage logger
    setupDbusSignalListener();

    // Start the Engine
    g_engineStarting = true;
    rc = engageStart();
    if(rc != ENGAGE_RESULT_OK)
    {
        std::cerr << "engageStart failed" << std::endl;
        goto end_function;
    }

    if(g_useadad)
    {
        registerADAD();
    }

    if(g_netDeviceName != nullptr)
    {
        registerNETD();
    }

    if(g_pszScript != nullptr && g_pszScript[0])
    {
        if(strstr(g_pszScript, ".lua") == 0)
        {
            if(haveScript)
            {
                if( !runScript() )
                {
                    continueWithCmdLine = false;
                }
            }
        }
        else
        {
            runLuaScript(g_pszScript);
            continueWithCmdLine = false;
        }
    }


    if(g_transmitFileAndExit)
    {
        while( !g_proceedWithExit )
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        continueWithCmdLine = false;
    }

    if(g_ncurses)
    {
        ncLoop();
        ncShutdown();
    }
    else
    {
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
    }

    unregisterADAD();
    unregisterNETD();

end_function:
    // Shutdown the dbus interface if any
    shutdownDbusSignalListener();

    // Stop the Engine
    g_shutdownEngineOnStopped = true;
    g_restartEngineOnStopped = false;
    g_reinitEngineOnStopped = false;
    engageStop();

    while( !g_engineStopped )
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Clean up
    g_new_groups.clear();
    delete g_singleRallypoint;
    delete g_rallypointCluster;

    g_wq.stop();

    return g_exitCode;
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

    else if(strcmp(buff, "restarteng") == 0)
    {
        doRestartEngine();
    }

    else if(strcmp(buff, "reiniteng") == 0)
    {
        doReinitEngine();
    }

    else if(strcmp(buff, "showaudio") == 0)
    {
        showAudioDevices();
    }

    else if(strcmp(buff, "shownic") == 0)
    {
        showNetworkDevices();
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
            filter = readTextFile(p);
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

    else if(strncmp(buff, "tm", 2) == 0)
    {
        char *p = (buff + 2);
        while( *p == ' ')
        {
            p++;
        }

        char *token;

        token = p;
        while( *p != 0 && *p != ' ')
        {
            p++;
        }

        if( *p == 0 )
        {
            std::cout << "******* ERROR *******: invalid syntax - hint 'tm index the_message_to_send'" << std::endl;
        }
        else
        {
            *p = 0;
            p++;

            if(strlen(p) > 0)
            {
                int idx = atoi(token);

                std::string jsonParams;

                ConfigurationObjects::BlobInfo bi;
                bi.payloadType = ConfigurationObjects::BlobInfo::PayloadType_t::bptAppTextUtf8;
                bi.rtpHeader.pt = 66;
                jsonParams = bi.serialize();

                doSendBlob(idx, (uint8_t*) p, strlen(p), jsonParams.c_str());
            }
            else
            {
                std::cout << "******* ERROR *******: invalid syntax - hint 'tm index the_message_to_send'" << std::endl;
            }
        }
    }

    else if(strncmp(buff, "txuri", 5) == 0)
    {
        char *p = (buff + 5);
        while( *p == ' ')
        {
            p++;
        }

        char *tokenForCount;

        tokenForCount = p;
        while( *p != 0 && *p != ' ')
        {
            p++;
        }

        *p = 0;
        p++;

        g_txUriRepeatCount = atoi(tokenForCount);
        g_txUriName = p;
    }

    else if(strcmp(buff, "`1") == 0)
    {
        devTest1();
    }
    else if(strcmp(buff, "`2") == 0)
    {
        devTest2();
    }
    else if(strcmp(buff, "`3") == 0)
    {
        devTest3();
    }
    else if(strcmp(buff, "`4") == 0)
    {
        devTest4();
    }
    else if(strcmp(buff, "`5") == 0)
    {
        devTest5();
    }
    else if(strcmp(buff, "`6") == 0)
    {
        devTest6();
    }
    else if(strcmp(buff, "`7") == 0)
    {
        devTest7();
    }
    else if(strcmp(buff, "`8") == 0)
    {
        devTest8();
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
    else if(strncmp(buff, "stu:", 4) == 0)
    {
        g_txUriName.assign(buff + 4);
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
    else if(buff[0] == 'y')
    {
        doMuteTx(buff[1] == 'a' ? -1 : atoi(buff + 1));
    }
    else if(buff[0] == 'k')
    {
        doUnmuteTx(buff[1] == 'a' ? -1 : atoi(buff + 1));
    }
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
    else if(buff[0] == 'g' && buff[1] == 's')
    {
        doGetStats(buff[2] == 'a' ? -1 : atoi(buff + 2));
    }
    else if(buff[0] == 'g' && buff[1] == 'h')
    {
        doGetHealth(buff[2] == 'a' ? -1 : atoi(buff + 2));
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

void showNetworkDevices()
{
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "Available network devices" << std::endl;

    try
    {
        const char *jsonString = engageGetNetworkInterfaceDevices();

        ConfigurationObjects::ListOfNetworkInterfaceDevice obj;

        if(!obj.deserialize(jsonString))
        {
            throw "";
        }

        for(std::vector<ConfigurationObjects::NetworkInterfaceDevice>::iterator itr = obj.list.begin();
            itr != obj.list.end();
            itr++)
        {
            std::cout << "name.................: " << itr->name << std::endl <<
                         "   friendlyName......: " << itr->friendlyName << std::endl <<
                         "   description.......: " << itr->description << std::endl <<
                         "   family............: " << itr->family << std::endl <<
                         "   address...........: " << itr->address << std::endl <<
                         "   available.........: " << itr->available << std::endl <<
                         "   isLoopback........: " << itr->isLoopback << std::endl <<
                         "   supportsMulticast.: " << itr->supportsMulticast << std::endl <<
                         "   hardwareAddress...: " << itr->hardwareAddress << std::endl <<
            std::endl;
        }
    }
    catch(...)
    {
        std::cout << "******* ERROR *******: exception while processing network interface devices list" << std::endl;
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
        groupConfig.alias = g_szUserConfigAlias;
    }
    else
    {
        groupConfig.alias.clear();
    }

    if(groupConfig.rallypoints.empty())
    {
        if(g_singleRallypoint != nullptr && g_singleRallypoint->host.port > 0)
        {
            groupConfig.rallypoints.push_back(*g_singleRallypoint);
        }
        else if(g_rallypointCluster != nullptr && !g_rallypointCluster->rallypoints.empty())
        {
            groupConfig.rallypointCluster = (*g_rallypointCluster);
        }
    }

    switch(g_multicastFailoverPolicy)
    {
        case 0:
            groupConfig.enableMulticastFailover = true;
            break;

        case 1:
            groupConfig.enableMulticastFailover = true;
            break;

        case 2:
            groupConfig.enableMulticastFailover = false;
            break;

        default:
            break;
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

    if(g_uiUserConfigAliasSpecializer > 0)
    {
        groupConfig.specializerAffinities.push_back(g_uiUserConfigAliasSpecializer);
    }

    // Serialize to a string
    std::string rc = groupConfig.serialize();

    return rc;
}

void doStartEngine()
{
    if(g_verboseScript) std::cout << "doStartEngine" << std::endl;
    g_engineStarting = true;
    engageStart();
}

void doStopEngine()
{
    if(g_verboseScript) std::cout << "doStopEngine" << std::endl;
    g_shutdownEngineOnStopped = false;
    g_restartEngineOnStopped = false;
    g_reinitEngineOnStopped = false;
    engageStop();
}

void doRestartEngine()
{
    if(g_verboseScript) std::cout << "doRestartEngine" << std::endl;
    g_shutdownEngineOnStopped = false;
    g_restartEngineOnStopped = true;
    g_reinitEngineOnStopped = false;
    engageStop();
}

void doReinitEngine()
{
    if(g_verboseScript) std::cout << "doReinitEngine" << std::endl;
    g_reinitInProgress = true;
    g_shutdownEngineOnStopped = false;
    g_restartEngineOnStopped = false;
    g_reinitEngineOnStopped = true;
    engageStop();
}

void doUpdatePresence(int index)
{
    if(g_pszPresenceFile == nullptr)
    {
        std::cerr << "no presence descriptor file (-pdf) provided" << std::endl;
        return;
    }

    if(g_verboseScript) std::cout << "doUpdatePresence: " << index << std::endl;
    std::string jsonText;
    jsonText = readTextFile(g_pszPresenceFile);

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

void doCreateUpTo(int index)
{
    if(g_verboseScript) std::cout << "doCreateUpTo: " << index << std::endl;
    std::string json;

    if(index == -1)
    {
        doCreate(index);
    }
    else
    {
        for(int x = 0; x < (int)g_new_groups.size() && x <= index; x++)
        {
            json = buildGroupCreationJson(x);
            engageCreateGroup(json.c_str());
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

void doJoinUpTo(int index)
{
    if(g_verboseScript) std::cout << "doJoinUpTo: " << index << std::endl;
    if(index == -1)
    {
        doJoin(index);
    }
    else
    {
        for(int x = 0; x < (int)g_new_groups.size() && x <= index; x++)
        {
            engageJoinGroup(g_new_groups[(size_t)x].id.c_str());
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
    params.flags = (uint16_t)g_txFlags;
    params.priority = (uint8_t)g_txPriority;
    //params.grantAudioUri = "./tx_on.wav";

    if(!g_anonymous)
    {
        params.alias = g_szUserConfigAlias;
    }

    params.subchannelTag = 0;
    params.includeNodeId = true;
    params.muted = false;
    params.txId = 0;

    if(g_uiUserConfigAliasSpecializer > 0)
    {
        params.receiverRxMuteForAliasSpecializer = true;
        params.aliasSpecializer = g_uiUserConfigAliasSpecializer;
    }

    params.audioUri.uri = g_txUriName;
    params.audioUri.repeatCount = g_txUriRepeatCount;

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

void doSetGroupAudioIds(int index, int inputId, int outputId)
{
    ConfigurationObjects::Group cfg;

    cfg.audio.inputId = inputId;
    cfg.audio.outputId = outputId;

    std::string jsonParams = cfg.serialize();

    if(g_verboseScript) std::cout << "doSetGroupAudioIds: " << index << std::endl;
    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            engageReconfigureGroup(g_new_groups[x].id.c_str(), jsonParams.c_str());
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            engageReconfigureGroup(g_new_groups[index].id.c_str(), jsonParams.c_str());
        }
        else
        {
            std::cerr << "invalid index" << std::endl;
        }
    }
}

void doGetStats(int index)
{
    if(g_verboseScript) std::cout << "doGetStats: " << index << std::endl;
    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            engageQueryGroupStats(g_new_groups[x].id.c_str());
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            engageQueryGroupStats(g_new_groups[index].id.c_str());
        }
        else
        {
            std::cerr << "invalid index" << std::endl;
        }
    }
}

void doGetHealth(int index)
{
    if(g_verboseScript) std::cout << "doGetHealth: " << index << std::endl;
    if(index == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            engageQueryGroupHealth(g_new_groups[x].id.c_str());
        }
    }
    else
    {
        if(index >= 0 && index < (int)g_new_groups.size())
        {
            engageQueryGroupHealth(g_new_groups[index].id.c_str());
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

void doGenerateMission(const char *passphrase, int channelCount, const char *rallypoint, const char *name, const char *fn)
{
    if(name == nullptr)
    {
        name = "";
    }

    if(fn == nullptr)
    {
        fn = "";
    }

    std::cout << "passphrase='" << passphrase << "'"
                << ", channelCount=" << channelCount << ""
                << ", rallypoint='" << rallypoint << "'"
                << ", name='" << name << "'"
                << ", fn='" << fn << "'"
                << std::endl;

    std::string ms;

    ms = engageGenerateMission(passphrase, channelCount, rallypoint, name);

    if(!ms.empty())
    {
        std::cout << ms << std::endl;

        if(fn == nullptr || fn[0] == 0)
        {
            std::cout << ms << std::endl;
        }
        else
        {
            FILE *fp;

            #ifndef WIN32
                fp = fopen(fn, "wt");
            #else
                if (fopen_s(&fp, fn, "wt") != 0)
                {
                    fp = nullptr;
                }
            #endif

            if(fp == nullptr)
            {
                std::cerr << "ERROR: cannot save to '" << fn << "'" << std::endl;
                return;
            }

            fputs(ms.c_str(), fp);

            fclose(fp);

            if(!g_quiet)
            {
                std::cout << "saved to '" << fn << "'" << std::endl;
            }
        }
    }
    else
    {
        std::cerr << "ERROR: no mission JSON produced" << std::endl;
    }
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

    doGenerateMission(passphrase.c_str(), channelCount, rallypoint.c_str(), name.c_str(), fn.c_str());
}

std::recursive_mutex                g_adadLock;

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
        std::lock_guard<std::recursive_mutex>   scopedLock(g_adadLock);

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
        std::lock_guard<std::recursive_mutex>   scopedLock(g_adadLock);

        _running = false;
        if(_threadHandle.joinable())
        {
            _threadHandle.join();
        }

        return ENGAGE_AUDIO_DEVICE_RESULT_OK;
    }

    int pause()
    {
        std::lock_guard<std::recursive_mutex>   scopedLock(g_adadLock);
        _paused = true;
        return ENGAGE_AUDIO_DEVICE_RESULT_OK;
    }

    int resume()
    {
        std::lock_guard<std::recursive_mutex>   scopedLock(g_adadLock);
        _paused = false;
        return ENGAGE_AUDIO_DEVICE_RESULT_OK;
    }

    int reset()
    {
        std::lock_guard<std::recursive_mutex>   scopedLock(g_adadLock);
        return ENGAGE_AUDIO_DEVICE_RESULT_OK;
    }

    int restart()
    {
        std::lock_guard<std::recursive_mutex>   scopedLock(g_adadLock);
        return ENGAGE_AUDIO_DEVICE_RESULT_OK;
    }

private:
    ADADInstance()
    {
        // Not to be used
    }

    void thread()
    {
        // One speaker buffer for everyone - we don't care if it gets trashed
        static int16_t STATIC_SPEAKER_BUFFER[8192];

        PlatformSetThreadName("adad");

        // The number of samples we produce/consume is g_adadSampleCount samples
        size_t  adadBufferSampleCount = (g_adadIntervalMs * (g_adadSampleRate / 1000) * g_adadChannels);

        g_miniLogger.d(TAG, "starting ADAD at %d sampling rate at %d ms intervals with %d channels for a buffer size of %zu samples per interval", g_adadSampleRate, g_adadIntervalMs, g_adadChannels, adadBufferSampleCount);

        int16_t *buffer = new int16_t[adadBufferSampleCount];
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

        uint64_t    now;
        uint64_t    started = 0;
        uint64_t    loops = 0;
        size_t      msToSleep;

        //uint64_t        rxCount = 0;

        if(_direction == ConfigurationObjects::AudioDeviceDescriptor::Direction_t::dirInput)
        {
            if(g_adadMicFile != nullptr)
            {
                #ifndef WIN32
                    fp = fopen(g_adadMicFile, "rb");
                #else
                    if (fopen_s(&fp, g_adadMicFile, "rb") != 0)
                    {
                        fp = nullptr;
                    }
                #endif

                if(fp == nullptr)
                {
                    g_miniLogger.f(TAG, "WARNING: specified adad microphone file '%s' cannot be opened", g_adadMicFile);
                    PlatformAbort();
                }
            }
        }
        else if(_direction == ConfigurationObjects::AudioDeviceDescriptor::Direction_t::dirOutput)
        {
            if(g_adadSpkFile != nullptr)
            {
                #ifndef WIN32
                    fp = fopen(g_adadSpkFile, "wb");
                #else
                    if (fopen_s(&fp, g_adadSpkFile, "wb") != 0)
                    {
                        fp = nullptr;
                    }
                #endif

                if(fp == nullptr)
                {
                    g_miniLogger.f(TAG, "WARNING: specified adad speaker file '%s' cannot be opened", g_adadSpkFile);
                    PlatformAbort();
                }
            }
        }

        memset(buffer, 0, adadBufferSampleCount * sizeof(int16_t));

        /*
        uint64_t lastLoopHit = 0;
        uint64_t totalDelta = 0;
        */

        while( _running )
        {
            msToSleep = g_adadIntervalMs;

            now = getTickMs();

            /*
            if(lastLoopHit != 0)
            {
                uint64_t delta = (now - lastLoopHit);
                totalDelta += delta;
                g_miniLogger.i(TAG, "loop hit, delta=%" PRIu64 ", avg=%f", delta, (double)(totalDelta / loops));
            }

            lastLoopHit = now;
            */

            if(started == 0)
            {
                started = now;
            }
            else
            {
                uint64_t avg = ((now - started) / loops);

                int64_t potentialTimeToSleep = 0;

                // Maybe catchup or slow down
                if(avg > (uint64_t)g_adadIntervalMs)
                {
                    potentialTimeToSleep = (g_adadIntervalMs - ((avg - g_adadIntervalMs) * 32));
                }
                else if( avg < (uint64_t)g_adadIntervalMs )
                {
                    potentialTimeToSleep = (g_adadIntervalMs + ((g_adadIntervalMs - avg) * 32));
                }
                else
                {
                    potentialTimeToSleep = (int64_t)g_adadIntervalMs;
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
                    //g_adadLock.lock();
                    rc = engageAudioDeviceReadBuffer(_deviceId, _instanceId, STATIC_SPEAKER_BUFFER, adadBufferSampleCount);
                    //g_adadLock.unlock();

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
                            /*
                            float total = 0.0;

                            for(int x = 0; x < rc; x++)
                            {
                                total += buffer[x];
                            }
                            */

                            //rxCount++;
                            /*
                            if(rxCount % 100 == 0)
                            {
                                std::cout << "ADADInstance readReadBuffer received " << rxCount << " buffers so far" << std::endl;
                            }
                            */
                            //std::cout << "ADADInstance readReadBuffer received " << rc << " samples with an average sample level of " << (total / rc) << std::endl;
                        }
                    }
                }
                else if(_direction == ConfigurationObjects::AudioDeviceDescriptor::Direction_t::dirInput)
                {
                    // Read audio from the file, if we can't or don't have one, just generate noise
                    if(fp != nullptr)
                    {
                        size_t amountRead = fread(buffer, 1, adadBufferSampleCount * 2, fp);
                        if(amountRead != adadBufferSampleCount * 2)
                        {
                            fseek(fp, 0, SEEK_SET);
                            amountRead = fread(buffer, 1, adadBufferSampleCount * 2, fp);
                            if(amountRead != adadBufferSampleCount * 2)
                            {
                                g_miniLogger.f(TAG, "******* ERROR *******: file read error from adad microphone file");
                                PlatformAbort();
                            }
                        }
                    }
                    else
                    {
                        for(size_t x = 0; x < adadBufferSampleCount; x++)
                        {
                            buffer[x] = (myRand32() % 32767);
                            if(myRand32() % 100 < 50)
                            {
                                buffer[x] *= -1;
                            }
                        }
                    }

                    engageAudioDeviceWriteBuffer(_deviceId, _instanceId, buffer, adadBufferSampleCount);
                }
                else
                {
                    assert(0);
                }
            }

            // Sleep for our device's "interval"
            std::this_thread::sleep_for(std::chrono::milliseconds(msToSleep));
        }

        if(fp != nullptr)
        {
            fclose(fp);
        }

        delete[] buffer;
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
    std::lock_guard<std::recursive_mutex>   scopedLock(g_adadLock);
    int rc = ENGAGE_AUDIO_DEVICE_RESULT_OK;

    ADADInstance *instance = nullptr;

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
                    instance->stop();
                    delete instance;
                    g_audioDeviceInstances.erase(itr);
                    break;

                // ... start an instance of a device identified by "instanceId"
                case EngageAudioDeviceCtlOp_t::eadStart:
                    instance->start();
                    break;

                // ... stop an instance of a device identified by "instanceId"
                case EngageAudioDeviceCtlOp_t::eadStop:
                    instance->stop();
                    break;

                // ... pause an instance of a device identified by "instanceId"
                case EngageAudioDeviceCtlOp_t::eadPause:
                    instance->pause();
                    break;

                // ... resume an instance of a device identified by "instanceId"
                case EngageAudioDeviceCtlOp_t::eadResume:
                    instance->resume();
                    break;

                // ... reset an instance of a device identified by "instanceId"
                case EngageAudioDeviceCtlOp_t::eadReset:
                    instance->reset();
                    break;

                // ... restart an instance of a device identified by "instanceId"
                case EngageAudioDeviceCtlOp_t::eadRestart:
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
            g_miniLogger.e(TAG, "MyAudioDeviceCallback for an unknown instance id of %d", instanceId);
            rc = ENGAGE_AUDIO_DEVICE_INVALID_INSTANCE_ID;
        }
    }

    return rc;
}

void registerADAD()
{
    std::lock_guard<std::recursive_mutex>   scopedLock(g_adadLock);

    // Setup the speaker
    {
        ConfigurationObjects::AudioDeviceDescriptor speakerDevice;
        speakerDevice.direction = ConfigurationObjects::AudioDeviceDescriptor::Direction_t::dirOutput;
        speakerDevice.deviceId = 0;
        speakerDevice.samplingRate = g_adadSampleRate;
        speakerDevice.channels = g_adadChannels;
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
        microphoneDevice.samplingRate = g_adadSampleRate;
        microphoneDevice.channels = g_adadChannels;
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
    std::lock_guard<std::recursive_mutex>   scopedLock(g_adadLock);

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
    std::lock_guard<std::recursive_mutex>   scopedLock(g_adadLock);

    for(std::map<int16_t, ADADInstance*>::iterator itr = g_audioDeviceInstances.begin();
        itr != g_audioDeviceInstances.end();
        itr++)
    {
        itr->second->stop();
        delete itr->second;
    }

    g_audioDeviceInstances.clear();
}

std::deque<std::vector<uint8_t>>        _myNetworkDeviceTxQueue;
std::mutex                              _myNetworkDeviceTxQueueLock;

int MyNetworkDeviceCallback(int16_t deviceId, EngageNetworkDeviceCtlOp_t op, const char *jsonMetaData, uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5)
{
    int rc = 0;

    switch( op )
    {
        case EngageNetworkDeviceCtlOp_t::enetStart:
            {
                g_miniLogger.i(TAG, "MyNetworkDeviceCallback - enetStart");

                _myNetworkDeviceTxQueueLock.lock();
                {
                    _myNetworkDeviceTxQueue.clear();
                }
                _myNetworkDeviceTxQueueLock.unlock();

                break;
            }

        case EngageNetworkDeviceCtlOp_t::enetStop:
            {
                g_miniLogger.i(TAG, "MyNetworkDeviceCallback - enetStop");

                _myNetworkDeviceTxQueueLock.lock();
                {
                    _myNetworkDeviceTxQueue.clear();
                }
                _myNetworkDeviceTxQueueLock.unlock();
                
                break;
            }

        case EngageNetworkDeviceCtlOp_t::enetSend:
            {
                const uint8_t *buff = (const uint8_t *)p1;
                size_t buffSz = (size_t)p2;
                //uint32_t timeoutMs = (uint32_t)p3;

                //g_miniLogger.i(TAG, "MyNetworkDeviceCallback - enetSend %zu bytes", buffSz);

                if(buff != nullptr && buffSz > 0)
                {
                    std::vector<uint8_t> packet;
                    std::copy(buff, buff + buffSz, std::back_inserter(packet));
                    
                    _myNetworkDeviceTxQueueLock.lock();
                    {
                        _myNetworkDeviceTxQueue.push_back(packet);
                        if(_myNetworkDeviceTxQueue.size() > 10)
                        {
                            _myNetworkDeviceTxQueue.pop_front();
                        }
                    }
                    _myNetworkDeviceTxQueueLock.unlock();

                    rc = (int)buffSz;
                }
                break;
            }

        case EngageNetworkDeviceCtlOp_t::enetRecv:
            {
                uint8_t *buff = (uint8_t *)p1;
                size_t maxBuffSz = (size_t)p2;
                uint32_t timeoutMs = (uint32_t)p3;
                
                uint64_t tsStarted = getTickMs();

                //g_miniLogger.i(TAG, "MyNetworkDeviceCallback - enetRecv");

                while( true )
                {
                    _myNetworkDeviceTxQueueLock.lock();
                    {
                        if(!_myNetworkDeviceTxQueue.empty())
                        {
                            std::vector<uint8_t> packet = _myNetworkDeviceTxQueue.front();
                            _myNetworkDeviceTxQueue.pop_front();

                            size_t sizeToCopy;

                            if(packet.size() <= maxBuffSz)
                            {
                                sizeToCopy = packet.size();
                            }
                            else
                            {
                                sizeToCopy = maxBuffSz;
                            }

                            memcpy(buff, &packet[0], sizeToCopy);
                            rc = (int)sizeToCopy;
                        }
                    }
                    _myNetworkDeviceTxQueueLock.unlock();

                    if(rc == 0)
                    {
                        if(getTickMs() - tsStarted >= (uint64_t)timeoutMs)
                        {
                            rc = 0;
                            break;
                        }

                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    }
                    else
                    {
                        break;
                    }
                }                

                break;
            }
    }

    return rc;
}

void registerNETD()
{
    ConfigurationObjects::NetworkDeviceDescriptor netDevice;
    netDevice.deviceId = 0;
    netDevice.name = g_netDeviceName;
    netDevice.manufacturer = "";
    std::string json = netDevice.serialize();
    g_networkDeviceId = engageNetworkDeviceRegister(json.c_str(), MyNetworkDeviceCallback);
    if(g_networkDeviceId < 0)
    {
        g_networkDeviceId = 0;
    }
}

void unregisterNETD()
{
    if(g_networkDeviceId > 0)
    {
        engageNetworkDeviceUnregister(g_networkDeviceId);
        g_networkDeviceId = 0;
    }
}

void showUsage()
{
    std::cout << "usage: engage-cmd -ep:<engine_policy_file> -mission:<mission_file> [options]" << std::endl << std::endl
              << "\twhere [options] are:" << std::endl << std::endl
              << "\t-quiet ................................ quiet - minimal output" << std::endl
              << "\t-ll:<log_level> ....................... specify the Engage Engine logging level" << std::endl
              << "\t-cs:<certificate_store_file> .......... specify a certificate store file" << std::endl
              << "\t-csp:<certificate_store_pwd> .......... password (if any) for the certificate store in hexstring format" << std::endl
              << "\t-rp:<rallypoint_file> ................. specify a rallypoint configuration" << std::endl
              << "\t-nic:<nic_name> ....................... specify the name of the default nic" << std::endl
              << "\t-nid:<node_id> ........................ set the node id" << std::endl
              << "\t-pdf:<presence_descriptor_file> ....... file containing json to be sent via engageUpdatePresenceDescriptor" << std::endl
              << "\t-ua:<user_alias> ...................... set the user's alias" << std::endl
              << "\t-ui:<user_id> ......................... set the user's id" << std::endl
              << "\t-ud:<user_display> .................... set the user's display name" << std::endl
              << "\t-uas:<user_alias_specializer> ......... set the user's alias specializer (16-bit unsigned integer)" << std::endl
              << "\t-ut:<user_tx_priority> ................ set audio transmit priority" << std::endl
              << "\t-uf:<user_tx_flags> ................... set audio transmit flags" << std::endl
              << "\t-putenv:<key>=<value> ................. set an environment variable" << std::endl
              << "\t-verbose .............................. enable verbose script mode" << std::endl
              << "\t-anon ................................. operate in anonymous identity mode" << std::endl
              << "\t-useadad .............................. use an application-defined audio device" << std::endl
              << "\t-adadmic:<file_name>................... name of microphone file to use for the ADAD" << std::endl
              << "\t-adadspk:<file_name>................... name of speaker file to use for the ADAD" << std::endl
              << "\t-adadms:<milliseconds>................. interval milliseconds to use for the ADAD (default 20)" << std::endl
              << "\t-adadsr:<sample_rate>.................. sample rate to use for the ADAD (default 16,000)" << std::endl
              << "\t-adadch:<channels>..................... channel count (1 or 2) to use for the ADAD (default 2)" << std::endl
              << "\t-jsonobjects .......................... display json object configuration" << std::endl
              << "\t-sn:<speaker_name> .................... set name of audio device to use for speaker" << std::endl
              << "\t-mn:<microphone_name> ................. set name of audio device to use for microphone" << std::endl
              << "\t-dbusconn:<dbus_connection_name> ...... set the DBUS connection name" << std::endl
              << "\t-dbussig:<dbus_signal_name> ........... set the DBUS signal name" << std::endl
              << "\t-getdeviceid .......................... display the device ID and exit" << std::endl
              << "\t-acj .................................. automatically create and join all groups" << std::endl              
              << "\t-tf:<file_name> ....................... transmit an audio file and exit" << std::endl
              << "\t-tfr:<count> .......................... repeat count for the transmitted audio file" << std::endl
              << "\t-netd:<network_device_name>............ create and register a loopback network device name" << std::endl
              << "\t-fcpath:<path_to_fips_engine>.......... sets the path for the FIPS140-2 crypto module" << std::endl
              << "\t-fcdebug............................... enables debugging (if available) in the FIPS140-2 crypto module" << std::endl              
              ;
}

void showHelp()
{
    std::cout << "q.............quit" << std::endl;
    std::cout << "!<command>....execute a shell command" << std::endl;
    std::cout << "-.............draw a line" << std::endl;
    std::cout << std::endl;
    
    std::cout << "starteng......start engine" << std::endl;
    std::cout << "stopeng.......stop engine" << std::endl;
    std::cout << "restarteng....restart engine" << std::endl;
    std::cout << "reiniteng....reinitialize engine" << std::endl;
    std::cout << "showaudio.....show audio devices" << std::endl;
    std::cout << "shownic.......show network interface devices" << std::endl;
    std::cout << std::endl;

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
    std::cout << "y<N>..........mute tx on group index N" << std::endl;
    std::cout << "ya............mute tx on all groups" << std::endl;
    std::cout << "k<N>..........unmute tx on group index N" << std::endl;
    std::cout << "ka............unmute tx on all groups" << std::endl;

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
    for(std::vector<ConfigurationObjects::PresenceDescriptorGroupItem>::iterator itr = pd.groupAliases.begin();
        itr != pd.groupAliases.end();
        itr++)
    {
        std::cout << "     " << itr->groupId << ", alias=" << itr->alias << ", status=" << itr->status << std::endl;
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
    bool rc = false;

    try
    {
        std::string jsonText;
        jsonText = readTextFile(pszFn);

        nlohmann::json j = nlohmann::json::parse(jsonText);
        ConfigurationObjects::from_json(j, *pPolicy);
        rc = true;
    }
    catch (...)
    {
    }

    return rc;
}

bool loadMission(const char *pszFn, ConfigurationObjects::Mission *pMission, 
                                    ConfigurationObjects::Rallypoint *pSingleRp, 
                                    ConfigurationObjects::RallypointCluster *pRpCluster,
                                    int *pMulticastFailoverPolicy)
{
    bool rc = false;

    try
    {
        std::string jsonText;

        // If the mission file name starts with '@' we will treat the rest fo the string as parameters for
        // generating a mission.  The string is formatted as follows:
        //
        // @[passphrase][number_of_channels][groupOptions]
        if(pszFn[0] == '@')
        {
            char buff[1024];
            static const char *GMSEPS = "[]";

            strcpy_s(buff, sizeof(buff), pszFn + 1);

            char *ctx = nullptr;
            char *tok;

            char *passphrase = nullptr;
            char *channelCount = nullptr;
            char *groupOpts = nullptr;

            tok = strtok_s(buff, GMSEPS, &ctx);
            passphrase = tok;

            tok = strtok_s(nullptr, GMSEPS, &ctx);
            channelCount = tok;

            tok = strtok_s(nullptr, GMSEPS, &ctx);
            groupOpts = tok;

            // Generate the template
            std::string generatedTemplate = engageGenerateMission(passphrase, atoi(channelCount), "", "");

            // We will assume that this mission is to be used for load testing
            ConfigurationObjects::Mission m;
            m.deserialize(generatedTemplate.c_str());

            if(groupOpts != nullptr)
            {
                for(std::vector<ConfigurationObjects::Group>::iterator itr = m.groups.begin();
                    itr != m.groups.end();
                    itr++)
                {
                    if(strstr(groupOpts, "nocrypto") != 0)
                    {
                        itr->cryptoPassword.clear();
                    }

                    if(strstr(groupOpts, "nomulticastfailover") != 0)
                    {
                        itr->enableMulticastFailover = false;
                    }

                    if(strstr(groupOpts, "notimeline") != 0)
                    {
                        itr->timeline.enabled = false;
                    }
                }
            }

            jsonText = m.serialize(3);
        }
        else
        {
            jsonText = readTextFile(pszFn);
        }

        nlohmann::json j = nlohmann::json::parse(jsonText);
        ConfigurationObjects::from_json(j, *pMission);
        rc = true;

        // We may have a global rallypoint defined in this JSON
        try
        {
            nlohmann::json rp = j.at("rallypoint");
            std::string address = rp.at("address");
            int port = rp.at("port");

            pSingleRp->host.address = address;
            pSingleRp->host.port = port;
            pSingleRp->certificate = "@default_cert.pem";
            pSingleRp->certificateKey = "@default_key.pem";
        }
        catch(...)
        {
            pSingleRp->clear();
        }

        try
        {
            *pMulticastFailoverPolicy = j.at("multicastFailoverPolicy");
        }
        catch(...)
        {
            *pMulticastFailoverPolicy = -1;
        }

        // We may have a global rallypoint cluster defined in this JSON
        try
        {
            (*pRpCluster) = j.at("rallypointCluster");
        }
        catch(...)
        {
            pRpCluster->clear();
        }
    }
    catch (...)
    {
    }

    return rc;
}

bool loadSingleRp(const char *pszFn, ConfigurationObjects::Rallypoint *pRp)
{
    bool rc = false;

    try
    {
        std::string jsonText;
        jsonText = readTextFile(pszFn);

        nlohmann::json j = nlohmann::json::parse(jsonText);
        ConfigurationObjects::from_json(j, *pRp);
        rc = true;
    }
    catch (...)
    {
    }
    return rc;
}

bool loadRpCluster(const char *pszFn, ConfigurationObjects::RallypointCluster *pCluster)
{
    bool rc = false;

    try
    {
        std::string jsonText;
        jsonText = readTextFile(pszFn);

        nlohmann::json j = nlohmann::json::parse(jsonText);
        ConfigurationObjects::from_json(j, *pCluster);
        rc = true;
    }
    catch (...)
    {
    }
    return rc;
}

#define MAYBE_UNUSED(_x) (void)(_x)

#define EEJ_LAMBDA_BEGIN() \
    std::string eej = (eventExtraJson != nullptr ? eventExtraJson : ""); \
    g_wq.submit(([eej]() \
    { \
        MAYBE_UNUSED(eej);

#define EEJ_S1_LAMBDA_BEGIN(_s1) \
    std::string s1 = (_s1 != nullptr ? _s1 : ""); \
    std::string eej = (eventExtraJson != nullptr ? eventExtraJson : ""); \
    g_wq.submit(([s1, eej]() \
    { \
        MAYBE_UNUSED(s1); \
        MAYBE_UNUSED(eej);

#define BASIC_ID_LAMBDA_BEGIN() \
    std::string id = pId; \
    std::string eej = (eventExtraJson != nullptr ? eventExtraJson : ""); \
    g_wq.submit(([id, eej]() \
    { \
        MAYBE_UNUSED(id); \
        MAYBE_UNUSED(eej);

#define LAMBDA_BEGIN_S1(_s1) \
    std::string id = pId; \
    std::string eej = (eventExtraJson != nullptr ? eventExtraJson : ""); \
    std::string s1 = (_s1 != nullptr ? _s1 : ""); \
    g_wq.submit(([id, eej, s1]() \
    { \
        MAYBE_UNUSED(id); \
        MAYBE_UNUSED(eej); \
        MAYBE_UNUSED(s1);

#define LAMBDA_BEGIN_N1(_n1) \
    std::string id = pId; \
    std::string eej = (eventExtraJson != nullptr ? eventExtraJson : ""); \
    g_wq.submit(([id, eej, _n1]() \
    { \
        MAYBE_UNUSED(id); \
        MAYBE_UNUSED(eej); \
        MAYBE_UNUSED(_n1);

#define LAMBDA_BEGIN_N2(_n1, _n2) \
    std::string id = pId; \
    std::string eej = (eventExtraJson != nullptr ? eventExtraJson : ""); \
    g_wq.submit(([id, eej, _n1, _n2]() \
    { \
        MAYBE_UNUSED(id); \
        MAYBE_UNUSED(eej); \
        MAYBE_UNUSED(_n1); \
        MAYBE_UNUSED(_n2);

#define LAMBDA_END()    }));


void checkIfExitNeeded()
{
    if(g_transmitFileAndExit)
    {
        bool shouldExit = true;

        for(std::vector<GroupInfo>::iterator itr = g_new_groups.begin();
            itr != g_new_groups.end();
            itr++)
        {
            if(itr->type == 1)
            {
                if(itr->createFailed || itr->connectFailed || itr->joinFailed || itr->txFailed || itr->txUsurped)
                {
                    g_exitCode = 1;
                    break;
                }

                if(!itr->txEnded)
                {
                    shouldExit = false;
                    break;
                }
            }
        }

        if(shouldExit)
        {
            g_proceedWithExit = true;
        }
    }
}

void on_ENGAGE_ENGINE_STARTED(const char *eventExtraJson)
{
    EEJ_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_ENGINE_STARTED");
        g_engineStarted = true;
        g_engineStarting = false;
        g_engineStopped = false;
        g_reinitInProgress = false;

        if(g_autoCreateAndJoin)
        {
            doCreate(-1);
        }

        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_ENGINE_STOPPED(const char *eventExtraJson)
{
    EEJ_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_ENGINE_STOPPED");
        g_engineStarted = false;
        g_engineStarting = false;
        g_engineStopped = true;

        if(g_shutdownEngineOnStopped)
        {
            g_miniLogger.d(TAG, "on_ENGAGE_ENGINE_STOPPED: shutting down engine");
            engageShutdown();
        }
        else if(g_restartEngineOnStopped)
        {
            g_miniLogger.d(TAG, "on_ENGAGE_ENGINE_STOPPED: restarting engine");
            g_restartEngineOnStopped = false;
            g_engineStarting = true;
            engageStart();
        }
        else if(g_reinitEngineOnStopped)
        {
            g_miniLogger.d(TAG, "---------------------------------------------------- on_ENGAGE_ENGINE_STOPPED: reinitializing engine");

            engageShutdown();

            g_miniLogger.d(TAG, "---------------------------------------------------- on_ENGAGE_ENGINE_STOPPED: engine has shutdown");

            g_reinitEngineOnStopped = false;
            if(doAppInitialization())
            {
                g_engineStarting = true;
                engageStart();
            }
        }
        else
        {
            g_miniLogger.d(TAG, "on_ENGAGE_ENGINE_STOPPED: no special action to be taken");
        }

        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_ENGINE_AUDIO_DEVICES_REFRESHED(const char *eventExtraJson)
{
    EEJ_LAMBDA_BEGIN()
    {
        g_miniLogger.e(TAG, "on_ENGAGE_ENGINE_AUDIO_DEVICES_REFRESHED new info=%s", eej.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_RP_PAUSING_CONNECTION_ATTEMPT(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_RP_PAUSING_CONNECTION_ATTEMPT %s, %s", id.c_str(), eej.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_RP_CONNECTING(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_RP_CONNECTING %s, %s", id.c_str(), eej.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_RP_CONNECTED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_RP_CONNECTED %s, %s", id.c_str(), eej.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_RP_DISCONNECTED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_RP_DISCONNECTED %s, %s", id.c_str(), eej.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_RP_ROUNDTRIP_REPORT(const char *pId, uint32_t rtMs, uint32_t rtRating, const char *eventExtraJson)
{
    LAMBDA_BEGIN_N2(rtMs, rtRating)
    {
        g_miniLogger.d(TAG, "on_ENGAGE_RP_ROUNDTRIP_REPORT %s, ms=%u, rating=%u, %s", id.c_str(), rtMs, rtRating, eej.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_BRIDGE_CREATED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_BRIDGE_CREATED %s - %s", id.c_str(), eej.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_BRIDGE_CREATE_FAILED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.e(TAG, "on_ENGAGE_BRIDGE_CREATE_FAILED %s - %s", id.c_str(), eej.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_BRIDGE_DELETED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_BRIDGE_DELETED %s", id.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_CREATED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_CREATED %s - %s", id.c_str(), eej.c_str());
        int index = indexOfGroupId(id.c_str());
        if(index >= 0)
        {
            g_new_groups[index].created = true;
            g_new_groups[index].createFailed = false;
            g_new_groups[index].deleted = false;

            if(g_autoCreateAndJoin)
            {
                doJoin(-1);
            }
        }

        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_CREATE_FAILED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.e(TAG, "on_ENGAGE_GROUP_CREATE_FAILED %s - %s", id.c_str(), eej.c_str());
        int index = indexOfGroupId(id.c_str());
        if(index >= 0)
        {
            g_new_groups[index].created = false;
            g_new_groups[index].createFailed = true;
        }

        checkIfExitNeeded();
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_DELETED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_DELETED %s", id.c_str());
        int index = indexOfGroupId(id.c_str());
        if(index >= 0)
        {
            g_new_groups[index].deleted = true;
            if(g_new_groups[index].isDynamic)
            {
                g_new_groups.erase(g_new_groups.begin()+index);
            }
        }

        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_CONNECTED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_CONNECTED %s, %s", id.c_str(), eej.c_str());
        int index = indexOfGroupId(id.c_str());
        if(index >= 0)
        {
            g_new_groups[index].connected = true;
            g_new_groups[index].connectFailed = false;
            g_new_groups[index].notConnected = false;

            if(g_transmitFileAndExit)
            {
                if(g_new_groups[index].type == 1)
                {
                    doBeginTx(index);
                }
            }
        }

        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_CONNECT_FAILED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.e(TAG, "on_ENGAGE_GROUP_CONNECT_FAILED %s, %s", id.c_str(), eej.c_str());
        int index = indexOfGroupId(id.c_str());
        if(index >= 0)
        {
            g_new_groups[index].connected = false;
            g_new_groups[index].connectFailed = true;
            g_new_groups[index].notConnected = true;
        }

        checkIfExitNeeded();
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_DISCONNECTED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_DISCONNECTED %s, %s", id.c_str(), eej.c_str());
        int index = indexOfGroupId(id.c_str());
        if(index >= 0)
        {
            g_new_groups[index].connected = false;
            g_new_groups[index].connectFailed = false;
            g_new_groups[index].notConnected = true;
        }

        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_JOINED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_JOINED %s", id.c_str());
        int index = indexOfGroupId(id.c_str());
        if(index >= 0)
        {
            g_new_groups[index].joined = true;
            g_new_groups[index].joinFailed = false;
            g_new_groups[index].notJoined = false;
        }

        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_JOIN_FAILED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.e(TAG, "on_ENGAGE_GROUP_JOIN_FAILED %s", id.c_str());
        int index = indexOfGroupId(id.c_str());
        if(index >= 0)
        {
            g_new_groups[index].joined = false;
            g_new_groups[index].joinFailed = true;
            g_new_groups[index].notJoined = true;
        }

        checkIfExitNeeded();
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_LEFT(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_LEFT %s", id.c_str());
        int index = indexOfGroupId(id.c_str());
        if(index >= 0)
        {
            g_new_groups[index].joined = false;
            g_new_groups[index].joinFailed = false;
            g_new_groups[index].notJoined = true;
        }

        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_MEMBER_COUNT_CHANGED(const char *pId, size_t newCount, const char *eventExtraJson)
{
    LAMBDA_BEGIN_N1(newCount)
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_MEMBER_COUNT_CHANGED %s, %zu", id.c_str(), newCount);
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_RX_STARTED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_RX_STARTED %s", id.c_str());
        int index = indexOfGroupId(id.c_str());
        if(index >= 0)
        {
            g_new_groups[index].rxStarted = true;
            g_new_groups[index].rxEnded = false;
        }

        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_RX_ENDED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_RX_ENDED %s", id.c_str());
        int index = indexOfGroupId(id.c_str());
        if(index >= 0)
        {
            g_new_groups[index].rxStarted = false;
            g_new_groups[index].rxEnded = true;
        }

        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_RX_MUTED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_RX_MUTED %s%s%s", id.c_str(), (eej.empty() ? "" : " - "), (eej.empty() ? "" : eej.c_str()));
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_RX_UNMUTED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_RX_UNMUTED %s%s%s", id.c_str(), (eej.empty() ? "" : " - "), (eej.empty() ? "" : eej.c_str()));
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_RX_SPEAKERS_CHANGED(const char *pId, const char *groupTalkerJson, const char *eventExtraJson)
{
    LAMBDA_BEGIN_S1(groupTalkerJson)
    {
        std::string listOfNames;
        ConfigurationObjects::GroupTalkers gt;

        int index = indexOfGroupId(id.c_str());

        if(index >= 0)
        {
            g_new_groups[index].talkers.clear();
        }

        if(!gt.deserialize(s1.c_str()))
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
                char tmp[128];

                info = itr->alias;
                info.append(" nodeId='");
                info.append(itr->nodeId);
                info.append("'");
                snprintf(tmp, sizeof(tmp), " (f=%u, p=%d, txId=%u, dup=%d, spc=%u, rxMute=%d)", 
                                itr->rxFlags, 
                                itr->txPriority, 
                                itr->txId, 
                                itr->duplicateCount, 
                                itr->aliasSpecializer,
                                itr->rxMuted);

                info.append(tmp);

                if(index >= 0)
                {
                    g_new_groups[index].talkers.push_back(info);
                }

                listOfNames += info;
            }
        }

        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_RX_SPEAKERS_CHANGED %s, [%s]", id.c_str(), listOfNames.c_str());
    
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_TX_STARTED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_TX_STARTED %s, x=%s", id.c_str(), eej.c_str());
        int index = indexOfGroupId(id.c_str());
        if(index >= 0)
        {
            g_new_groups[index].txStarted = true;
            g_new_groups[index].txEnded = false;
            g_new_groups[index].txFailed = false;
            g_new_groups[index].txUsurped = false;
        }

        // Unmute TX for good measure because the policy may have dictated muteTxOnTx
        engageUnmuteGroupTx(id.c_str());

        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_TX_ENDED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        ConfigurationObjects::GroupTxDetail gtd;

        try
        {
            gtd = nlohmann::json::parse(eej.c_str())["groupTxDetail"];
        }
        catch(...)
        {
            gtd.status = ConfigurationObjects::GroupTxDetail::TxStatus_t::txsTxEnded;
        }

        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_TX_ENDED %s, x=%s", id.c_str(), eej.c_str());
        int index = indexOfGroupId(id.c_str());
        if(index >= 0)
        {
            g_new_groups[index].txStarted = false;
            g_new_groups[index].txEnded = true;
            g_new_groups[index].txFailed = (gtd.status != ConfigurationObjects::GroupTxDetail::TxStatus_t::txsTxEnded);
            //g_new_groups[index].txUsurped = (gtd.status == ConfigurationObjects::GroupTxDetail::TxStatus_t::txsPriorityTooLow);
        }

        checkIfExitNeeded();
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_TX_FAILED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.e(TAG, "on_ENGAGE_GROUP_TX_FAILED %s, x=%s", id.c_str(), eej.c_str());
        int index = indexOfGroupId(id.c_str());
        if(index >= 0)
        {
            g_new_groups[index].txStarted = false;
            g_new_groups[index].txEnded = false;
            g_new_groups[index].txFailed = true;
        }

        checkIfExitNeeded();
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_TX_USURPED_BY_PRIORITY(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.w(TAG, "on_ENGAGE_GROUP_TX_USURPED_BY_PRIORITY %s, x=%s", id.c_str(), eej.c_str());
        int index = indexOfGroupId(id.c_str());
        if(index >= 0)
        {
            g_new_groups[index].txStarted = false;
            g_new_groups[index].txEnded = true;
            g_new_groups[index].txFailed = false;
            g_new_groups[index].txUsurped = true;
        }

        checkIfExitNeeded();
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_MAX_TX_TIME_EXCEEDED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.w(TAG, "on_ENGAGE_GROUP_MAX_TX_TIME_EXCEEDED %s, x=%s", id.c_str(), eej.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_TX_MUTED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_TX_MUTED %s", id.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_TX_UNMUTED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_TX_UNMUTED %s", id.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
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
    LAMBDA_BEGIN_S1(pszNodeJson)
    {
        ConfigurationObjects::PresenceDescriptor pd;
        if(pd.deserialize(s1.c_str()))
        {
            g_miniLogger.d(TAG, "on_ENGAGE_GROUP_NODE_DISCOVERED %s, %s, %s: ", id.c_str(), pd.identity.nodeId.c_str(), s1.c_str());
            addOrUpdatePd(pd);
        }
        else
        {
            g_miniLogger.d(TAG, "on_ENGAGE_GROUP_NODE_DISCOVERED %s, COULD NOT PARSE JSON", id.c_str());
        }
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_NODE_REDISCOVERED(const char *pId, const char *pszNodeJson, const char *eventExtraJson)
{
    LAMBDA_BEGIN_S1(pszNodeJson)
    {
        ConfigurationObjects::PresenceDescriptor pd;
        if(pd.deserialize(s1.c_str()))
        {
            g_miniLogger.d(TAG, "on_ENGAGE_GROUP_NODE_REDISCOVERED %s, %s, %s", id.c_str(), pd.identity.nodeId.c_str(), s1.c_str());
            addOrUpdatePd(pd);
        }
        else
        {
            g_miniLogger.w(TAG, "on_ENGAGE_GROUP_NODE_REDISCOVERED %s, COULD NOT PARSE JSON", id.c_str());
        }
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_NODE_UNDISCOVERED(const char *pId, const char *pszNodeJson, const char *eventExtraJson)
{
    LAMBDA_BEGIN_S1(pszNodeJson)
    {
        ConfigurationObjects::PresenceDescriptor pd;
        if(pd.deserialize(s1.c_str()))
        {
            g_miniLogger.d(TAG, "on_ENGAGE_GROUP_NODE_UNDISCOVERED: %s, %s, %s", id.c_str(), pd.identity.nodeId.c_str(), s1.c_str());
            removePd(pd);
        }
        else
        {
            g_miniLogger.w(TAG, "on_ENGAGE_GROUP_NODE_UNDISCOVERED %s, COULD NOT PARSE JSON", id.c_str());
        }
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_ASSET_DISCOVERED(const char *pId, const char *pszGroupJson, const char *eventExtraJson)
{
    LAMBDA_BEGIN_S1(pszGroupJson)
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_ASSET_DISCOVERED %s, %s: ", id.c_str(), s1.c_str());

        int index = indexOfGroupId(id.c_str());
        if(index < 0)
        {
            ConfigurationObjects::Group grp;

            grp.deserialize(s1.c_str());
            GroupInfo gi;

            gi.id = id.c_str();
            gi.name = grp.name;
            gi.type = 1;
            gi.isEncrypted = (!grp.cryptoPassword.empty());
            gi.allowsFullDuplex = false;
            gi.created = false;
            gi.createFailed = false;
            gi.deleted = false;
            gi.isDynamic = true;
            gi.jsonConfiguration = s1.c_str();

            g_new_groups.push_back(gi);
        }

        engageJoinGroup(id.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_ASSET_REDISCOVERED(const char *pId, const char *pszGroupJson, const char *eventExtraJson)
{
    LAMBDA_BEGIN_S1(pszGroupJson)
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_ASSET_REDISCOVERED %s, %s: ", id.c_str(), s1.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_ASSET_UNDISCOVERED(const char *pId, const char *pszGroupJson, const char *eventExtraJson)
{
    LAMBDA_BEGIN_S1(pszGroupJson)
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_ASSET_UNDISCOVERED %s, %s: ", id.c_str(), s1.c_str());

        engageLeaveGroup(id.c_str());
        engageDeleteGroup(id.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_LICENSE_CHANGED(const char *eventExtraJson)
{
    EEJ_LAMBDA_BEGIN()
    {
        const char *p = engageGetActiveLicenseDescriptor();
        g_miniLogger.d(TAG, "on_ENGAGE_LICENSE_CHANGED %s", p);
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_LICENSE_EXPIRED(const char *eventExtraJson)
{
    EEJ_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_LICENSE_EXPIRED");
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_LICENSE_EXPIRING(const char *pSecsLeft, const char *eventExtraJson)
{
    EEJ_S1_LAMBDA_BEGIN(pSecsLeft)
    {
        g_miniLogger.d(TAG, "on_ENGAGE_LICENSE_EXPIRING in %s", s1.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_BLOB_SENT(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_BLOB_SENT %s", id.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_BLOB_SEND_FAILED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_BLOB_SEND_FAILED %s", id.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_BLOB_RECEIVED(const char *pId, const char *pszBlobJson, const uint8_t *blob, size_t blobSize, const char *eventExtraJson)
{
    ConfigurationObjects::BlobInfo bi;

    if(!bi.deserialize(pszBlobJson))
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_BLOB_RECEIVED ERROR: Cannot parse blob info JSON");
        return;
    }

    // We'll make a copy to work with
    uint8_t *blobCopy = new uint8_t[blobSize];
    memcpy(blobCopy, blob, blobSize);

    if(bi.payloadType == ConfigurationObjects::BlobInfo::bptUndefined)
    {
        std::cout << "on_ENGAGE_GROUP_BLOB_RECEIVED [UNDEFINED]: " << pId << ", blobSize=" << blobSize << ", blobJson=" << pszBlobJson << std::endl;
        std::cout << "\tsrc=" << bi.source << std::endl;
        std::cout << "\ttgt=" << bi.target << std::endl;
    }
    else if(bi.payloadType == ConfigurationObjects::BlobInfo::bptAppTextUtf8)
    {
        std::cout << "on_ENGAGE_GROUP_BLOB_RECEIVED [APP TEXT UTF8]: " << pId << ", blobSize=" << blobSize << ", blobJson=" << pszBlobJson << std::endl;
        std::cout << "\tsrc=" << bi.source << std::endl;
        std::cout << "\ttgt=" << bi.target << std::endl;

        std::string msg;

        msg.assign((const char*)blob, blobSize);
        std::cout << "\tmsg=" << msg << std::endl;
    }
    else if(bi.payloadType == ConfigurationObjects::BlobInfo::bptJsonTextUtf8)
    {
        std::cout << "on_ENGAGE_GROUP_BLOB_RECEIVED [JSON TEXT UTF8]: " << pId << ", blobSize=" << blobSize << ", blobJson=" << pszBlobJson << std::endl;
        std::cout << "\tsrc=" << bi.source << std::endl;
        std::cout << "\ttgt=" << bi.target << std::endl;
    }
    else if(bi.payloadType == ConfigurationObjects::BlobInfo::bptAppBinary)
    {
        std::cout << "on_ENGAGE_GROUP_BLOB_RECEIVED [APP BINARY]: " << pId << ", blobSize=" << blobSize << ", blobJson=" << pszBlobJson << std::endl;
        std::cout << "\tsrc=" << bi.source << std::endl;
        std::cout << "\ttgt=" << bi.target << std::endl;
    }
    else if(bi.payloadType == ConfigurationObjects::BlobInfo::bptEngageBinaryHumanBiometrics)
    {
        std::cout << "on_ENGAGE_GROUP_BLOB_RECEIVED [ENGAGE HUMAN BIOMETRICS : HEART RATE]: " << pId << ", blobSize=" << blobSize << ", blobJson=" << pszBlobJson << std::endl;
        std::cout << "\tsrc=" << bi.source << std::endl;
        std::cout << "\ttgt=" << bi.target << std::endl;

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
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_RTP_SENT %s", id.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_RTP_SEND_FAILED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_RTP_SEND_FAILED %s", id.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_RTP_RECEIVED(const char *pId, const char *pszRtpHeaderJson, const uint8_t *payload, size_t payloadSize, const char *eventExtraJson)
{
    std::string id = pId;
    std::string eej = (eventExtraJson != nullptr ? eventExtraJson : "");
    std::string hj = (pszRtpHeaderJson != nullptr ? pszRtpHeaderJson : "");
    g_wq.submit(([id, eej, hj, payloadSize]()
    {
        MAYBE_UNUSED(id);
        MAYBE_UNUSED(eej);
        MAYBE_UNUSED(hj);

        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_RTP_RECEIVED %s, %zu, %s", id.c_str(), payloadSize, hj.c_str());
        ncRefreshUi();
    }
    ));
}

void on_ENGAGE_GROUP_RAW_SENT(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_RAW_SENT %s", id.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_RAW_SEND_FAILED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_RAW_SEND_FAILED %s", id.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_RAW_RECEIVED(const char *pId, const uint8_t *raw, size_t rawSize, const char *eventExtraJson)
{
    g_miniLogger.d(TAG, "on_ENGAGE_GROUP_RAW_RECEIVED %s", pId);
    ncRefreshUi();
}


void on_ENGAGE_GROUP_TIMELINE_EVENT_STARTED(const char *pId, const char *eventJson, const char *eventExtraJson)
{
    LAMBDA_BEGIN_S1(eventJson)
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_TIMELINE_EVENT_STARTED %s, %s", id.c_str(), s1.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_TIMELINE_EVENT_UPDATED(const char *pId, const char *eventJson, const char *eventExtraJson)
{
    LAMBDA_BEGIN_S1(eventJson)
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_TIMELINE_EVENT_UPDATED %s, %s", id.c_str(), s1.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_TIMELINE_EVENT_ENDED(const char *pId, const char *eventJson, const char *eventExtraJson)
{
    LAMBDA_BEGIN_S1(eventJson)
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_TIMELINE_EVENT_ENDED %s, %s", id.c_str(), s1.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_TIMELINE_REPORT(const char *pId, const char *reportJson, const char *eventExtraJson)
{
    LAMBDA_BEGIN_S1(reportJson)
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_TIMELINE_REPORT %s, %s", id.c_str(), s1.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_TIMELINE_REPORT_FAILED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.e(TAG, "on_ENGAGE_GROUP_TIMELINE_REPORT_FAILED %s", id.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_TIMELINE_GROOMED(const char *pId, const char *eventListJson, const char *eventExtraJson)
{
    LAMBDA_BEGIN_S1(eventListJson)
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_TIMELINE_GROOMED %s, %s", id.c_str(), s1.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_HEALTH_REPORT(const char *pId, const char *healthReportJson, const char *eventExtraJson)
{
    LAMBDA_BEGIN_S1(healthReportJson)
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_HEALTH_REPORT %s, %s", id.c_str(), s1.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_HEALTH_REPORT_FAILED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.e(TAG, "on_ENGAGE_GROUP_HEALTH_REPORT_FAILED %s", id.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_STATS_REPORT(const char *pId, const char *statsReportJson, const char *eventExtraJson)
{
    LAMBDA_BEGIN_S1(statsReportJson)
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_STATS_REPORT %s, %s", id.c_str(), s1.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_STATS_REPORT_FAILED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.e(TAG, "on_ENGAGE_GROUP_STATS_REPORT_FAILED %s", id.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_RX_VOLUME_CHANGED(const char *pId, int16_t leftLevelPerc, int16_t rightLevelPerc, const char *eventExtraJson)
{
    std::string id = pId;
    std::string eej = (eventExtraJson != nullptr ? eventExtraJson : "");
    g_wq.submit(([id, eej, leftLevelPerc, rightLevelPerc]()
    {
        MAYBE_UNUSED(id);
        MAYBE_UNUSED(eej);

        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_RX_VOLUME_CHANGED %s (%d, %d)", id.c_str(), leftLevelPerc, rightLevelPerc);
        ncRefreshUi();
    }
    ));
}

void on_ENGAGE_GROUP_RX_DTMF(const char *pId, const char *dtmfJson, const char *eventExtraJson)
{
    LAMBDA_BEGIN_S1(dtmfJson)
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_RX_DTMF %s: %s", id.c_str(), s1.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_RECONFIGURED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_RECONFIGURED %s: %s", id.c_str(), eej.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
}

void on_ENGAGE_GROUP_RECONFIGURATION_FAILED(const char *pId, const char *eventExtraJson)
{
    BASIC_ID_LAMBDA_BEGIN()
    {
        g_miniLogger.d(TAG, "on_ENGAGE_GROUP_RECONFIGURATION_FAILED %s: %s", id.c_str(), eej.c_str());
        ncRefreshUi();
    }
    LAMBDA_END()
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
                  iCreateUpTo,
                  iDelete,
                  iJoin,
                  iJoinUpTo,
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
                  iOnEqualGoto,
                  iOnLessGoto,
                  iOnGreaterGoto,
                  iCls,
                  iStartEngine,
                  iStopEngine,
                  iRestartEngine,
                  iReinitEngine,
                  iIfCondition,
                  iWaitForGroup,
                  iWaitForEngine,
                  iSetTxUri
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
            if(intParamFromPlaceholder)
            {
                std::string tmp = expandString(intPlaceholder.c_str());
                rndRange = atoi(tmp.c_str());
            }

            if(rndRange > 0)
            {
                return (myRand32() % rndRange);
            }
            else
            {
                return myRand32();
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
            if(intParamFromPlaceholder2)
            {
                std::string tmp = expandString(intPlaceholder2.c_str());
                rndRange = atoi(tmp.c_str());
            }

            if( rndRange > 0)
            {
                return (myRand32() % rndRange);
            }
            else
            {
                return myRand32();
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
    if(strcmp(tok, "(allgroups)") == 0)
    {
        pIns->intParam = -1;
    }
    else if(tok[0] == 'r')
    {
        tok++;
        if(tok[0] == '$')
        {
            pIns->intPlaceholder.assign(tok);
            pIns->intParamFromPlaceholder = true;
            pIns->randomizeInt = true;
        }
        else
        {
            pIns->intParam = atoi(tok);
            pIns->randomizeInt = true;
        }
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
    if(strcmp(tok, "(allgroups)") == 0)
    {
        pIns->intParam2 = -1;
    }
    else if(tok[0] == 'r')
    {
        tok++;

        if(tok[0] == '$')
        {
            pIns->intPlaceholder2.assign(tok);
            pIns->intParamFromPlaceholder2 = true;
            pIns->randomizeInt = true;
        }
        else
        {
            pIns->intParam2 = atoi(tok);
            pIns->randomizeInt = true;
        }
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
    else if( strcmp(tok, "createupto") == 0 )
    {
        pIns->type = Instruction::iCreateUpTo;
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
    else if( strcmp(tok, "joinupto") == 0 )
    {
        pIns->type = Instruction::iJoinUpTo;
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
    else if( strcmp(tok, "on") == 0 || strcmp(tok, "onequal") == 0 )
    {
        pIns->type = Instruction::iOnEqualGoto;

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
    else if( strcmp(tok, "onless") == 0 )
    {
        pIns->type = Instruction::iOnLessGoto;

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
    else if( strcmp(tok, "ongreater") == 0 )
    {
        pIns->type = Instruction::iOnGreaterGoto;

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
    else if( strcmp(tok, "enginerestart") == 0 )
    {
        pIns->type = Instruction::iRestartEngine;
    }
    else if( strcmp(tok, "enginereinit") == 0 )
    {
        pIns->type = Instruction::iReinitEngine;
    }
    else if( strcmp(tok, "settxuri") == 0 )
    {
        pIns->type = Instruction::iSetTxUri;

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        processIntParameter(pIns, tok);

        tok = strtok_s(nullptr, SEPS, &tokenCtx);
        pIns->stringParam = tok;
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
    bool rc = true;

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
            rc = false;
            break;
        }

        // Add the instruction
        g_scriptInstructions.push_back(ins);
    }

    fclose(fp);

    return rc;
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
    std::string rc;

    rc = s;

    while( true )
    {
        tokenStart = rc.find("${", tokenStart);
        if(tokenStart == std::string::npos)
        {
            break;
        }
        size_t tokenEnd = rc.find("}", tokenStart);
        if(tokenEnd == std::string::npos)
        {
            break;
        }

        std::string varName = rc.substr(tokenStart + 2, (tokenEnd - tokenStart - 2));
        std::string replSeapLdh = "${" + varName + "}";
        std::string replReplace;
        if(!varName.empty())
        {
            std::map<std::string, int>::iterator    itr = g_intVars.find(varName);
            if(itr != g_intVars.end())
            {
                char tmp[64];
                snprintf(tmp, sizeof(tmp), "%d", itr->second);
                replReplace = tmp;
            }
        }

        rc = replaceAll(rc, replSeapLdh, replReplace);
    }

    return rc;
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
    bool rc = false;
    bool error = false;
    std::list<Instruction>::iterator itrIns;

    // Set global variables
    size_t audioGroups = 0;
    size_t controlGroups = 0;
    size_t rawGroups = 0;
    for(size_t x = 0; x < g_new_groups.size(); x++)
    {
        if(g_new_groups[x].type == 1)
        {
            audioGroups++;
        }
        else if(g_new_groups[x].type == 2)
        {
            controlGroups++;
        }
        else if(g_new_groups[x].type == 3)
        {
            rawGroups++;
        }
    }

    setVar("@MISSION_OVERALL_GROUP_COUNT", (int)g_new_groups.size());
    setVar("@MISSION_AUDIO_GROUP_COUNT", (int)audioGroups);
    setVar("@MISSION_CONTROL_GROUP_COUNT", (int)controlGroups);
    setVar("@MISSION_RAW_GROUP_COUNT", (int)rawGroups);

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
                rc = true;
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
                doCreate(ins.getExpandedIntParam((int)g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iCreateUpTo:
            {
                doCreateUpTo(ins.getExpandedIntParam((int)ins.intParam));
                itrIns++;
            }
            break;

            case Instruction::iDelete:
            {
                doDelete(ins.getExpandedIntParam((int)g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iJoin:
            {
                doJoin(ins.getExpandedIntParam((int)g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iJoinUpTo:
            {
                doJoinUpTo(ins.getExpandedIntParam((int)g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iLeave:
            {
                doLeave(ins.getExpandedIntParam((int)g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iBeginTx:
            {
                doBeginTx(ins.getExpandedIntParam((int)g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iEndTx:
            {
                doEndTx(ins.getExpandedIntParam((int)g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iMuteRx:
            {
                doMuteRx(ins.getExpandedIntParam((int)g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iUnmuteRx:
            {
                doUnmuteRx(ins.getExpandedIntParam((int)g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iMuteTx:
            {
                doMuteTx(ins.getExpandedIntParam((int)g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iUnmuteTx:
            {
                doUnmuteTx(ins.getExpandedIntParam((int)g_new_groups.size()));
                itrIns++;
            }
            break;

            case Instruction::iSet:
            {
                error = !setVar(ins.stringParam.c_str(), ins.getExpandedIntParam(ins.intParam));
                itrIns++;
            }
            break;

            case Instruction::iAdd:
            {
                error = !addVar(ins.stringParam.c_str(), ins.getExpandedIntParam(ins.intParam));
                itrIns++;
            }
            break;

            case Instruction::iSub:
            {
                error = !subVar(ins.stringParam.c_str(), ins.getExpandedIntParam(ins.intParam * -1));
                itrIns++;
            }
            break;

            case Instruction::iCompare:
            {
                int result = 0;
                error = !compVar(ins.stringParam.c_str(), ins.getExpandedIntParam(ins.intParam), &result);
                if(!error)
                {
                    // what do with the comparison result
                }
                itrIns++;
            }
            break;

            case Instruction::iOnEqualGoto:
            case Instruction::iOnLessGoto:
            case Instruction::iOnGreaterGoto:
            {
                if(g_verboseScript) std::cout << "onXgoto: " << ins.stringParam << std::endl;

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
                    bool satisfied;

                    if(ins.type == Instruction::iOnEqualGoto)
                    {
                        satisfied = (itr->second == ins.intParam);
                    }
                    else if(ins.type == Instruction::iOnLessGoto)
                    {
                        satisfied = (itr->second > ins.intParam);
                    }
                    else
                    {
                        satisfied = (itr->second < ins.intParam);
                    }

                    if(satisfied)
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
                            std::cerr << "onXgoto: label '" << ins.stringParam2 << "' not found!" << std::endl;
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

            case Instruction::iRestartEngine:
            {
                doRestartEngine();
                itrIns++;
            }
            break;

            case Instruction::iReinitEngine:
            {
                doReinitEngine();
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
                            PlatformAbort();
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
                            ((ins.stringParam.compare("starting") == 0) && (g_engineStarting)) ||
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
                        PlatformAbort();
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
                            ((ins.stringParam.compare("txfailed") == 0) && (g_new_groups[ins.intParam].txFailed)) ||
                            ((ins.stringParam.compare("txusurped") == 0) && (g_new_groups[ins.intParam].txUsurped))
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
                        PlatformAbort();
                    }

                    itrIns = itrFind;
                }
                else
                {
                    itrIns++;
                }
            }
            break;

            case Instruction::iSetTxUri:
            {
                if(g_verboseScript) std::cout << "iSetTxUri: " << ins.stringParam << std::endl;
                std::string expanded = expandString(ins.stringParam.c_str());
                g_txUriRepeatCount = ins.getExpandedIntParam(0);
                g_txUriName = expanded;
                itrIns++;
            }
            break;
        }
    }

    return rc;
}

bool registerCallbacks()
{
    EngageEvents_t cb;

    memset(&cb, 0, sizeof(cb));

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
    cb.PFN_ENGAGE_GROUP_TIMELINE_GROOMED = on_ENGAGE_GROUP_TIMELINE_GROOMED;

    cb.PFN_ENGAGE_GROUP_HEALTH_REPORT = on_ENGAGE_GROUP_HEALTH_REPORT;
    cb.PFN_ENGAGE_GROUP_HEALTH_REPORT_FAILED = on_ENGAGE_GROUP_HEALTH_REPORT_FAILED;

    cb.PFN_ENGAGE_BRIDGE_CREATED = on_ENGAGE_BRIDGE_CREATED;
    cb.PFN_ENGAGE_BRIDGE_CREATE_FAILED = on_ENGAGE_BRIDGE_CREATE_FAILED;
    cb.PFN_ENGAGE_BRIDGE_DELETED = on_ENGAGE_BRIDGE_DELETED;

    cb.PFN_ENGAGE_GROUP_STATS_REPORT = on_ENGAGE_GROUP_STATS_REPORT;
    cb.PFN_ENGAGE_GROUP_STATS_REPORT_FAILED = on_ENGAGE_GROUP_STATS_REPORT_FAILED;

    cb.PFN_ENGAGE_GROUP_RX_VOLUME_CHANGED = on_ENGAGE_GROUP_RX_VOLUME_CHANGED;
    cb.PFN_ENGAGE_GROUP_RX_DTMF = on_ENGAGE_GROUP_RX_DTMF;

    cb.PFN_ENGAGE_GROUP_RECONFIGURED = on_ENGAGE_GROUP_RECONFIGURED;
    cb.PFN_ENGAGE_GROUP_RECONFIGURATION_FAILED = on_ENGAGE_GROUP_RECONFIGURATION_FAILED;

    return (engageRegisterEventCallbacks(&cb) == ENGAGE_RESULT_OK);
}

static int l_ecGetRandom64(lua_State *L)
{
    lua_pushinteger(L, (lua_Integer)myRand64());
    return 1;
}

static int l_ecGetTickMs(lua_State *L)
{
    lua_pushinteger(L, (lua_Integer)getTickMs());
    return 1;
}

static int l_ecGetWaitingKeyboardInput(lua_State *L)
{
    std::string str = getWaitingKeyboardInput();
    lua_pushstring(L, str.c_str());
    return 1;
}

static int l_ecStartEngine(lua_State *L)
{
    doStartEngine();
    return 0;
}

static int l_ecStopEngine(lua_State *L)
{
    doStopEngine();
    return 0;
}

static int l_ecRestartEngine(lua_State *L)
{
    doRestartEngine();
    return 0;
}

static int l_ecReinitEngine(lua_State *L)
{
    doReinitEngine();
    return 0;
}

static int l_ecIsEngineReinitInProgress(lua_State *L)
{
    lua_pushboolean(L, g_reinitInProgress == true);
    return 1;
}



static int l_ecIsEngineStarted(lua_State *L)
{
    lua_pushboolean(L, g_engineStarted == true);
    return 1;
}

static int l_ecIsEngineStopped(lua_State *L)
{
    lua_pushboolean(L, g_engineStopped == true);
    return 1;
}

static int l_ecLogFatal(lua_State *L)
{
    const char *msg = luaL_checkstring(L, 1);
    g_miniLogger.f("Lua", "%s", msg);
    return 0;
}

static int l_ecLogError(lua_State *L)
{
    const char *msg = luaL_checkstring(L, 1);
    g_miniLogger.e("Lua", "%s", msg);
    return 0;
}

static int l_ecLogWarn(lua_State *L)
{
    const char *msg = luaL_checkstring(L, 1);
    g_miniLogger.w("Lua", "%s", msg);
    return 0;
}

static int l_ecLogInfo(lua_State *L)
{
    const char *msg = luaL_checkstring(L, 1);
    g_miniLogger.i("Lua", "%s", msg);
    return 0;
}

static int l_ecLogDebug(lua_State *L)
{
    const char *msg = luaL_checkstring(L, 1);
    g_miniLogger.d("Lua", "%s", msg);
    return 0;
}

static int l_ecSleep(lua_State *L)
{
    std::this_thread::sleep_for(std::chrono::milliseconds((int)luaL_checkinteger(L, 1)));
    return 0;
}

static int l_ecIsGroupCreated(lua_State *L)
{
    lua_pushboolean(L, g_new_groups[(int)luaL_checkinteger(L, 1)].created);
    return 1;
}

static int l_ecIsGroupJoined(lua_State *L)
{
    lua_pushboolean(L, g_new_groups[(int)luaL_checkinteger(L, 1)].joined);
    return 1;
}

static int l_ecIsGroupConnected(lua_State *L)
{
    lua_pushboolean(L, g_new_groups[(int)luaL_checkinteger(L, 1)].connected);
    return 1;
}

static bool isReady(int idx)
{
    if(idx == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            if (!g_new_groups[x].created ||
                !g_new_groups[x].joined ||
                !g_new_groups[x].connected)
            {
                return false;
            }
        }

        return true;
    }
    else
    {
        return (g_new_groups[idx].created &&
                g_new_groups[idx].joined &&
                g_new_groups[idx].connected);
    }
}

static bool isTxStarted(int idx)
{
    if(idx == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            if(g_new_groups[x].type == 1)
            {
                if (!g_new_groups[x].txStarted)
                {
                    return false;
                }
            }
        }

        return true;
    }
    else
    {
        return (g_new_groups[idx].type == 1 && g_new_groups[idx].txStarted);
    }
}

static bool isTxEnded(int idx)
{
    if(idx == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            if(g_new_groups[x].type == 1)
            {
                if (!g_new_groups[x].txEnded)
                {
                    return false;
                }
            }
        }

        return true;
    }
    else
    {
        return (g_new_groups[idx].type == 1 && g_new_groups[idx].txEnded);
    }
}

static bool isTxFailed(int idx)
{
    if(idx == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            if(g_new_groups[x].type == 1)
            {
                if (!g_new_groups[x].txFailed)
                {
                    return false;
                }
            }
        }

        return true;
    }
    else
    {
        return (g_new_groups[idx].type == 1 && g_new_groups[idx].txFailed);
    }
}

static bool isTxUsurped(int idx)
{
    if(idx == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            if(g_new_groups[x].type == 1)
            {
                if (!g_new_groups[x].txUsurped)
                {
                    return false;
                }
            }
        }

        return true;
    }
    else
    {
        return (g_new_groups[idx].type == 1 && g_new_groups[idx].txUsurped);
    }
}

static bool isRxStarted(int idx)
{
    if(idx == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            if(g_new_groups[x].type == 1)
            {
                if (!g_new_groups[x].rxStarted)
                {
                    return false;
                }
            }
        }

        return true;
    }
    else
    {
        return (g_new_groups[idx].type == 1 && g_new_groups[idx].rxStarted);
    }
}

static bool isRxEnded(int idx)
{
    if(idx == -1)
    {
        for(size_t x = 0; x < g_new_groups.size(); x++)
        {
            if(g_new_groups[x].type == 1)
            {
                if (!g_new_groups[x].rxEnded)
                {
                    return false;
                }
            }
        }

        return true;
    }
    else
    {
        return (g_new_groups[idx].type == 1 && g_new_groups[idx].rxEnded);
    }
}

static int l_ecGetGroupCount(lua_State *L)
{
    lua_pushinteger(L, (int)g_new_groups.size());
    return 1;
}

static int l_ecGetGroupId(lua_State *L)
{
    lua_pushstring(L, g_new_groups[(int)luaL_checkinteger(L, 1)].id.c_str());
    return 1;
}

static int l_ecGetGroupName(lua_State *L)
{
    lua_pushstring(L, g_new_groups[(int)luaL_checkinteger(L, 1)].name.c_str());
    return 1;
}

static int l_ecGetGroupType(lua_State *L)
{
    lua_pushinteger(L, g_new_groups[(int)luaL_checkinteger(L, 1)].type);
    return 1;
}

static int l_ecIsGroupTypeAudio(lua_State *L)
{
    lua_pushboolean(L, g_new_groups[(int)luaL_checkinteger(L, 1)].type == 1);
    return 1;
}

static int l_ecGetGroupJson(lua_State *L)
{
    lua_pushstring(L, g_new_groups[(int)luaL_checkinteger(L, 1)].jsonConfiguration.c_str());
    return 1;
}

static int l_ecSetGroupJson(lua_State *L)
{
    int index = (int)luaL_optinteger(L, 1, -1);
    if(index >= 0)
    {
        g_new_groups[index].jsonConfiguration = luaL_checkstring(L, 2);
    }
    
    return 0;
}

static int l_ecIsGroupTypePresence(lua_State *L)
{
    lua_pushboolean(L, g_new_groups[(int)luaL_checkinteger(L, 1)].type == 2);
    return 1;
}

static int l_ecIsGroupTypeRaw(lua_State *L)
{
    lua_pushboolean(L, g_new_groups[(int)luaL_checkinteger(L, 1)].type == 3);
    return 1;
}

static int l_ecIsGroupReady(lua_State *L)
{
    lua_pushboolean(L, isReady((int)luaL_checkinteger(L, 1)));
    return 1;
}

static int l_ecWaitForGroupReady(lua_State *L)
{
    uint64_t started = getTickMs();
    uint64_t timeout = (uint64_t)luaL_optinteger(L, 2, -1);
    bool encounteredTimeout = false;

    while( !isReady((int)luaL_checkinteger(L, 1)) )
    {
        if(timeout > 0 && (getTickMs() - started >= timeout))
        {
            encounteredTimeout = true;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    lua_pushboolean(L, (encounteredTimeout ? 0 : 1));

    return 1;
}

static int l_ecIsGroupTxStarted(lua_State *L)
{
    lua_pushboolean(L, isTxStarted((int)luaL_checkinteger(L, 1)));
    return 1;
}

static int l_ecIsGroupTxFailed(lua_State *L)
{
    lua_pushboolean(L, isTxFailed((int)luaL_checkinteger(L, 1)));
    return 1;
}

static int l_ecIsGroupTxEnded(lua_State *L)
{
    lua_pushboolean(L, isTxEnded((int)luaL_checkinteger(L, 1)));
    return 1;
}

static int l_ecIsGroupTxUsurped(lua_State *L)
{
    lua_pushboolean(L, isTxUsurped((int)luaL_checkinteger(L, 1)));
    return 1;
}

static int l_ecWaitForGroupTxStarted(lua_State *L)
{
    uint64_t started = getTickMs();
    uint64_t timeout = (uint64_t)luaL_optinteger(L, 2, -1);
    bool encounteredTimeout = false;

    while( !isTxStarted((int)luaL_checkinteger(L, 1)) && 
           !isTxEnded((int)luaL_checkinteger(L, 1)) &&
           !isTxFailed((int)luaL_checkinteger(L, 1)) )
    {
        if(timeout > 0 && (getTickMs() - started >= timeout))
        {
            encounteredTimeout = true;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    lua_pushboolean(L, (encounteredTimeout ? 0 : 1));

    return 1;
}

static int l_ecWaitForGroupTxEnded(lua_State *L)
{
    uint64_t started = getTickMs();
    uint64_t timeout = (uint64_t)luaL_optinteger(L, 2, -1);
    bool encounteredTimeout = false;

    while( !isTxEnded((int)luaL_checkinteger(L, 1)) && !isTxFailed((int)luaL_checkinteger(L, 1)) )
    {
        if(timeout > 0 && (getTickMs() - started >= timeout))
        {
            encounteredTimeout = true;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    lua_pushboolean(L, (encounteredTimeout ? 0 : 1));

    return 1;
}

static int l_ecSetTxUri(lua_State *L)
{
    g_txUriRepeatCount = (int)luaL_checkinteger(L, 1);
    g_txUriName = luaL_checkstring(L, 2);
    return 0;
}

static int l_ecSetExitCode(lua_State *L)
{
    g_exitCode = (int)luaL_checkinteger(L, 1);
    return 0;
}

static int l_ecSetTxPriority(lua_State *L)
{
    g_txPriority = (int)luaL_checkinteger(L, 1);
    return 0;
}

static int l_ecSetTxFlags(lua_State *L)
{
    g_txFlags = (int)luaL_checkinteger(L, 1);
    return 0;
}

static int l_ecIsGroupRxStarted(lua_State *L)
{
    lua_pushboolean(L, isRxStarted((int)luaL_checkinteger(L, 1)));
    return 1;
}

static int l_ecIsGroupRxEnded(lua_State *L)
{
    lua_pushboolean(L, isRxEnded((int)luaL_checkinteger(L, 1)));
    return 1;
}

#define lua_pushglobalcfunction(_L, _F, _N) \
                                    lua_pushcfunction(_L, _F); \
                                    lua_setglobal(_L, _N)

#define LUA_FUNC(_fn) \
    lua_pushglobalcfunction(L, l_##_fn, #_fn)


#define LUA_GROUP_OP(_luaFn, _ecFn) \
    static int l_##_luaFn(lua_State *L) \
    { \
        _ecFn((int)luaL_optinteger(L, 1, -1)); \
        return 0; \
    }

#define LUA_GROUP_OP_T1(_luaFn, _ecFn) \
    static int l_##_luaFn(lua_State *L) \
    { \
        int idx = (int)luaL_checkinteger(L, 1); \
        if(idx == -1) \
        { \
            for(size_t x = 0; x < g_new_groups.size(); x++) \
            { \
                if(g_new_groups[x].type == 1) \
                { \
                    _ecFn((int)x); \
                } \
            } \
        } \
        else \
        { \
            _ecFn(idx); \
        } \
        return 0;\
    }

LUA_GROUP_OP(ecCreateGroup, doCreate)
LUA_GROUP_OP(ecDeleteGroup, doDelete)
LUA_GROUP_OP(ecJoinGroup, doJoin)
LUA_GROUP_OP(ecLeaveGroup, doLeave)

static int l_ecSetGroupAudioIds(lua_State *L)
{
    doSetGroupAudioIds((int)luaL_optinteger(L, 1, -1),
                       (int)luaL_optinteger(L, 2, -1),
                       (int)luaL_optinteger(L, 3, -1));

    return 0;
}

LUA_GROUP_OP_T1(ecBeginGroupTx, doBeginTx)
LUA_GROUP_OP_T1(ecEndGroupTx, doEndTx)

LUA_GROUP_OP_T1(ecMuteGroupTx, doMuteTx)
LUA_GROUP_OP_T1(ecUnmuteGroupTx, doUnmuteTx)

LUA_GROUP_OP_T1(ecMuteGroupRx, doMuteRx)
LUA_GROUP_OP_T1(ecUnmuteGroupRx, doUnmuteRx)


int runLuaScript(const char *fn)
{
    std::string cfn = fn;

    std::thread luaThread = std::thread([cfn]()
    {
        PlatformSetThreadName("luaExec");

        lua_State *L = luaL_newstate();
        luaL_openlibs(L);

        LUA_FUNC(ecGetRandom64);
        LUA_FUNC(ecGetTickMs);
        LUA_FUNC(ecGetWaitingKeyboardInput);

        LUA_FUNC(ecLogFatal);
        LUA_FUNC(ecLogError);
        LUA_FUNC(ecLogWarn);
        LUA_FUNC(ecLogInfo);
        LUA_FUNC(ecLogDebug);

        LUA_FUNC(ecSleep);

        LUA_FUNC(ecStartEngine);
        LUA_FUNC(ecStopEngine);
        LUA_FUNC(ecRestartEngine);
        LUA_FUNC(ecReinitEngine);

        LUA_FUNC(ecIsEngineStarted);
        LUA_FUNC(ecIsEngineStopped);

        LUA_FUNC(ecIsEngineReinitInProgress);

        LUA_FUNC(ecGetGroupCount);
        LUA_FUNC(ecGetGroupId);
        LUA_FUNC(ecGetGroupName);
        LUA_FUNC(ecGetGroupType);
        LUA_FUNC(ecGetGroupJson);
        LUA_FUNC(ecSetGroupJson);
        LUA_FUNC(ecIsGroupTypeAudio);
        LUA_FUNC(ecIsGroupTypePresence);
        LUA_FUNC(ecIsGroupTypeRaw);

        LUA_FUNC(ecIsGroupCreated);
        LUA_FUNC(ecIsGroupJoined);
        LUA_FUNC(ecIsGroupConnected);
        LUA_FUNC(ecIsGroupReady);
        LUA_FUNC(ecWaitForGroupReady);

        LUA_FUNC(ecIsGroupTxStarted);
        LUA_FUNC(ecIsGroupTxFailed);
        LUA_FUNC(ecIsGroupTxEnded);
        LUA_FUNC(ecIsGroupTxUsurped);
        LUA_FUNC(ecWaitForGroupTxStarted);
        LUA_FUNC(ecWaitForGroupTxEnded);

        LUA_FUNC(ecIsGroupRxStarted);
        LUA_FUNC(ecIsGroupRxEnded);

        LUA_FUNC(ecCreateGroup);
        LUA_FUNC(ecDeleteGroup);

        LUA_FUNC(ecJoinGroup);
        LUA_FUNC(ecLeaveGroup);

        LUA_FUNC(ecBeginGroupTx);
        LUA_FUNC(ecEndGroupTx);

        LUA_FUNC(ecMuteGroupTx);
        LUA_FUNC(ecUnmuteGroupTx);

        LUA_FUNC(ecMuteGroupRx);
        LUA_FUNC(ecUnmuteGroupRx);

        LUA_FUNC(ecSetGroupAudioIds);

        LUA_FUNC(ecSetTxUri);
        LUA_FUNC(ecSetExitCode);

        LUA_FUNC(ecSetTxPriority);
        LUA_FUNC(ecSetTxFlags);

        int rc = luaL_dofile(L, cfn.c_str());

        if(rc != 0)
        {
            g_miniLogger.e(TAG, "error while executing lua script - %s",
                                    lua_tostring(L, -1));
        }

        lua_close(L);
    });

    luaThread.join();

    return 0;
}

#if defined(RTS_SUPPORT_NCURSES)
WINDOW *ncCreateNewWin(const char *title, int height, int width, int starty, int startx)
{    
    WINDOW *rc = newwin(height, width, starty, startx);

    return rc;
}
#endif

void ncInit()
{
    if(!g_ncurses)
    {
        return;
    }
    
    #if defined(RTS_SUPPORT_NCURSES)
    {
        ncRefreshRequested = false;

        initscr();
        halfdelay(10);

        start_color();

        init_pair(ENGAGE_LOG_LEVEL_FATAL, COLOR_WHITE, COLOR_RED);
        init_pair(ENGAGE_LOG_LEVEL_ERROR, COLOR_RED, COLOR_BLACK);
        init_pair(ENGAGE_LOG_LEVEL_WARNING, COLOR_YELLOW, COLOR_BLACK);
        init_pair(ENGAGE_LOG_LEVEL_INFORMATIONAL, COLOR_GREEN, COLOR_BLACK);
        init_pair(ENGAGE_LOG_LEVEL_DEBUG, COLOR_WHITE, COLOR_BLACK);

        init_pair(CLR_RX_ON, COLOR_GREEN, COLOR_BLACK);
        init_pair(CLR_RX_OFF, COLOR_WHITE, COLOR_BLACK);

        int actHeight; // = (LINES / 2) - 2;
        
        actHeight = (LINES - 2);
        ncRxActivityWin = ncCreateNewWin("Activity", actHeight, COLS - 2, 0, 0);
        scrollok(ncRxActivityWin, true);
        
        //int groupHeight = LINES - actHeight - 2;
        //ncNodeWin = ncCreateNewWin("Nodes", groupHeight, COLS - 2, actHeight + 1, 0);
        //scrollok(ncRxActivityWin, true);
        
        //ncLogWin = ncCreateNewWin("Log", logHeight, COLS - 2, uiHeight + 1, 0);
        //scrollok(ncLogWin->win, true);
    }
    #endif
}

void ncLogThis(int level, const char *s)
{
    if(!g_ncurses)
    {
        return;
    }

    #if defined(RTS_SUPPORT_NCURSES)
    {
        /*
        std::string capturedS = s;
        g_wq.submit(([level, capturedS]()
        {
            ncLock.lock();
            {
                if(ncLogWin != nullptr)
                {
                    wattron(ncLogWin->win, COLOR_PAIR(level));
                    wprintw(ncLogWin->win, "%s\n", capturedS.c_str());
                    wattroff(ncLogWin->win, COLOR_PAIR(level));

                    ncRefreshScreen();
                }
            }
            ncLock.unlock();
        }
        ));
        */
    }
    #endif
}

void ncShutdown()
{
    if(!g_ncurses)
    {
        return;
    }
    
    #if defined(RTS_SUPPORT_NCURSES)
    {
        ncLock.lock();
        {
            if(ncRxActivityWin != nullptr)
            {
                delwin(ncRxActivityWin);
                ncRxActivityWin = nullptr;
            }

            if(ncNodeWin != nullptr)
            {
                delwin(ncNodeWin);
                ncNodeWin = nullptr;
            }

            if(ncLogWin != nullptr)
            {
                delwin(ncLogWin);
                ncLogWin = nullptr;
            }

            endwin();
        }
        ncLock.unlock();
    }
    #endif
}

const char *groupTypeDesc(int v)
{
    switch(v)
    {
        case 1:
            return "AUD";
        case 2:
            return "MC ";
        case 3:
            return "RAW";
        default:
            return "???";
    }
}

const char *groupNodeStatusDesc(int v)
{
    switch(v)
    {
        case 1:
            return "Created";
        case 2:
            return "Joined";
        case 3:
            return "Connected";
        default:
            return "???";
    }
}

void ncRefreshUi()
{
    if(!g_ncurses)
    {
        return;
    }

    #if defined(RTS_SUPPORT_NCURSES)
    {
        g_wq.submit(([]()
        {
            ncLock.lock();
            {
                ncRefreshUpdates++;

                char tmp[256];
                int row;

                if(ncRxActivityWin != nullptr)
                {
                    snprintf(tmp, sizeof(tmp), " Activity - %" PRIu64 " updates", ncRefreshUpdates);
                    wclear(ncRxActivityWin);
                    wmove(ncRxActivityWin, 0, 0);
                    wprintw(ncRxActivityWin, tmp);
                    
                    row = 1;        
                    for(size_t x = 0; x < g_new_groups.size(); x++)
                    {
                        wmove(ncRxActivityWin, row, 1);
                        
                        wprintw(ncRxActivityWin, "--------------------------------------------------------------------------------------------------------");
                        row++;

                        wmove(ncRxActivityWin, row, 1);

                        int color;

                        if(g_new_groups[x].rxStarted)
                        {
                            color = COLOR_PAIR(CLR_RX_ON);
                        }
                        else
                        {
                            color = COLOR_PAIR(CLR_RX_OFF);
                        }

                        wattron(ncRxActivityWin, color);

                        wprintw(ncRxActivityWin, "%2d %s %s %s [%s, %s, %s] %s", 
                                    x, 
                                    groupTypeDesc(g_new_groups[x].type),
                                    g_new_groups[x].id.c_str(),
                                    g_new_groups[x].name.c_str(),
                                    (g_new_groups[x].created ? "Created" : g_new_groups[x].createFailed ? "CreateFailed" : g_new_groups[x].deleted ? "Deleted" : "?"),
                                    (g_new_groups[x].joined ? "Joined" : g_new_groups[x].createFailed ? "JoinFailed" : g_new_groups[x].deleted ? "" : "?"),
                                    (g_new_groups[x].connected ? "Connected" : g_new_groups[x].connectFailed ? "ConnectFailed" : g_new_groups[x].notConnected ? "NotConnected" : "?"),                                    
                                    (g_new_groups[x].rxStarted ? "RX" : "  "));

                        wattroff(ncRxActivityWin, color);

                        row++;

                        for(std::list<std::string>::iterator itrTalker = g_new_groups[x].talkers.begin();
                            itrTalker != g_new_groups[x].talkers.end();
                            itrTalker++)
                        {
                            wmove(ncRxActivityWin, row, 1);
                            wprintw(ncRxActivityWin, "     -> %s", itrTalker->c_str());
                            row++;
                        }                
                    }
                }

                if(ncNodeWin != nullptr)
                {
                    snprintf(tmp, sizeof(tmp), " Nodes - %zu", g_nodes.size());
                    wclear(ncNodeWin);
                    wmove(ncNodeWin, 0, 0);
                    wprintw(ncNodeWin, tmp);

                    row = 1;
                    for(size_t x = 0; x < g_new_groups.size(); x++)
                    {
                        wmove(ncNodeWin, row, 1);                        
                        wprintw(ncNodeWin, "--------------------------------------------------------------------------------------------------------");
                        row++;

                        wmove(ncNodeWin, row, 1);                        
                        wprintw(ncNodeWin, "%2d %s %s %s  [%s, %s, %s]", 
                                    x, 
                                    groupTypeDesc(g_new_groups[x].type),
                                    g_new_groups[x].id.c_str(),
                                    g_new_groups[x].name.c_str(),
                                    (g_new_groups[x].created ? "Created" : g_new_groups[x].createFailed ? "CreateFailed" : g_new_groups[x].deleted ? "Deleted" : "?"),
                                    (g_new_groups[x].joined ? "Joined" : g_new_groups[x].createFailed ? "JoinFailed" : g_new_groups[x].deleted ? "" : "?"),
                                    (g_new_groups[x].connected ? "Connected" : g_new_groups[x].connectFailed ? "ConnectFailed" : g_new_groups[x].notConnected ? "NotConnected" : "?"));
                        row++;
                        
                        std::map<std::string, ConfigurationObjects::PresenceDescriptor>::iterator itrNodes;
                        for(itrNodes = g_nodes.begin();
                            itrNodes != g_nodes.end();
                            itrNodes++)
                        {
                            ConfigurationObjects::PresenceDescriptor *pd = &(itrNodes->second);
                            for(std::vector<ConfigurationObjects::PresenceDescriptorGroupItem>::iterator itrPdgi = pd->groupAliases.begin();
                                itrPdgi != pd->groupAliases.end();
                                itrPdgi++)
                            {
                                if(itrPdgi->groupId.compare(g_new_groups[x].id) == 0)
                                {
                                    wmove(ncNodeWin, row, 1);
                                    wprintw(ncNodeWin, "     -> %s%s [%s]  [%s]  [%s]", 
                                                (pd->self ? "*" : ""), 
                                                pd->identity.userId.c_str(),
                                                pd->identity.displayName.c_str(),
                                                itrPdgi->alias.c_str(),
                                                groupNodeStatusDesc(itrPdgi->status));
                                    row++;
                                }
                            }
                        }
                    }
                }

                ncRefreshScreen();                
            }
            ncLock.unlock();            
        }
        ));
    }
    #endif
}

void ncLoop()
{
    if(!g_ncurses)
    {
        return;
    }
    
    #if defined(RTS_SUPPORT_NCURSES)
    {
        while( true )
        {
            int rc = wgetch(ncRxActivityWin);
            if( rc != ERR )
            {
                break;
            }        
        }
    }
    #endif
}

void ncRefreshScreen()
{
    if(!g_ncurses)
    {
        return;
    }

    #if defined(RTS_SUPPORT_NCURSES)
    {
        //ncLock.lock();
        {
            if(!ncRefreshRequested)
            {
                ncRefreshRequested = true;

                g_wq.submit(([]()
                {
                    ncLock.lock();
                    {
                        ncRefreshRequested = false;

                        if(ncRxActivityWin != nullptr)
                        {
                            wrefresh(ncRxActivityWin);
                        }

                        if(ncNodeWin != nullptr)
                        {
                            wrefresh(ncNodeWin);
                        }
                        
                        if(ncLogWin != nullptr)
                        {
                            wrefresh(ncLogWin);
                        }

                    }
                    ncLock.unlock();
                }                
                ));
            }
        }
        //ncLock.unlock();
    }
    #endif
}
