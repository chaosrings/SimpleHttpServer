#pragma once
#include <string>
#include <functional>
#include "HttpStatusCode.h"
#include "Socket/Socket.h"
#include "Request/HttpRequest.h"
#include "Response/HttpResponse.h"
#include "ThreadPool/threadpool.hpp"
#include "Buffer/Buffer.h"
#include "Reactor/Channel.h"
#include "Reactor/EventLoop.h"
enum ProcessState
{
    EXPECT_HEADERS=1,
    INCOMPLETE_HEADERS,
    EXPECT_RECVBODY,
    INCOMPLETE_BODY,
    GET_ALL_REQ,
    FINISH
};

class ServeHttp
{
private:
    Socket::Socket socket;
    ProcessState state;
    Buffer inbuf;
    Buffer outbuf;
    HttpRequest request;
    HttpResponse response;
    std::unordered_map<std::string,std::function<std::string(HttpRequest&,HttpResponse&)>> *route;
    std::shared_ptr<Channel> channel;
    EventLoop* loop;
public:
    ServeHttp(
    Socket::Socket && sock,
    decltype(route) pRoute,
    EventLoop* pLoop
    ) noexcept:socket(std::move(sock)){
        route=pRoute;
        channel.reset(new Channel(pLoop,socket.get_fd()));
        state=ProcessState::EXPECT_HEADERS;
        request.keep_alive=false;
        channel->setReadHandler(std::bind(&ServeHttp::handleRead,this));
        channel->setWriteHandler(std::bind(&ServeHttp::handleWrite,this));
        channel->enableReading();
    }
    sp_channel getChannel(){return channel;}

    //ServeHttp(ServeHttp &&rhs) =default;
    ~ServeHttp(){socket.close();}
    void outStatusCode(Http::StatusCode code);
    bool serveStatic();
    bool serveDynamic();

    bool parserHeader(std::string &strbuf);
    bool parserUrl(std::string& url);
    bool parserBody(std::string& strbuf);
    
    void handleRead();
    void handleWrite();
    void handleError(Http::StatusCode code);
};