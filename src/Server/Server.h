#include "Reactor/EventLoop.h"
#include "Reactor/Channel.h"
#include "Reactor/EventLoopThreadPool.h"
#include "Socket/Socket.h"
#include <memory>

class Server
{
public:
    Server(EventLoop* _mainLoop,int _threadNum,int _port);
    ~Server(){listenSock.close();}
    void start();
    void handNewConn();
    void handThisConn(){}
private:
    static const int MAXFDS=66666;
    EventLoop* mainLoop;
    int  threadNum;
    int port;
    Socket listenSock;
    int acceptedNum;
    std::unique_ptr<EventLoopThreadPool> eventloopThreadPool;
    std::shared_ptr<Channel> acceptChannel;
};