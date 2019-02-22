#include "EventLoop.h"
#include<unistd.h>
#include<mutex>
#include<sys/eventfd.h>

int createEventfd()
{
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        
        abort();
    }
    return evtfd;
}
EventLoop::EventLoop():
timerManager(new TimerManager()),
poller(new Epoll()),
isQuit(false),
isCallingPendingTasks(false),
threadid(std::this_thread::get_id()),
wakeupChannel(new Channel(this,createEventfd()))
{
    wakeupChannel->setEvents(EPOLLIN);
    wakeupChannel->setReadHandler(std::bind(&EventLoop::handleRead,this));
    poller->add(wakeupChannel.get());
}
EventLoop::~EventLoop()
{
    
}
void EventLoop::handleRead()
{
    uint64_t one=1;
    ssize_t n=::read(wakeupChannel->getFd(),&one,sizeof one);
    if(n!=sizeof one)
    {
        //TODO
    }
}


void EventLoop::wakeup()
{
    //写入wakeup fd以在poll后执行handread
    uint64_t one=1;
    ssize_t n=::write(wakeupChannel->getFd(),&one,sizeof one);
    if(n!=sizeof one)
    {
        //TODO
    }
}
void EventLoop::doPendingTasks()
{
    isCallingPendingTasks.store(true);
    std::vector<Task> localTasks;
    {
        std::lock_guard<std::mutex> guard(Mutex);
        localTasks.swap(pendingTasks);
    }
    for(auto &task:localTasks)
        task();
    isCallingPendingTasks.store(false);
}

void EventLoop::runTaskInLoopThread(Task&& t)
{
    if(isInLoopThread())
    {
        //当前线程是eventloop所属的线程，直接执行
        t();
    }
    else
    {
        //另外的线程安全的将任务放入pendingTask中
        {
            std::lock_guard<std::mutex> lock(Mutex);
            pendingTasks.emplace_back(std::forward<Task>(t));
        }
        //唤醒eventloop所属的线程执行到dopendingTask
        if(!isInLoopThread())
            wakeup();
    }
}

void EventLoop::loop()
{
    std::vector<Channel*> ret;
    while(!isQuit.load())
    {
        ret=poller->poll();
        for(auto &iter:ret)
            iter->handleEvents();
        timerManager->handleExpiredEvent();
        doPendingTasks();
    }
}

void EventLoop::quit()
{
    isQuit=true;
}
