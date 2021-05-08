//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

#include <nan.h>
#include <node_buffer.h>
#include <string>
#include <iostream>
#include <thread>
#include <map>
#include <mutex>
#include <atomic>

#include "EngageInterface.h"
#include "EngagePlatformNotifications.h"

using namespace std;
using namespace Nan;
using namespace v8;

#define ENGAGE_BINDING(_nm) \
    Nan::Set(target, \
        New<String>(#_nm).ToLocalChecked(), \
        GetFunction(New<FunctionTemplate>(_nm)).ToLocalChecked());

#define STRVAL(_infoIndex) \
    *Nan::Utf8String(info[_infoIndex]->ToString(Nan::GetCurrentContext()).FromMaybe(v8::Local<v8::String>()))

#define INTVAL(_index) \
    info[_index]->Int32Value(Nan::GetCurrentContext()).FromJust()

#define ENGAGE_CB_NO_PARAMS(_ename) \
    void on_ ## _ename(const char *eventExtraJson) \
    { \
        CrossThreadCallbackWorker *cbw = getCallback(#_ename); \
        if(!cbw) \
        { \
            return; \
        } \
        cbw->enqueue(eventExtraJson); \
        cbw->RELEASE_OBJECT_REFERENCE(); \
    }

#define ENGAGE_CB_STR_PARAM(_ename) \
    void on_ ## _ename(const char *str, const char *eventExtraJson) \
    { \
        CrossThreadCallbackWorker *cbw = getCallback(#_ename); \
        if(!cbw) \
        { \
            return; \
        } \
        cbw->enqueue(str, eventExtraJson); \
        cbw->RELEASE_OBJECT_REFERENCE(); \
    }

#define ENGAGE_CB_ID_PARAM(_ename) \
    void on_ ## _ename(const char *id, const char *eventExtraJson) \
    { \
        CrossThreadCallbackWorker *cbw = getCallback(#_ename); \
        if(!cbw) \
        { \
            return; \
        } \
        cbw->enqueue(id, eventExtraJson); \
        cbw->RELEASE_OBJECT_REFERENCE(); \
    }

#define ENGAGE_CB_ID_PLUS_ONE_STRING_PARAM(_ename) \
    void on_ ## _ename(const char *id, const char *s, const char *eventExtraJson) \
    { \
        CrossThreadCallbackWorker *cbw = getCallback(#_ename); \
        if(!cbw) \
        { \
            return; \
        } \
        cbw->enqueue(id, s, eventExtraJson); \
        cbw->RELEASE_OBJECT_REFERENCE(); \
    }

#define ENGAGE_CB_TABLE_ENTRY(_pfn, _ename) \
    g_eventCallbacks._pfn = on_ ## _ename;

// This little class wraps callbacks into JS from non-JS threads.  It does so by
// posting the callback/event onto the libuv event loop from which the instance
// of this worker class was created.  So ... be very careful only to create instances
// of this class from a thread that originated in libuv!!
class CrossThreadCallbackWorker
{
public:
    class Parameter
    {
    public:
        typedef enum {ptString, ptInt, ptStringVector} Type_t;

        Type_t _type;
    };

    class StringParameter : public Parameter
    {
    public:
        StringParameter(const char *s)
        {
            _type = ptString;
            if(s != nullptr && s[0] != 0)
            {
                _val = s;
            }
        }

        std::string _val;
    };

    class IntParameter : public Parameter
    {
    public:
        IntParameter(int i)
        {
            _type = ptInt;
            _val = i;
        }

        int _val;
    };

    class StringVectorParameter : public Parameter
    {
    public:
        StringVectorParameter(const char *array[])
        {
            _type = ptStringVector;

            // Our array is either null to begin with or terminated with a nullptr at the end
            if(array != nullptr)
            {
                size_t index = 0;
                while(array[index] != nullptr)
                {
                    _val.push_back(array[index] != nullptr ? array[index] : "");
                    index++;
                }
            }
        }

        std::vector<std::string> _val;
    };

    explicit CrossThreadCallbackWorker(v8::Local<v8::Function> fn)
    {
        Nan::HandleScope scope;

        // We're going to post this work on this libuv queue
        _evLoop = GetCurrentEventLoop();

        // Marshal the V8 function into a NAN callback structure
        _cb.Reset(fn);

        _workCtx.data = this;
        _isBusy = false;
        _refCount = 0;

        // TODO: I'm not convinced we even need this as there's no object here to get
        // garbage-collected.  TBD !!
        v8::Local<v8::Object> obj = New<v8::Object>();
        _persistentHandle.Reset(obj);
        _resource = new AsyncResource("CrossThreadCallbackWorker", obj);

        ADD_OBJECT_REFERENCE();
    }

    virtual ~CrossThreadCallbackWorker()
    {
        Nan::HandleScope scope;

        if (!_persistentHandle.IsEmpty())
        {
            _persistentHandle.Reset();
        }

        delete _resource;
    }

    void ADD_OBJECT_REFERENCE()
    {
        assert(_refCount >= 0);
        _refCount++;
    }

    void RELEASE_OBJECT_REFERENCE()
    {
        assert(_refCount > 0);
        if(_refCount.fetch_sub(1) == 1)
        {
            delete this;
        }
    }

    void enqueue(const char *extra)
    {
        std::vector<Parameter*> *params = new std::vector<Parameter*>();
        params->push_back(new StringParameter(extra));
        enqueue(params);
    }

    void enqueue(const char *s, const char *extra)
    {
        std::vector<Parameter*> *params = new std::vector<Parameter*>();
        params->push_back(new StringParameter(s));
        params->push_back(new StringParameter(extra));
        enqueue(params);
    }

    void enqueue(const char *s1, const char *s2, const char *extra)
    {
        std::vector<Parameter*> *params = new std::vector<Parameter*>();
        params->push_back(new StringParameter(s1));
        params->push_back(new StringParameter(s2));
        params->push_back(new StringParameter(extra));
        enqueue(params);
    }

    void enqueue(std::vector<Parameter*> *parameters)
    {
        // We need to wait here for the worker to become available (it may already be
        // queued in JS).  The chances of this happening are very slim as callbacks
        // don't happen that often and, when they do, they'll return in just a few
        // microseconds.  But, let's be paranoid shall we!
        _lock.lock();

        while(_isBusy)
        {
            _lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            _lock.lock();
        }

        // Pass over the pending parameters
        _pendingParameters = parameters;

        _isBusy = true;

        _lock.unlock();

        ADD_OBJECT_REFERENCE();
        uv_queue_work(_evLoop,
                      &_workCtx,
                      CrossThreadCallbackWorker::onExecuteWork,
                      reinterpret_cast<uv_after_work_cb>(CrossThreadCallbackWorker::onWorkCompleted));
    }

    static void onExecuteWork(uv_work_t* workCtx)
    {
        // libuv wants us to execute something.  But we don't have any actual work to do.  So
        // we'll just return.
    }

    static void onWorkCompleted(uv_work_t* workCtx)
    {
        // We'll just bounce this though to the instance's method
        static_cast<CrossThreadCallbackWorker*>(workCtx->data)->internal_onWorkCompleted();
    }

private:
    NAN_DISALLOW_ASSIGN_COPY_MOVE(CrossThreadCallbackWorker)

    void internal_onWorkCompleted()
    {
        _lock.lock();

        // The real work here is to make the callback...
        Nan::HandleScope scope;

        v8::Isolate         *isolate = v8::Isolate::GetCurrent();
        v8::Local<Context>  context = v8::Context::New(isolate);

        // ... which we'll do here.
        if(_pendingParameters == nullptr || _pendingParameters->size() == 0)
        {
            _cb.Call(0, nullptr, _resource);
        }
        else
        {
            // We can only call into V8 here to build up the parameters because only now are we
            // on a thread that V8 owns.
            v8::Local<v8::Value> *argv = new v8::Local<v8::Value>[_pendingParameters->size()];

            int index = 0;
            for(std::vector<Parameter*>::iterator itr = _pendingParameters->begin();
                itr != _pendingParameters->end();
                itr++)
            {
                if((*itr)->_type == Parameter::ptString)
                {
                    argv[index] = Nan::New<v8::String>(((StringParameter*)(*itr))->_val).ToLocalChecked();
                }
                else if((*itr)->_type == Parameter::ptStringVector)
                {
                    StringVectorParameter *svp = (StringVectorParameter*)(*itr);
                    v8::Local<v8::Array> jsArray = Nan::New<v8::Array>(svp->_val.size());

                    int speakerIndex = 0;
                    for(std::vector<std::string>::iterator itrSpeakers = svp->_val.begin();
                        itrSpeakers != svp->_val.end();
                        itrSpeakers++)
                    {
                        #ifndef WIN32
                            #pragma GCC diagnostic push
                            #pragma GCC diagnostic ignored "-Wunused-result"
                        #endif
                        {
                            jsArray->Set(context, speakerIndex, v8::String::NewFromUtf8(isolate, itrSpeakers->c_str(), NewStringType::kNormal).ToLocalChecked());
                        }
                        #ifndef WIN32
                            #pragma GCC diagnostic pop
                        #endif

                        speakerIndex++;
                    }

                    argv[index] = jsArray;
                }
                else if((*itr)->_type == Parameter::ptInt)
                {
                    argv[index] = Nan::New<v8::Integer>(((IntParameter*)(*itr))->_val);
                }

                index++;
            }

            // Call into JS-land
            _cb.Call(_pendingParameters->size(), argv, _resource);

            // Get rid of any pending parameters
            if(_pendingParameters != nullptr)
            {
                for(std::vector<Parameter*>::iterator itr = _pendingParameters->begin();
                    itr != _pendingParameters->end();
                    itr++)
                {
                    delete *itr;
                }

                _pendingParameters->clear();
                delete _pendingParameters;
                _pendingParameters = nullptr;
            }
        }

        // ... and only now can we say that this worker is no longer in use - i.e. its out
        // out the libuv queue and won't cause a bus error in case it gets reused very
        // quickly (that can happen if this code runs in onExecuteWork).
        _isBusy = false;

        _lock.unlock();

        // Finally, let go of the reference we added when this was enqueue
        RELEASE_OBJECT_REFERENCE();
    }

    std::mutex _lock;
    uv_loop_t *_evLoop;
    uv_work_t _workCtx;
    bool _isBusy;
    Nan::Callback _cb;
    Nan::Persistent<v8::Object> _persistentHandle;
    AsyncResource *_resource;
    std::atomic<int>  _refCount;
    std::vector<Parameter*> *_pendingParameters;
};

typedef std::map<std::string, CrossThreadCallbackWorker*> CallbackMap_t;

static EngageEvents_t g_eventCallbacks;
static bool g_wantCallbacks = true;
static CallbackMap_t g_cbMap;
static std::mutex g_cbMapLock;
static std::string g_loggingHookFn;

//--------------------------------------------------------
// Returns the callback associated with an event name (if any)
static CrossThreadCallbackWorker *getCallback(const char *cbName)
{
    if(!g_wantCallbacks)
    {
        return nullptr;
    }

    CrossThreadCallbackWorker *rc = nullptr;

    g_cbMapLock.lock();

    CallbackMap_t::iterator itr = g_cbMap.find(cbName);
    if(itr != g_cbMap.end())
    {
        rc = itr->second;
        rc->ADD_OBJECT_REFERENCE();
    }

    g_cbMapLock.unlock();

    return rc;
}


// The following convey events from Engage up to JS.  They all follow the same
// basic flow of finding a callback for the event name and, if one is found, calling
// it.  Generally callbacks have no parameters, or a single string parameter.  But,
// some of them are a little unique.  So, for those that look the same, we'll use the
// ENGAGE_CB_xx_PARAMS macros.  For the others, we'll actually write all the code for each.
ENGAGE_CB_NO_PARAMS(engineStarted)
ENGAGE_CB_NO_PARAMS(engineStopped)
ENGAGE_CB_NO_PARAMS(engineAudioDevicesRefreshed)

ENGAGE_CB_ID_PARAM(rpPausingConnectionAttempt)
ENGAGE_CB_ID_PARAM(rpConnecting)
ENGAGE_CB_ID_PARAM(rpConnected)
ENGAGE_CB_ID_PARAM(rpDisconnected)
void on_rpRoundtripReport(const char *id, uint32_t rtMs, uint32_t rtQualityRating, const char * eventExtraJson)
{
    CrossThreadCallbackWorker *cbw = getCallback("rpRoundtripReport");
    if(!cbw)
    {
        return;
    }

    std::vector<CrossThreadCallbackWorker::Parameter*> *params = new std::vector<CrossThreadCallbackWorker::Parameter*>();
    params->push_back(new CrossThreadCallbackWorker::StringParameter(id));
    params->push_back(new CrossThreadCallbackWorker::IntParameter((int)rtMs));
    params->push_back(new CrossThreadCallbackWorker::IntParameter((int)rtQualityRating));
    params->push_back(new CrossThreadCallbackWorker::StringParameter(eventExtraJson));
    cbw->enqueue(params);

    cbw->RELEASE_OBJECT_REFERENCE();
}

ENGAGE_CB_ID_PARAM(groupCreated)
ENGAGE_CB_ID_PARAM(groupCreateFailed)
ENGAGE_CB_ID_PARAM(groupDeleted)

ENGAGE_CB_ID_PARAM(groupConnected)
ENGAGE_CB_ID_PARAM(groupConnectFailed)
ENGAGE_CB_ID_PARAM(groupDisconnected)

ENGAGE_CB_ID_PARAM(groupJoined)
ENGAGE_CB_ID_PARAM(groupJoinFailed)
ENGAGE_CB_ID_PARAM(groupLeft)

ENGAGE_CB_ID_PARAM(groupRxStarted)
ENGAGE_CB_ID_PARAM(groupRxEnded)

ENGAGE_CB_ID_PARAM(groupRxMuted)
ENGAGE_CB_ID_PARAM(groupRxUnmuted)
ENGAGE_CB_ID_PLUS_ONE_STRING_PARAM(groupRxSpeakersChanged)

ENGAGE_CB_ID_PLUS_ONE_STRING_PARAM(groupNodeDiscovered)
ENGAGE_CB_ID_PLUS_ONE_STRING_PARAM(groupNodeRediscovered)
ENGAGE_CB_ID_PLUS_ONE_STRING_PARAM(groupNodeUndiscovered)

ENGAGE_CB_ID_PARAM(groupTxStarted);
ENGAGE_CB_ID_PARAM(groupTxEnded);
ENGAGE_CB_ID_PARAM(groupTxFailed);
ENGAGE_CB_ID_PARAM(groupTxUsurpedByPriority);
ENGAGE_CB_ID_PARAM(groupMaxTxTimeExceeded);

ENGAGE_CB_ID_PARAM(groupTxMuted)
ENGAGE_CB_ID_PARAM(groupTxUnmuted)

ENGAGE_CB_ID_PLUS_ONE_STRING_PARAM(groupAssetDiscovered);
ENGAGE_CB_ID_PLUS_ONE_STRING_PARAM(groupAssetRediscovered);
ENGAGE_CB_ID_PLUS_ONE_STRING_PARAM(groupAssetUndiscovered);

ENGAGE_CB_NO_PARAMS(licenseChanged)
ENGAGE_CB_NO_PARAMS(licenseExpired)
ENGAGE_CB_STR_PARAM(licenseExpiring)

ENGAGE_CB_ID_PLUS_ONE_STRING_PARAM(groupTimelineEventStarted);
ENGAGE_CB_ID_PLUS_ONE_STRING_PARAM(groupTimelineEventUpdated);
ENGAGE_CB_ID_PLUS_ONE_STRING_PARAM(groupTimelineEventEnded);
ENGAGE_CB_ID_PLUS_ONE_STRING_PARAM(groupTimelineReport);
ENGAGE_CB_ID_PARAM(groupTimelineReportFailed);
ENGAGE_CB_ID_PLUS_ONE_STRING_PARAM(groupTimelineGroomed);

ENGAGE_CB_ID_PLUS_ONE_STRING_PARAM(groupHealthReport);
ENGAGE_CB_ID_PARAM(groupHealthReportFailed);

ENGAGE_CB_ID_PLUS_ONE_STRING_PARAM(groupStatsReport);
ENGAGE_CB_ID_PARAM(groupStatsReportFailed);

void on_groupRxVolumeChanged(const char *id, int16_t leftLevelPerc, int16_t rightLevelPerc, const char * eventExtraJson)
{
    CrossThreadCallbackWorker *cbw = getCallback("groupRxVolumeChanged");
    if(!cbw)
    {
        return;
    }

    std::vector<CrossThreadCallbackWorker::Parameter*> *params = new std::vector<CrossThreadCallbackWorker::Parameter*>();
    params->push_back(new CrossThreadCallbackWorker::StringParameter(id));
    params->push_back(new CrossThreadCallbackWorker::IntParameter((int)leftLevelPerc));
    params->push_back(new CrossThreadCallbackWorker::IntParameter((int)rightLevelPerc));
    params->push_back(new CrossThreadCallbackWorker::StringParameter(eventExtraJson));
    cbw->enqueue(params);

    cbw->RELEASE_OBJECT_REFERENCE();
}

ENGAGE_CB_ID_PLUS_ONE_STRING_PARAM(groupRxDtmf)


//--------------------------------------------------------
// Registers an event name and the JS callback function
NAN_METHOD(on)
{
    bool haveAFunction = false;
    std::string functionName = STRVAL(0);

    if(info.Length() >= 2 && (!info[1]->IsUndefined()) && (!info[1]->IsNull()))
    {
        haveAFunction = true;
    }

    g_cbMapLock.lock();

    // Find our map entry
    CallbackMap_t::iterator itr = g_cbMap.find(functionName.c_str());

    // We don't yet have an entry in the map
    if(itr == g_cbMap.end())
    {
        // If we don't have a function, then we're done
        if(!haveAFunction)
        {
            g_cbMapLock.unlock();
            return;
        }

        // We have a function but not yet an entry, make it and plug it into the map
        g_cbMap[functionName.c_str()] = new CrossThreadCallbackWorker(Nan::To<v8::Function>(info[1]).ToLocalChecked());
    }
    else
    {
        // We have an entry ...

        // ... get rid of it (we're either removing or replacing)
        itr->second->RELEASE_OBJECT_REFERENCE();
        g_cbMap.erase(itr);

        // ... and maybe put in the new one
        if(haveAFunction)
        {
            g_cbMap[functionName.c_str()] = new CrossThreadCallbackWorker(Nan::To<v8::Function>(info[1]).ToLocalChecked());
        }
    }

    g_cbMapLock.unlock();
}


//--------------------------------------------------------
NAN_METHOD(initialize)
{
    memset(&g_eventCallbacks, 0, sizeof(g_eventCallbacks));

    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_ENGINE_STARTED, engineStarted);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_ENGINE_STOPPED, engineStopped);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_ENGINE_AUDIO_DEVICES_REFRESHED, engineAudioDevicesRefreshed);

    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_RP_PAUSING_CONNECTION_ATTEMPT, rpPausingConnectionAttempt);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_RP_CONNECTING, rpConnecting);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_RP_CONNECTED, rpConnected);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_RP_DISCONNECTED, rpDisconnected);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_RP_ROUNDTRIP_REPORT, rpRoundtripReport);

    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_CREATED, groupCreated);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_CREATE_FAILED, groupCreateFailed);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_DELETED, groupDeleted);

    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_CONNECTED, groupConnected);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_CONNECT_FAILED, groupConnectFailed);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_DISCONNECTED, groupDisconnected);

    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_JOINED, groupJoined);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_JOIN_FAILED, groupJoinFailed);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_LEFT, groupLeft);
    //ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_MEMBER_COUNT_CHANGED)(const char *pId, size_t newCount);

    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_NODE_DISCOVERED, groupNodeDiscovered);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_NODE_REDISCOVERED, groupNodeRediscovered);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_NODE_UNDISCOVERED, groupNodeUndiscovered);

    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_RX_STARTED, groupRxStarted);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_RX_ENDED, groupRxEnded);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_RX_SPEAKERS_CHANGED, groupRxSpeakersChanged);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_RX_MUTED, groupRxMuted);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_RX_UNMUTED, groupRxUnmuted);

    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_TX_STARTED, groupTxStarted);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_TX_ENDED, groupTxEnded);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_TX_FAILED, groupTxFailed);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_TX_USURPED_BY_PRIORITY, groupTxUsurpedByPriority);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_MAX_TX_TIME_EXCEEDED, groupMaxTxTimeExceeded);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_TX_MUTED, groupTxMuted);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_TX_UNMUTED, groupTxUnmuted);

    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_ASSET_DISCOVERED, groupAssetDiscovered);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_ASSET_REDISCOVERED, groupAssetRediscovered);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_ASSET_UNDISCOVERED, groupAssetUndiscovered);

    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_LICENSE_CHANGED, licenseChanged);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_LICENSE_EXPIRED, licenseExpired);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_LICENSE_EXPIRING, licenseExpiring);

    // TODO PFN_ENGAGE_GROUP_BLOB_SENT
    // TODO PFN_ENGAGE_GROUP_BLOB_SEND_FAILED
    // TODO PFN_ENGAGE_GROUP_BLOB_RECEIVED

    // TODO PFN_ENGAGE_GROUP_RTP_SENT
    // TODO PFN_ENGAGE_GROUP_RTP_SEND_FAILED
    // TODO PFN_ENGAGE_GROUP_RTP_RECEIVED

    // TODO PFN_ENGAGE_GROUP_RAW_SENT
    // TODO PFN_ENGAGE_GROUP_RAW_SEND_FAILED
    // TODO PFN_ENGAGE_GROUP_RAW_RECEIVED

    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_TIMELINE_EVENT_STARTED, groupTimelineEventStarted);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_TIMELINE_EVENT_UPDATED, groupTimelineEventUpdated);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_TIMELINE_EVENT_ENDED, groupTimelineEventEnded);
    
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_TIMELINE_REPORT, groupTimelineReport);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_TIMELINE_REPORT_FAILED, groupTimelineReportFailed);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_TIMELINE_GROOMED, groupTimelineGroomed);
    
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_HEALTH_REPORT, groupHealthReport);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_HEALTH_REPORT_FAILED, groupHealthReportFailed);
    
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_STATS_REPORT, groupStatsReport);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_STATS_REPORT_FAILED, groupStatsReportFailed);

    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_RX_VOLUME_CHANGED, groupRxVolumeChanged);
    ENGAGE_CB_TABLE_ENTRY(PFN_ENGAGE_GROUP_RX_DTMF, groupRxDtmf);

    engageRegisterEventCallbacks(&g_eventCallbacks);

    engageInitialize(STRVAL(0), STRVAL(1), STRVAL(2));
}

