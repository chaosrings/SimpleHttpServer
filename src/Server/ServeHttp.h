#pragma once

#include <string>
#include "HttpStatusCode.h"
#include "../Socket/Socket.h"
#include "../Request/HttpRequest.h"

class ServeHttp
{
private:
    Socket::Socket socket;
    std::string strbuf;
    HttpRequest request;
public:
    ServeHttp(Socket::Socket && sock) noexcept:socket(std::move(sock)){}
    ServeHttp(ServeHttp &&rhs) noexcept 
        :socket(std::move(rhs.socket)),
        strbuf(std::move(rhs.strbuf)),
        request(std::move(rhs.request)){}
    ~ServeHttp(){socket.close();}
    void sendStatus(Http::StatusCode code);
    bool serveStatic(HttpRequest& request);
    bool serveDynamic();
    bool process();
    bool sendStatusCode(Http::StatusCode errorcode);
};