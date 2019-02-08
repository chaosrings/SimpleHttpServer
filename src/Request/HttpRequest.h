#pragma once
#include <unordered_map>
#include <string>
#include <chrono>
#include <memory>
#include "../Socket/Socket.h"
#include "../DataVariant/Abstract.h"
class HttpRequest
{
private:
    int flags;
    //get parameters from url  
    bool parserUrl();
public:
    //request headers
    std::unordered_map<std::string,std::string> headers;
    //request params
    std::unordered_map<std::string,std::string> params;
    //request content-type
    std::string contentType;
    //request data
    std::unique_ptr<Abstract> data;
    std::string method;
    std::string url;
    std::string version;
    std::chrono::milliseconds timeout;
    bool keep_alive;
    HttpRequest() noexcept{}
    HttpRequest(const HttpRequest &rhs) noexcept =default;
    HttpRequest(HttpRequest&& rhs) noexcept :headers(std::move(rhs.headers)),params(std::move(rhs.params)),
    data(std::move(rhs.data)),
    method(std::move(rhs.method)),
    url(std::move(rhs.url)),
    version(std::move(rhs.version)),
    timeout(std::move(rhs.timeout)),
    keep_alive(rhs.keep_alive){}
    ~HttpRequest()=default;
    HttpRequest& operator=(const HttpRequest& rhs) noexcept =default;
    HttpRequest& operator=(HttpRequest&& rhs) noexcept=default;

    std::string getUrl(){return this->url;}
    std::string getMethod(){return this->method;}
    std::string getHeader(std::string header)
    {
        auto iter=headers.find(header);
        return iter!=headers.end()? iter->second:"";
    }
    std::string getParameter(std::string key)
    {
        auto iter=params.find(key);
        return iter!=params.end()? iter->second:"";
    }
    bool isKeepAlive(){return keep_alive;}
    std::string getRequestFileType();
    bool parserHeader(std::string &strbuf,long &untreated);
    bool setState();
    bool parserBody(std::string &strbuf,long &untreated,Socket::Socket &socket);
};