//--------------------------------------------------------
NAN_METHOD(enableCallbacks)
{
    g_wantCallbacks = true;
}

//--------------------------------------------------------
NAN_METHOD(disableCallbacks)
{
    g_wantCallbacks = false;
}

//--------------------------------------------------------
NAN_METHOD(setLogLevel)
{
    engageSetLogLevel(INTVAL(0));
}

//--------------------------------------------------------
NAN_METHOD(setLogTagExtension)
{
    engageSetLogTagExtension(STRVAL(0));
}

//--------------------------------------------------------
NAN_METHOD(engageEnableSyslog)
{
    engageEnableSyslog(INTVAL(0) == 1 ? ENGAGE_SYSLOG_ENABLE : ENGAGE_SYSLOG_DISABLE);
}

//--------------------------------------------------------
NAN_METHOD(engageEnableWatchdog)
{
    engageEnableWatchdog(INTVAL(0) == 1 ? ENGAGE_WATCHDOG_ENABLE : ENGAGE_WATCHDOG_DISABLE);
}

//--------------------------------------------------------
NAN_METHOD(shutdown)
{
    engageShutdown();
}

//--------------------------------------------------------
NAN_METHOD(start)
{
    engageStart();
}

//--------------------------------------------------------
NAN_METHOD(stop)
{
    engageStop();
}

