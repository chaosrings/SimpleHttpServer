#include "Epoll.h"
//#include "common.h"

#include <sys/socket.h>
#include <assert.h>
#ifdef DEBUG
#include <unistd.h>
#endif
using std::vector;
Epoll::Epoll()
{
    epollFd = epoll_create1(EPOLL_CLOEXEC);
    events.resize(4096);
}
void Epoll::add(Channel *chan)
{
    int fd = chan->getFd();
    struct epoll_event event;
    event.data.fd = fd;
    event.events = chan->getEvents();
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        //ERROR HANDLE
    }
    fd2chan[fd] = chan;
}
void Epoll::mod(Channel *chan)
{
    int fd = chan->getFd();

    struct epoll_event event;
    event.data.fd = fd;
    event.events = chan->getEvents();

    if (epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &event) < 0)
    {
        //ERROR HANDLE
    }
}
void Epoll::del(Channel *chan)
{
    int fd = chan->getFd();
    struct epoll_event event;
    event.data.fd = fd;
    if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, 0) < 0)
    {
        //ERROR HANDLE
    }
    fd2chan.erase(fd);
}
const int EPOLLWAIT_TIME = 10000;
vector<Channel *> Epoll::poll()
{
    int event_count = epoll_wait(epollFd, &(*events.begin()), events.size(), EPOLLWAIT_TIME);
    if (event_count < 0)
    {
        perror("epoll wait error");
    }
    std::vector<Channel *> activeChans;
    for (int i = 0; i < event_count; ++i)
    {
        int fd = events[i].data.fd;
        Channel *chan = fd2chan[fd];
        if (chan != nullptr)
        {
            //有效
            chan->setRevents(events[i].events);
            activeChans.push_back(chan);
        }
    }
#ifdef DEBUG
    {
        char mes[120];
        auto total = sprintf(mes, "epoll_fd : %d poll active event num : %d total event num : %d\n",this->epollFd,activeChans.size(), fd2chan.size());
        ::write(STDOUT_FILENO, mes, total);
    }
#endif
    return activeChans;
}
