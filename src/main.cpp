#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <atomic>
#include <future>
#include <memory.h>
#include <string>
#include <functional>

#include "Server/ServeHttp.h"
#include "Socket/Socket.h"
#include "Request/HttpRequest.h"
#include "Response/HttpResponse.h"
#include "ThreadPool/threadpool.hpp"
#include "Reactor/EventLoop.h"
using namespace std;

string getIndexHandle(HttpRequest &request,HttpResponse &response)
{
	string res="<html><body>";
	int uid=stoi(request.getParameter("uid"));
	res+="<h1>";
	if(uid==1)
	{
		res+="hello uid:1 ";
	}
	else if(uid==2)
	{
		res+="hello uid:2 you are lucky";
	}
	else
	{
		res+="welcome stranger";
	}
	res+="</h1>";
	res+="</body></html>";
	return res;
}

int main(int argc,char** argv)
{
	std::unordered_map<std::string,std::function<std::string(HttpRequest&,HttpResponse&)> > route;
	route["get:/index"]=getIndexHandle;
	if (argc != 2)
	{
		cout << "usage [] server port" << endl;
		return -1;
	}
	else
	{
		//thread_pool work_pool;
		cout << "init sever" << endl;
		Socket::Socket server;
		if (!server.open())
			cout << "error open" << endl;
		if (!server.bind(atoi(argv[1])))
			cout << "error bind" << endl;
		server.listen();
		EventLoop loop;
		thread t([&loop]()
		{
			loop.loop();
		});
		std::vector<shared_ptr<ServeHttp> > conns; 
		while (true)
		{
			auto client_sock = server.accept();
			if(client_sock.tcp_nodelay()==false)
				break;
			if(client_sock.tcp_nonblock()==false)
				break;
			conns.push_back(make_shared<ServeHttp>(std::move(client_sock),&route,&loop));
			loop.addToPoller(conns.back()->getChannel().get());
		}
		server.close();
	}
	return 0;
}