#include "HttpServer.h"
#include "Utils/Utils.h"
#include "DataVariant/TextPlain.h"
#include "DataVariant/FormUrlencoded.h"
#include "Timer/Timer.h"
#include "Reactor/EventLoop.h"

#include "common.h"

#include <future>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
static const int DEFAULT_TIMEOUT = 5;

HttpServer::HttpServer(Socket &&sock, decltype(route) pRoute, EventLoop *pLoop) noexcept : socket(std::move(sock)),
																						   route(pRoute),
																						   loop(pLoop),
																						   channel(new Channel(pLoop, socket.get_fd())),
																						   state(ProcessState::EXPECT_HEADERS)
{
	request.keep_alive = false;
	channel->setReadHandler(std::bind(&HttpServer::handleRead, this));
	channel->setWriteHandler(std::bind(&HttpServer::handleWrite, this));
}
HttpServer::~HttpServer()
{
#ifdef DEBUG
	std::string logMessage = this->socket.getMessage() + " finished,disconnection\n";
	::write(STDOUT_FILENO, logMessage.data(), logMessage.size());
#endif
	socket.close();
}
void HttpServer::setup()
{
	channel->setHolder(shared_from_this());
	channel->enableReading();
	loop->addToPoller(channel.get(), DEFAULT_TIMEOUT);
}

void HttpServer::outStatusCode(Http::StatusCode code)
{
	static std::unordered_map<int, std::string> dict{
		{0, "Empty"},
		{200, "OK"},
		{400, "Bad Request"},
		{401, "Unauthorized"},
		{404, "Not Found"},
		{500, "Internal Server Error"},
		{503, "Server Unavailable"}};
	int intcode = static_cast<int>(code);
	std::string message = "HTTP/1.1 " + std::to_string(intcode) + " " + dict[intcode] + "\r\n";
	outbuf.append(message);
}
bool HttpServer::serveStatic()
{
	char cwd_buf[200];
	getcwd(cwd_buf, sizeof(cwd_buf));
	std::string realPath = std::string(cwd_buf) + "/WEBINF" + request.url;
	int fd = open(realPath.c_str(), O_RDONLY, 0);
	if (fd == -1)
	{
		handleError(Http::StatusCode::NOT_FOUND);
		return false;
	}
	//get file Size
	struct stat filestat;
	if (stat(realPath.c_str(), &filestat) < 0)
	{
		handleError(Http::StatusCode::INTERNAL_SERVER_ERROR);
		return false;
	}
	unsigned long fileSize = filestat.st_size;
	/*----------------------set response headers-------------------*/
	HttpResponse response;
	response.putHeaderValue("Server", "Server by LZL");
	response.putHeaderValue("Connection", request.getHeader("connection"));
	response.putHeaderValue("Content-Length", std::to_string(fileSize));
	static std::unordered_set<std::string> imageTypes{"gif", "jpeg", "png", "jpg"};
	static std::unordered_set<std::string> textTypes{"html", "plain", "xml", "css"};
	auto reqFileType = request.getRequestFileType();
	if (imageTypes.find(reqFileType) != imageTypes.end())
	{
		//request img;
		response.putHeaderValue("Content-Type", "image/" + reqFileType);
	}
	else if (textTypes.find(reqFileType) != imageTypes.end())
	{
		//request text
		response.putHeaderValue("Content-Type", "text/" + reqFileType + ";utf-8");
	}
	else if (reqFileType == "js")
	{
		response.putHeaderValue("Content-Type", "application/javascript");
	}
	else
	{
		//default raw binary..,流式传输，5分钟过期
		response.putHeaderValue("Content-Type", "application/octet-stream");
		this->timer.lock()->updateExpire(60*5);
	}
	/*----------------------out status code-------------------*/
	outStatusCode(Http::StatusCode::OK);
	/*----------------------out headers--------------------*/
	std::string headerStr = std::move(response.toString());
	outbuf.append(headerStr);
	/*----------------------out body-------------------*/
	void *bytes = mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
	outbuf.append(bytes, fileSize);
	/*----close file descriptor and memory unmap----*/
	close(fd);
	munmap(bytes, fileSize);
	return true;
}
bool HttpServer::serveDynamic()
{
	auto methodname = request.getMethod() + ":" + request.getUrl();
	auto iter = route->find(methodname);
	if (iter == route->end())
	{
		//can't find method
		handleError(Http::StatusCode::NOT_FOUND);
		return false;
	}
	HttpResponse response;
	response.putHeaderValue("Server", "Server by LZL");
	response.putHeaderValue("Connection", request.getHeader("connection"));
	//handle iter and set content length
	std::string body_str = (iter->second)(request, response);
	response.putHeaderValue("Content-Length", std::to_string(body_str.size()));
	response.putHeaderValue("Content-Type", "text/html;utf-8");
	/*----------------------send status-------------------*/
	outStatusCode(Http::StatusCode::OK);
	/*----------------------send headers----------------*/
	std::string headers_str = std::move(response.toString());
	outbuf.append(headers_str);
	/*----------------------send body-----------------*/
	outbuf.append(body_str);
	return true;
}