//--------------------------------------------------------
NAN_METHOD(createGroup)
{
    engageCreateGroup(STRVAL(0));
}

//--------------------------------------------------------
NAN_METHOD(deleteGroup)
{
    engageDeleteGroup(STRVAL(0));
}

//--------------------------------------------------------
NAN_METHOD(joinGroup)
{
    engageJoinGroup(STRVAL(0));
}

//--------------------------------------------------------
NAN_METHOD(leaveGroup)
{
    engageLeaveGroup(STRVAL(0));
}

//--------------------------------------------------------
NAN_METHOD(beginGroupTx)
{
    engageBeginGroupTx(STRVAL(0), INTVAL(1), INTVAL(2));
}

//--------------------------------------------------------
NAN_METHOD(beginGroupTxAdvanced)
{
    engageBeginGroupTxAdvanced(STRVAL(0), STRVAL(1));
}

//--------------------------------------------------------
NAN_METHOD(endGroupTx)
{
    engageEndGroupTx(STRVAL(0));
}

//--------------------------------------------------------
NAN_METHOD(setGroupRxTag)
{
    engageSetGroupRxTag(STRVAL(0), INTVAL(1));
}

//--------------------------------------------------------
NAN_METHOD(muteGroupRx)
{
    engageMuteGroupRx(STRVAL(0));
}

