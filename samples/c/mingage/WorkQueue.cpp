//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

#include "WorkQueue.hpp"

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
