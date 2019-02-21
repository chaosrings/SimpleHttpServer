#include "EventLoop.h"
#include<sys/eventfd.h>


EventLoop::EventLoop(){
    poller.reset(new Epoll());
    isQuit.store(false);
}


void EventLoop::loop()
{
    std::vector<Channel*> ret;
    while(!isQuit.load())
    {
        ret.clear();
        ret=poller->poll();
        for(auto &iter:ret)
            iter->handleEvents();
    }
}

void EventLoop::quit()
{
    isQuit=true;
}
