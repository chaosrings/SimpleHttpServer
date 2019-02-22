#pragma once
#include "Server/HttpServer.h"
#include <unistd.h>
#include <memory>
#include <queue>
#include <chrono>
#include <map>
#include <list>
class TimerManager;
class TimerNode
{
public:
    using TimeType=int;
    TimerNode(std::shared_ptr<HttpServer> sp, TimeType timeout);
    ~TimerNode();
   
    void updateExpire(TimeType timeout);
    bool isValid();
    void setExpireTime(int expire){expireTime=expire;} 
    TimeType getExpireTime() const { return expireTime; } 
    void setTimeManager(TimerManager* manager){this->timerManager=manager;}
private:
    TimeType expireTime;
    TimerManager* timerManager;
    std::shared_ptr<HttpServer> httpServer;
};

class TimerManager 
{
public:
    using ContainerType= std::map<TimerNode::TimeType,std::list<std::shared_ptr<TimerNode>>>;
    TimerManager();
    ~TimerManager();
    void addTimer(std::shared_ptr<HttpServer> HttpServer, int timeout);
    void updateExpire(TimerNode* node,int timeout);
    void handleExpiredEvent();

private:
    ContainerType timerContainer;    
};