#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <sys/epoll.h>
#include <functional>

class EventLoop;
class ServeHttp;

class Channel
{
private:
    using CallBack=std::function<void()>;
    EventLoop* loop;
    int fd;
    //关心的事件,epoll根据event来取得活动channel
    uint32_t events;
    //目前的事件，根据revents来调用callback
    uint32_t revents; 

    CallBack readHandler;
    CallBack writeHandler;
    CallBack connHandler;
private:
    void update();
    void handleRead()
    {
        if(readHandler)
            readHandler();
    }
    void handleWrite()
    {
        if(writeHandler)
            writeHandler();
    }
    void handleConn()
    {
        if(connHandler)
            connHandler();
    }
public:
    Channel(EventLoop* _loop):loop(_loop){}
    Channel(EventLoop* _loop,int _fd):loop(_loop),fd(_fd){};
    ~Channel()=default;
    int getFd(){return fd;}
    void setFd(int _fd){fd=_fd;}
   
    void setRevents(uint32_t ev){revents=ev;}
    void setEvents(uint32_t ev){events=ev;}
    uint32_t getEvents(){return events;}

    void enableReading(){events|=EPOLLIN;update();}
    void disableReading(){events&=~EPOLLIN;update();}
    void enableWriting(){events|=EPOLLOUT;update(); }
    void disableWriting(){events&=~EPOLLOUT;update();}
    void disableAll(){events=0;update();}
    
    void setReadHandler(CallBack &&_readHandler){readHandler=_readHandler;}
    void setWriteHandler(CallBack &&_writeHandler){writeHandler=_writeHandler;}
    void setConnHandler(CallBack &&_connHandler){connHandler=_connHandler;}
    void handleEvents();
    
};
