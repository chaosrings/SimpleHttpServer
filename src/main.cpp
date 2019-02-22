#include "Reactor/EventLoop.h"
#include "Server/Server.h"
#include <iostream>

using namespace std;

int main(int argc,char** argv)
{
	
	if (argc != 2)
	{
		cout << "usage [] server port" << endl;
		return -1;
	}
	else
	{
		EventLoop mainloop;
		int threadNum=1;
		Server server(&mainloop,threadNum,atoi(argv[1]));
		server.start();
		mainloop.loop();
	}
	return 0;
}