//--------------------------------------------------------
NAN_METHOD(unmuteGroupRx)
{
    engageUnmuteGroupRx(STRVAL(0));
}

//--------------------------------------------------------
NAN_METHOD(muteGroupTx)
{
    engageMuteGroupTx(STRVAL(0));
}

//--------------------------------------------------------
NAN_METHOD(unmuteGroupTx)
{
    engageUnmuteGroupTx(STRVAL(0));
}

//--------------------------------------------------------
NAN_METHOD(setGroupRxVolume)
{
    engageSetGroupRxVolume(STRVAL(0), INTVAL(1), INTVAL(2));
}

//--------------------------------------------------------
NAN_METHOD(updatePresenceDescriptor)
{
    engageUpdatePresenceDescriptor(STRVAL(0), STRVAL(1), INTVAL(0));
}

//--------------------------------------------------------
NAN_METHOD(encrypt)
{
    uint8_t* inputBytes = (uint8_t*) node::Buffer::Data(info[0]);

    size_t inputOfs = INTVAL(1);
    size_t inputLen = INTVAL(2);

    // Our output is going to contain encrypted data padded to 16 bytes + another 16 bytes of IV
    uint8_t *outputBytes = new uint8_t[inputLen + 16 * 2];

    int bytesInOutput =  engageEncrypt(inputBytes + inputOfs, inputLen, outputBytes, STRVAL(3));

    if(bytesInOutput > 0)
    {
        info.GetReturnValue().Set(Nan::CopyBuffer((char*)outputBytes, bytesInOutput).ToLocalChecked());
    }

    delete[] outputBytes;
}

