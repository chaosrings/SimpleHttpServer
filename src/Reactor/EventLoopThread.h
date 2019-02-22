#pragma once
#include "EventLoop.h"
#include <thread>
#include <mutex>
#include <condition_variable>
using std::thread;
using std::mutex;
using std::condition_variable;
class EventLoopThread
{
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop* startLoop();

private:
    void threadFunc();
    EventLoop *loop;
    bool exiting;
    thread workingThread;
    mutex workingMutex;
    condition_variable cond;
};