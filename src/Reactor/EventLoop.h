#pragma once
#include <vector>
#include <memory>
#include <atomic>
#include <functional>
#include <mutex>
#include "Epoll.h"
#include "Channel.h"

class EventLoop
{
private:
    using Task=std::function<void()>;
    //待处理任务
    std::vector<std::function<void()>> pendingTask;
    //有需要处理的事件
    std::shared_ptr<Channel> wakeupChannel;
    //epoll
    std::atomic<bool> isQuit;
    std::shared_ptr<Epoll> poller;
    std::mutex Mutex;
    void wakeup();
    void handleRead();
    void doPendingTasks();
    void handleConn();
public:
    EventLoop();
    ~EventLoop()=default;
    void loop();
    void quit();
     void removeFromPoller(Channel* channel)
    {
        poller->epoll_del(channel);
    }
    void updatePoller(Channel* channel, int timeout = 0)
    {
        poller->epoll_mod(channel, std::chrono::seconds(timeout));
    }
    void addToPoller(Channel* channel, int timeout = 0)
    {
        std::lock_guard<std::mutex> l(Mutex);
        poller->epoll_add(channel,std::chrono::seconds(timeout));
    }
};