//--------------------------------------------------------
NAN_METHOD(decrypt)
{
    uint8_t* inputBytes = (uint8_t*) node::Buffer::Data(info[0]);

    size_t inputOfs = INTVAL(1);
    size_t inputLen = INTVAL(2);

    // Our output is not going to be larger than the input (if anything, it'll be smaller)
    uint8_t *outputBytes = new uint8_t[inputLen];

    int bytesInOutput =  engageDecrypt(inputBytes + inputOfs, inputLen, outputBytes, STRVAL(3));

    if(bytesInOutput > 0)
    {
        info.GetReturnValue().Set(Nan::CopyBuffer((char*)outputBytes, bytesInOutput).ToLocalChecked());
    }

    delete[] outputBytes;
}

//--------------------------------------------------------
NAN_METHOD(getVersion)
{
    const char *rc = engageGetVersion();

    if(rc == nullptr)
    {
        rc = "";
    }

    info.GetReturnValue().Set(New(rc).ToLocalChecked());
}

//--------------------------------------------------------
NAN_METHOD(getHardwareReport)
{
    const char *rc = engageGetHardwareReport();

    if(rc == nullptr)
    {
        rc = "";
    }

    info.GetReturnValue().Set(New(rc).ToLocalChecked());
}

//--------------------------------------------------------
NAN_METHOD(getActiveLicenseDescriptor)
{
    const char *rc = engageGetActiveLicenseDescriptor();

    if(rc == nullptr)
    {
        rc = "";
    }

    info.GetReturnValue().Set(New(rc).ToLocalChecked());
}

