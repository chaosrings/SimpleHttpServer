#pragma once
#include "Buffer/Buffer.h"
#include "Reactor/Channel.h"
#include "Socket/Socket.h"
#include "HttpStatusCode.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include <string>
#include <functional>
#include <memory>
enum ProcessState
{
    EXPECT_HEADERS = 1,
    INCOMPLETE_HEADERS,
    EXPECT_RECVBODY,
    INCOMPLETE_BODY,
    GET_ALL_REQ,
    FINISH
};
class TimerNode;
class EventLoop;
//enable_shared_from_this 必须被shared_ptr<T>(new T(args....)) 而不是reset
class HttpServer :public std::enable_shared_from_this<HttpServer>
{
  private:
    Socket socket;
    ProcessState state;
    Buffer inbuf;
    Buffer outbuf;
    HttpRequest request;
    HttpResponse response;
    std::unordered_map<std::string, std::function<std::string(HttpRequest &, HttpResponse &)>> *route;
    std::shared_ptr<Channel> channel;
    EventLoop *loop;
    std::weak_ptr<TimerNode> timer;

  private:
    bool serveStatic();
    bool serveDynamic();
    bool parserHeader(std::string &strbuf);
    bool parserUrl(std::string &url);
    bool parserBody(std::string &strbuf);
    void handleRead();
    void handleWrite();
    void handleError(Http::StatusCode code);

  public:
    HttpServer(Socket &&sock, decltype(route) pRoute, EventLoop *pLoop) noexcept;
    std::shared_ptr<Channel> getChannel() { return channel; }
    ~HttpServer();
    //在eventloop归属线程运行
    void setup();
    void setTimer(std::shared_ptr<TimerNode> _timer) { this->timer = _timer; }
    void outStatusCode(Http::StatusCode code);
};