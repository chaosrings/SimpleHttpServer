#pragma once
#include "Channel.h"
#include <chrono>
#include <vector>
#include <unordered_map>
#include <memory>
#include <sys/epoll.h>

using sp_channel=std::shared_ptr<Channel>;
class Epoll
{
private:
    static const int MAXFDS=100000;
    int epollFd;
    std::vector<epoll_event> events;
    std::unordered_map<int,Channel *> fd2chan; 
public:
    Epoll();
    ~Epoll()=default;
    std::vector<sp_channel> getEventsRequest(int event_count);
    void add(Channel *chan);
    void mod(Channel *chan);
    void del(Channel *chan);

    std::vector<Channel *> poll();
};
