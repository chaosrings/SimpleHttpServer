#include "Channel.h"
#include "EventLoop.h"
void Channel::update()
{
    loop->updatePoller(this);
}

void Channel::handleEvents()
{
    if (revents & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
    {
        handleRead();
    }
    if (revents & EPOLLOUT)
    {
        handleWrite();
    }
    else
    {
        handleConn();
    }
}