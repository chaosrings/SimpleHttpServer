#include "ServeHttp.h"
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>



void ServeHttp::sendStatus(Http::StatusCode code){
	static std::unordered_map<int,std::string> dict{
		{200,"OK"},
		{400,"Bad Request"},
		{401,"Unauthorized"},
		{404,"Not Found"},
		{500,"Internal Server Error"},
		{503,"Server Unavailable"}
	};
	int intcode=static_cast<int>(code);
	std::string message="HTTP/1.1 "+std::to_string(intcode)+" "+dict[intcode];
	this->socket.send(message);
}
bool ServeHttp::serveStatic(HttpRequest& request){
	char cwd_buf[200];
	getcwd(cwd_buf,sizeof(cwd_buf));
	std::string realPath=std::string(cwd_buf)+"/WEBINF"+request.url;
    int fd=open(realPath.c_str(),O_RDONLY,0);
    if(fd==-1)
	{
        sendStatus(Http::StatusCode::NOT_FOUND);
		return false;
	}
    //get file Size
    struct stat filestat;
    if(stat(realPath.c_str(),&filestat)<0){
        sendStatus(Http::StatusCode::INTERNAL_SERVER_ERROR);
		return false;
    }
	unsigned long fileSize=filestat.st_size;
	/*----------------------set response headers-------------------*/
	HttpResponse response;
	response.putHeaderValue("Server","Server by LZL");
	response.putHeaderValue("Connection",request.getHeader("connection"));
	response.putHeaderValue("Content-Length",std::to_string(fileSize));
	static std::unordered_set<std::string> imageTypes{"gif","jpeg","png","jpg"};
	static std::unordered_set<std::string> textTypes{"html","plain","xml"};
	auto reqFileType=request.getRequestFileType();
	if(imageTypes.find(reqFileType)!=imageTypes.end())
	{
		//request img;
		response.putHeaderValue("Content-Type","image/"+reqFileType);
		
	}
	else if(textTypes.find(reqFileType)!=imageTypes.end())
	{
		//request text
		response.putHeaderValue("Content-Type","text/"+reqFileType+";utf-8");
	}
	else
	{
		//default raw binary..
		response.putHeaderValue("Content-Type","application/octet-stream");
	}
    /*----------------------send headers-------------------*/
	sendStatus(Http::StatusCode::OK);
	std::string responseStr=std::move(response.toString());
	this->socket.send(responseStr);
	/*----------------------send body-------------------*/
    void* bytes=static_cast<char*>(mmap(nullptr,fileSize,PROT_READ,MAP_PRIVATE,fd,0));
	this->socket.send(bytes,fileSize);
	/*----------------------close file descriptor and memory unmap -------------------*/
    close(fd);
    munmap(bytes,fileSize);
    
}
bool ServeHttp::serveDynamic(HttpRequest& request,
	const std::unordered_map<std::string,std::function<std::string(HttpRequest&,HttpResponse&)> > &route)
{
	auto methodname=request.getMethod()+":"+request.getUrl();
	auto iter=route.find(methodname);
	if(iter==route.cend())
	{
		//can't find method
		this->sendStatus(Http::StatusCode::NOT_FOUND);
		return false;
	}
	HttpResponse response;
	response.putHeaderValue("Server","Server by LZL");
	response.putHeaderValue("Connection",request.getHeader("connection"));

	//handle iter and set content length
	std::string responseBody=(iter->second)(request,response);
	response.putHeaderValue("Content-Length",std::to_string(responseBody.size()));
	response.putHeaderValue("Content-Type","text/html;utf-8");

	 /*----------------------send status-------------------*/
	sendStatus(Http::StatusCode::OK);
	 /*----------------------send headers-------------------*/
	std::string responseStr=std::move(response.toString());
	this->socket.send(responseStr);
	/*----------------------send body-------------------*/
	this->socket.send(responseBody);
}

bool ServeHttp::process(const std::unordered_map<std::string,std::function<std::string(HttpRequest&,HttpResponse&)> >  &route)
{
	request.timeout=std::chrono::milliseconds(5000);
	do
	{
		if(strbuf.size()<4096)
			strbuf.resize(4096);
		long untreated=this->socket.nonblock_recv(this->strbuf,request.timeout);
		//error occur or client close
		if(untreated<0||untreated==0)
			goto closesock;
		do
		{
			if(request.parserHeader(strbuf,untreated)==false)
			{
				//log message
				sendStatus(Http::StatusCode::BAD_REQUEST);
				goto closesock;
			}
			if(request.setState()==false)
			{
				sendStatus(Http::StatusCode::BAD_REQUEST);
				goto closesock;
			}
			if(request.method=="post")
			{
				//only post may contain body
				if(request.parserBody(strbuf,untreated,this->socket)==false)
				{
					sendStatus(Http::StatusCode::BAD_REQUEST);
					goto closesock;
				}
			}
			if(request.method=="get"&&request.getRequestFileType()!="")
			{
				//request static file
				serveStatic(request);
			}
			else
			{
				//serve dynamic
				serveDynamic(request,route);
			}
		}while(untreated>0&&request.isKeepAlive()==true);
	}while(request.isKeepAlive());
closesock:
	this->socket.close();
	//log message
	return true;
}