void HttpServer::handleError(Http::StatusCode code)
{
	outStatusCode(code);
	string mes = outbuf.retrieveAllAsString() + "\r\n";
	socket.send(mes);
	handleClose();
}

bool HttpServer::parserHeader(std::string &strbuf)
{
	using std::string;
	if (strbuf.empty())
		return false;
	//end of http headers
	size_t headers_end = strbuf.size() - 4;
	//headers_end+=2 to end loop(cur_pos!=header_end)
	headers_end += 2;
	if (headers_end == std::string::npos)
		return false;
	//METHOD URL VERSION\r\n
	size_t cur_pos = 0;
	//end of one line
	size_t line_end = strbuf.find("\r\n");
	if (line_end == string::npos)
		return false;
	strbuf[line_end] = '\0';
	size_t method_end = strbuf.find(' ', cur_pos);
	if (method_end == string::npos)
		return false;
	request.method = Utils::StringToLower(std::move(strbuf.substr(cur_pos, method_end - cur_pos)));

	cur_pos = method_end + 1;
	size_t url_end = strbuf.find(' ', cur_pos);
	if (url_end == string::npos)
		return false;
	request.url = strbuf.substr(cur_pos, url_end - cur_pos);

	cur_pos = url_end + 6;
	// HTTP/x.x
	size_t version_end = line_end;
	if (cur_pos > version_end)
		return false;
	request.version = strbuf.substr(cur_pos, version_end - cur_pos);

	//skip \r\n and set cur_pos begin of a new line
	cur_pos = line_end + 2;
	line_end = strbuf.find("\r\n", cur_pos);
	strbuf[line_end] = 0;
	for (; cur_pos != headers_end; cur_pos = line_end + 2, line_end = strbuf.find("\r\n", cur_pos))
	{
		auto headername_end = strbuf.find(':', cur_pos);
		string header_name = Utils::StringToLower(std::move(strbuf.substr(cur_pos, headername_end - cur_pos)));
		string header_value = Utils::StringToLower(std::move(strbuf.substr(headername_end + 1, line_end - (headername_end + 1))));
		header_value = Utils::trim(std::move(header_value));
		//move instead of copy
		request.headers.emplace(std::move(header_name), std::move(header_value));
	}
	//parser url
	if (request.method == "get")
		return request.setParamsFromUrl();

	return true;
}

