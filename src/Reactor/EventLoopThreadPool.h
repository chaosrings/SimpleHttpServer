#pragma once
#include "EventLoopThread.h"

class EventLoopThreadPool
{
public:
    EventLoopThreadPool(EventLoop* _mainLoop, int _numThreads);

    ~EventLoopThreadPool()=default;
    void start();

    EventLoop *getNextLoop();

private:
    EventLoop* mainLoop;
    bool started;
    int numThreads;
    int next;
    std::vector<std::shared_ptr<EventLoopThread>> threads;
    std::vector<EventLoop*> subLoops;
};