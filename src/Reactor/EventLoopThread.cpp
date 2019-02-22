#include "EventLoopThread.h"

EventLoopThread::EventLoopThread()
: loop(nullptr),
exiting(false),
workingThread(std::bind(&EventLoopThread::threadFunc, this))
{
}

EventLoopThread::~EventLoopThread()
{
    exiting = true;
    if (loop != nullptr)
    {
        loop->quit();
        workingThread.join();
    }
}

void EventLoopThread::threadFunc()
{
    EventLoop localLoop;
    {
        std::lock_guard<mutex> guard(workingMutex);
        loop=&localLoop;
    }
    cond.notify_one();
    localLoop.loop();
    loop=nullptr;
}

EventLoop* EventLoopThread::startLoop()
{
    std::unique_lock<mutex> guard(workingMutex);
    cond.wait(guard,[this](){return this->loop!=nullptr;});
    return loop;
}