//--------------------------------------------------------
NAN_METHOD(getLicenseDescriptor)
{
    const char *rc = engageGetLicenseDescriptor(STRVAL(0), STRVAL(1), STRVAL(2), STRVAL(3));

    if(rc == nullptr)
    {
        rc = "";
    }

    info.GetReturnValue().Set(New(rc).ToLocalChecked());
}

//--------------------------------------------------------
NAN_METHOD(updateLicense)
{
    engageUpdateLicense(STRVAL(0), STRVAL(1), STRVAL(2), STRVAL(3));
}

// TODO: engageSendGroupRtp
// TODO: engageRegisterGroupRtpHandler
// TODO: engageUnregisterGroupRtpHandler
// TODO: engageSendGroupBlob
// TODO: engageSendGroupRaw
// TODO: engagePlatformServiceDiscovered
// TODO: engagePlatformServiceRediscovered
// TODO: engagePlatformServiceUndiscovered

//--------------------------------------------------------
NAN_METHOD(queryGroupTimeline)
{
    engageQueryGroupTimeline(STRVAL(0), STRVAL(1));
}

//--------------------------------------------------------
NAN_METHOD(queryGroupHealth)
{
    engageQueryGroupHealth(STRVAL(0));
}

//--------------------------------------------------------
NAN_METHOD(queryGroupStats)
{
    engageQueryGroupStats(STRVAL(0));
}

