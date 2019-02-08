#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <atomic>
#include <future>
#include <memory.h>
#include <string>

#include "Server/ServeHttp.h"
#include "Socket/Socket.h"
#include "Request/HttpRequest.h"
#include "ThreadPool/threadpool.hpp"
using namespace std;

void testClient(char*  dotip,uint16_t port)
{
	int clientsock_fd = ::socket(PF_INET, SOCK_STREAM, 0);
	
	sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_addr.s_addr = inet_addr(dotip);
	serveraddr.sin_family = AF_INET;	
	serveraddr.sin_port = htons(port);;
	cout << "connet "<<dotip<<" : " << port << endl;
	if (::connect(clientsock_fd, reinterpret_cast<struct sockaddr*>(&serveraddr), sizeof(serveraddr)) ==-1)
		cout << "connet error" << endl;
	std::string tosend = "GET /index.html HTTP/1.1\r\n"\
	"User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:61.0) Gecko/20100101 Firefox/61.0\r\n"\
	"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"\
	"Accept-Language: en-GB,en;q=0.5\r\n"\
	"Accept-Encoding: gzip, deflate\r\n"\
	"Referer: http://hub.hust.edu.cn/index.jsp\r\n";
	int count = 0;
	vector<char> recv_buf(4096);
	while (count<3)
	{
		cout << "send " << count + 1 << endl;
		::send(clientsock_fd, reinterpret_cast<const void*>(tosend.data()), tosend.size(), 0);
		++count;
		//std::this_thread::sleep_for(std::chrono::milliseconds(2500));
	}
	close(clientsock_fd);
}


int main(int argc,char** argv)
{
	if (argc != 2)
	{
		cout << "usage [] server port" << endl;
		return -1;
	}
	else if (argc == 3)
	{
		thread_pool work_pool;
		cout << "init sever" << endl;
		Socket::Socket server;
		if (!server.open())
			cout << "error open" << endl;
		if (!server.bind(atoi(argv[1])))
			cout << "error bind" << endl;
		server.listen();
		while (true)
		{
			auto client_sock = server.accept();
			auto fut=work_pool.sumbit([&client_sock]()
			{
				ServeHttp serve(std::move(client_sock));
				bool success=serve.process();
				#ifdef DEBUG
					std::cout<<"thread "<<std::this_thread::get_id()<<" process success"<<endl;
				#endif
				return success? 0:-1;
			});
		}
		server.close();
	}
	return 0;
}