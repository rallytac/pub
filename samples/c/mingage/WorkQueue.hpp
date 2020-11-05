//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

#ifndef WorkQueue_hpp
#define WorkQueue_hpp

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

    Lambda *createLambda(std::function<void()> r);

    void dispatcher();
    void clear();
    void returnToPool(Lambda *l);
};

#endif /* WorkQueue_hpp */
