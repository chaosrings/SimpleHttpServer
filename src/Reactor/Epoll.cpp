#include "Epoll.h"
#include <sys/socket.h>
#include <assert.h>
using std::vector;
Epoll::Epoll()
{
    epollFd = epoll_create1(EPOLL_CLOEXEC);
    events.resize(4096);
}

void Epoll::epoll_add(Channel *chan, const std::chrono::milliseconds &timeout)
{
    int fd = chan->getFd();
    if (timeout.count() > 0)
    {
        //TODO
    }
    struct epoll_event event;
    event.data.fd = fd;
    event.events = chan->getEvents();

    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        //ERROR HANDLE
    }
    fd2chan[fd] = chan;
}
void Epoll::epoll_mod(Channel *chan, const std::chrono::milliseconds &timeout)
{
    int fd = chan->getFd();
    if (timeout.count() > 0)
    {
        //TODO
    }
    struct epoll_event event;
    event.data.fd = fd;
    event.events = chan->getEvents();

    if (epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &event) < 0)
    {
        //ERROR HANDLE
    }
}
void Epoll::epoll_del(Channel *chan)
{
    int fd = chan->getFd();
    struct epoll_event event;
    event.data.fd = fd;
    if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, 0) < 0)
    {
        //ERROR HANDLE
    }
    fd2chan[fd]=nullptr;
}
const int EPOLLWAIT_TIME = 10000;
vector<Channel*> Epoll::poll()
{
    int event_count = epoll_wait(epollFd, &(*events.begin()), events.size(), EPOLLWAIT_TIME);
    if (event_count < 0)
    {
        perror("epoll wait error");
    }
    std::vector<Channel*> activeChans;
    for (int i = 0; i < event_count; ++i)
    {
        int fd = events[i].data.fd;
        Channel* chan = fd2chan[fd];
        if (chan!=nullptr)
        {
            //有效
            chan->setRevents(events[i].events);
            activeChans.push_back(chan);
        }
    }
    return activeChans;
}

