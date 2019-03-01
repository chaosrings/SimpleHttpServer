#pragma once
#include <vector>
#include <memory>
#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include "Timer/Timer.h"
#include "Epoll.h"
#include "Channel.h"

class EventLoop
{
  private:
    using Task = std::function<void()>;
    //待处理任务
    std::vector<std::function<void()>> pendingTasks;
    //用于唤醒该Eventloop的channel，其实就是让其从epollwait返回
    std::shared_ptr<Channel> wakeupChannel;
    std::atomic<bool> isQuit;
    std::atomic<bool> isCallingPendingTasks;
    std::shared_ptr<Epoll> poller;
    std::mutex Mutex;
    std::thread::id threadid;
    std::shared_ptr<TimerManager> timerManager;

  private:
    void wakeup();
    void handleRead();
    void doPendingTasks();

  public:
    EventLoop();
    ~EventLoop();
    void loop();
    void quit();
    //接口暴露给eventloop外的线程
    void runTaskInLoopThread(Task &&task);
    //只会在IO线程中被调用
    void addToPoller(Channel *channel, TimerNode::TimeType timeout = 0)
    {
        auto holder = channel->getHolder();
        if (holder)
        {
            //属于serveHttp 的channel
            timerManager->addTimer(holder, timeout);
        }
        poller->add(channel);
    }
    void removeFromPoller(Channel *channel)
    {
        poller->del(channel);
    }
    void updatePoller(Channel *channel)
    {
        auto holder=channel->getHolder();
        poller->mod(channel);
    }

    bool isInLoopThread() { return threadid == std::this_thread::get_id(); }
};