#include "EventLoopThreadPool.h"
#include <assert.h>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* _mainLoop,int _numThread)
:mainLoop(_mainLoop),
numThreads(_numThread),
started(false),
next(0){}

EventLoop* EventLoopThreadPool::getNextLoop()
{
    assert(started);
    EventLoop* res=mainLoop;
    if(!subLoops.empty())
    {
        res=subLoops[next];
        next=(next+1)%numThreads;
    }
    return res;
}

void EventLoopThreadPool::start()
{
    for(int i=0;i<numThreads;++i)
    {
        std::shared_ptr<EventLoopThread> t(new EventLoopThread());
        threads.emplace_back(t);
        subLoops.emplace_back(t->startLoop());
    }
    started=true;
}