//--------------------------------------------------------
NAN_METHOD(getNetworkInterfaceDevices)
{
    const char *rc = engageGetNetworkInterfaceDevices();

    if(rc == nullptr)
    {
        rc = "";
    }

    info.GetReturnValue().Set(New(rc).ToLocalChecked());
}

//--------------------------------------------------------
NAN_METHOD(getAudioDevices)
{
    const char *rc = engageGetAudioDevices();

    if(rc == nullptr)
    {
        rc = "";
    }

    info.GetReturnValue().Set(New(rc).ToLocalChecked());
}

//--------------------------------------------------------
NAN_METHOD(generateMission)
{
    const char *rc = engageGenerateMission(STRVAL(0), INTVAL(1), STRVAL(2), STRVAL(3));

    if(rc == nullptr)
    {
        rc = "";
    }

    info.GetReturnValue().Set(New(rc).ToLocalChecked());
}

//--------------------------------------------------------
NAN_METHOD(generateMissionUsingCertStore)
{
    const char *rc = engageGenerateMissionUsingCertStore(STRVAL(0), INTVAL(1), STRVAL(2), STRVAL(3), STRVAL(4), STRVAL(5), STRVAL(6));

    if(rc == nullptr)
    {
        rc = "";
    }

    info.GetReturnValue().Set(New(rc).ToLocalChecked());
}