bool HttpServer::parserBody(std::string &strbuf)
{
	std::string contentType = request.getHeader("content-type");
	if (contentType == "")
	{
		return true;
	}
	bool process_result = true;
	if (contentType == "application/x-www-form-urlencoded")
	{
		request.data.reset(new FormUrlencoded());
		process_result = request.data->parser(strbuf);
		request.params = dynamic_cast<FormUrlencoded *>(request.data.get())->getParams();
	}
	else if (contentType == "text/plain")
	{
		request.data.reset(new TextPlain());
		process_result = request.data->parser(strbuf);
	}
	else
	{
		//TODO multipart/form-data
	}
	return process_result;
}
void HttpServer::handleRead()
{
	//超时返回
	auto guard = timer.lock();
	if (guard)
		guard->updateExpire(DEFAULT_TIMEOUT);
	else
		return;
	int saveError = 0;
	do
	{
		if (inbuf.empty() || state == ProcessState::INCOMPLETE_HEADERS || state == ProcessState::INCOMPLETE_BODY)
		{
			//为空或者接受不完全
			ssize_t n = inbuf.readFd(this->socket.get_fd(), &saveError);
			if (n <= 0)
			{
				//立即失效
				guard->updateExpire(0);
				return;
			}
		}
		if (this->state == ProcessState::EXPECT_HEADERS || this->state == ProcessState::INCOMPLETE_HEADERS)
		{
			auto header_end = inbuf.findCRLFCRLF();
			if (header_end != nullptr)
			{
				//完全接收头部信息
				std::string header_str = std::move(inbuf.retrieveAsString(header_end + 4 - inbuf.peek()));
				bool result = parserHeader(header_str);
				request.setState();
				state = ProcessState::EXPECT_RECVBODY;
			}
			else
			{
				state = ProcessState::INCOMPLETE_HEADERS;
				//跳出循环，下一次read event再处理
				break;
			}
		}
		if (this->state == ProcessState::EXPECT_RECVBODY || this->state == ProcessState::INCOMPLETE_BODY)
		{
			size_t body_len = request.getBodyLength();
			if (body_len == 0)
			{
				state = ProcessState::GET_ALL_REQ;
			}
			else if (body_len > 0 && inbuf.readableBytes() >= body_len)
			{
				std::string body_str = std::move(inbuf.retrieveAsString(body_len));
				parserBody(body_str);
				state = ProcessState::GET_ALL_REQ;
			}
			else
			{
				state = ProcessState::INCOMPLETE_BODY;
				//跳出循环，下一次read event再处理
				break;
			}
		}
		if (this->state == ProcessState::GET_ALL_REQ)
		{
			if (request.method == "get" && request.getRequestFileType() != "")
				serveStatic();
			else
				serveDynamic();
			//处理完成，判断keepalive，为真循继续处理下一个请求
			if (request.isKeepAlive())
			{
				state = ProcessState::EXPECT_HEADERS;
			}
			else
			{
				state = ProcessState::FINISH;
				handleClose();
			}
			//允许写response
			channel->enableWriting();
		}
	} while (state != ProcessState::FINISH && !inbuf.empty());
}

void HttpServer::handleWrite()
{
	ssize_t n = ::write(socket.get_fd(), outbuf.peek(), outbuf.readableBytes());
	if (n > 0)
	{
		outbuf.retrieve(n);
#ifdef DEBUG
		std::string mes = this->socket.getMessage() + " write " + std::to_string(n) +
						  " bytes, there is " + std::to_string(outbuf.readableBytes()) + " bytes left\n";
		::write(STDOUT_FILENO, mes.data(), mes.size());
#endif
		if (outbuf.readableBytes() == 0)
		{
			//写完，取消监听out事件,默认失效
			channel->disableWriting();
			this->timer.lock()->updateExpire(DEFAULT_TIMEOUT);
		}
		else
		{
			//未写完
			channel->enableWriting();
		}
	}
}

void HttpServer::handleClose()
{
	//拥有的socket只能当HttpServer被析构的时候才关闭
	//这里将时间设置为立即超时，HttpServer将会在EventLoop的事件handle后被析构
	auto guard = timer.lock();
	if (guard)
		guard->updateExpire(-1);
	else
		return;
}