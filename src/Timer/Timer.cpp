#include "Timer.h"
#include "Server/HttpServer.h"
#include <sys/time.h>
#include <unistd.h>
#include <queue>

int getNowIntTime()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec;
}

TimerNode::TimerNode(std::shared_ptr<HttpServer> _httpServer, TimeType timeout)
{
    this->httpServer = _httpServer;
    expireTime = getNowIntTime() + timeout;
}

TimerNode::~TimerNode()
{
    //default
}

void TimerNode::updateExpire(int timeout)
{
    auto guard = timerManager;
    if (guard)
    {
        guard->updateExpire(this, timeout);
    }
    else
    {
        this->httpServer.reset();
    }
}

bool TimerNode::isValid()
{
    TimeType nowTime = getNowIntTime();
    if (nowTime < expireTime)
        return true;
    return false;
}

TimerManager::TimerManager()
{
}

TimerManager::~TimerManager()
{
}

void TimerManager::updateExpire(TimerNode* node, TimerNode::TimeType timeout)
{
    TimerNode::TimeType oldExpire = node->getExpireTime();
    if (this->timerContainer.find(oldExpire) != this->timerContainer.end())
    {
        //设置新的过期时间点
        TimerNode::TimeType newExpire = getNowIntTime() + timeout;
        node->setExpireTime(newExpire); 
        //遍历ExpireList找到要删除的节点，并在添加到新的ExpireList
        auto &oldExpireList=timerContainer[oldExpire];
        auto &newExpireList=timerContainer[newExpire];
        for(auto iter=oldExpireList.begin();iter!=oldExpireList.end();++iter)
        {
            if(iter->get()==node)
            {
                newExpireList.push_back(*iter);
                //如果节点中已经没有TimeNode则删除该借节点
                oldExpireList.erase(iter);
                if(oldExpireList.empty())
                    timerContainer.erase(oldExpire);
                break;
            }
        }
    }
    else
    {
        //LOG MESSAGE
    }
}
void TimerManager::addTimer(std::shared_ptr<HttpServer> _httpServer, TimerNode::TimeType timeout)
{
    std::shared_ptr<TimerNode> new_node(new TimerNode(_httpServer, timeout));
    _httpServer->setTimer(new_node);
    new_node->setTimeManager(this);
    timerContainer[new_node->getExpireTime()].push_back(new_node);
}
void TimerManager::handleExpiredEvent()
{
    while (!timerContainer.empty())
    {
        //最小的时间点,map来管理时，按失效时间从小到大排序
        auto oldestIter = timerContainer.begin();
        auto &oldestSet = oldestIter->second;
        assert(!oldestSet.empty());
        if (!(*oldestSet.begin())->isValid())
        {
            //一个set中的计时器是同时失效的，所以只要任意一个失效就删除所有
            timerContainer.erase(oldestIter);
        }
        else
            break;
    }
}