//--------------------------------------------------------
NAN_METHOD(setMissionId)
{
    engageSetMissionId(STRVAL(0));
}

//--------------------------------------------------------
NAN_METHOD(openCertStore)
{
    engageOpenCertStore(STRVAL(0), STRVAL(1));
}

//--------------------------------------------------------
NAN_METHOD(closeCertStore)
{
    engageCloseCertStore();
}

// TODO: engageSetCertStoreCertificatePem
// TODO: engageSetCertStoreCertificatePem
// TODO: engageSetCertStoreCertificateP12
// TODO: engageDeleteCertStoreCertificate
// TODO: engageGetCertStoreCertificatePem
// TODO: engageGetCertificateDescriptorFromPem
// TODO: engageImportCertStoreElementFromCertStore

//--------------------------------------------------------
NAN_METHOD(logMsg)
{
    engageLogMsg(INTVAL(0), STRVAL(1), STRVAL(2));
}

//--------------------------------------------------------
NAN_METHOD(platformNotifyChanges)
{
    engagePlatformNotifyChanges(STRVAL(0));
}

//--------------------------------------------------------
static void internalEngageLoggingHook(int level, const char *tag, const char *message)
{
    if(g_loggingHookFn.empty())
    {
        return;
    }

    CrossThreadCallbackWorker *cbw = getCallback(g_loggingHookFn.c_str());
    if(!cbw)
    {
        return;
    }

    std::vector<CrossThreadCallbackWorker::Parameter*> *params = new std::vector<CrossThreadCallbackWorker::Parameter*>();
    params->push_back(new CrossThreadCallbackWorker::IntParameter(level));
    params->push_back(new CrossThreadCallbackWorker::StringParameter(tag));
    params->push_back(new CrossThreadCallbackWorker::StringParameter(message));
    cbw->enqueue(params);

    cbw->RELEASE_OBJECT_REFERENCE();
}

NAN_METHOD(hookEngineLogging)
{
    g_loggingHookFn = STRVAL(0);

    if(!g_loggingHookFn.empty())
    {
        engageSetLoggingOutputOverride(internalEngageLoggingHook);
    }
    else
    {
        engageSetLoggingOutputOverride(nullptr);
    }
}

//--------------------------------------------------------
NAN_MODULE_INIT(Init)
{
    ENGAGE_BINDING(on);

    ENGAGE_BINDING(setLogLevel);

    ENGAGE_BINDING(enableCallbacks);
    ENGAGE_BINDING(disableCallbacks);

    ENGAGE_BINDING(initialize);
    ENGAGE_BINDING(shutdown);

    ENGAGE_BINDING(start);
    ENGAGE_BINDING(stop);

    ENGAGE_BINDING(createGroup);
    ENGAGE_BINDING(deleteGroup);
    ENGAGE_BINDING(joinGroup);
    ENGAGE_BINDING(leaveGroup);

    ENGAGE_BINDING(beginGroupTx);
    ENGAGE_BINDING(beginGroupTxAdvanced);
    ENGAGE_BINDING(endGroupTx);
    ENGAGE_BINDING(setGroupRxTag);

    ENGAGE_BINDING(muteGroupRx);
    ENGAGE_BINDING(unmuteGroupRx);

    ENGAGE_BINDING(muteGroupTx);
    ENGAGE_BINDING(unmuteGroupTx);

    ENGAGE_BINDING(setGroupRxVolume);

    ENGAGE_BINDING(queryGroupTimeline);

    ENGAGE_BINDING(updatePresenceDescriptor);

    ENGAGE_BINDING(encrypt);
    ENGAGE_BINDING(decrypt);

    ENGAGE_BINDING(updateLicense);
    ENGAGE_BINDING(getVersion);
    ENGAGE_BINDING(getHardwareReport);
    ENGAGE_BINDING(getNetworkInterfaceDevices);
    ENGAGE_BINDING(getAudioDevices);

    ENGAGE_BINDING(getActiveLicenseDescriptor);
    ENGAGE_BINDING(getLicenseDescriptor);

    ENGAGE_BINDING(openCertStore);
    ENGAGE_BINDING(closeCertStore);

    ENGAGE_BINDING(generateMission);
    ENGAGE_BINDING(generateMissionUsingCertStore);    

    ENGAGE_BINDING(setMissionId);
    ENGAGE_BINDING(platformNotifyChanges);

    ENGAGE_BINDING(logMsg);
    ENGAGE_BINDING(hookEngineLogging);
}

NODE_MODULE(engage, Init)
