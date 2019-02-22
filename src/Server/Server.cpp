#include "Server.h"
#include "HttpServer.h"
#include "App/application.h"
#include <iostream>
static const int DEFAULT_TIMEOUT = 2;

Server::Server(EventLoop* _mainLoop,int _threadNum,int _port):
mainLoop(_mainLoop),
threadNum(_threadNum),
port(_port),
acceptedNum(0)
{
    if(listenSock.open()==false)
    {
        std::cout<<"error open\n";
        exit(-1);
    }
    if(listenSock.bind(static_cast<uint16_t>(port))==false)
    {
        std::cout<<"error bind\n";
        exit(-1);
    }
    if(listenSock.listen()==false)
    {
        std::cout<<"error open\n";
        exit(-1);
    }
    eventloopThreadPool.reset(new EventLoopThreadPool(_mainLoop,_threadNum));
    acceptChannel.reset(new Channel(_mainLoop,listenSock.get_fd()));
}

void Server::start()
{
    eventloopThreadPool->start();
    acceptChannel->setEvents(EPOLLIN);
    acceptChannel->setReadHandler(std::bind(&Server::handNewConn,this));
    acceptChannel->setConnHandler(std::bind(&Server::handThisConn,this));
    mainLoop->addToPoller(acceptChannel.get());
}

void Server::handNewConn()
{
    Socket clientSock=listenSock.accept();
    if(acceptedNum>MAXFDS)
    {
        clientSock.close();
        return;
    }
    clientSock.tcp_nodelay();
    clientSock.tcp_nonblock();
    EventLoop* subLoop=eventloopThreadPool->getNextLoop();
    std::shared_ptr<HttpServer> httpServer(new HttpServer(std::move(clientSock),getRouteMap(),subLoop));
    subLoop->runTaskInLoopThread(std::bind(&HttpServer::setup,httpServer));
}