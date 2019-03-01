#include "application.h"
#include <future>
string getIndexHandle(HttpRequest &request,HttpResponse &response)
{
	string res="<html><body>";
	std::string uidstr=request.getParameter("uid");
	int uid=0;
	if(uidstr!="")
		uid=stoi(uidstr);
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
std::unordered_map<std::string,std::function<std::string(HttpRequest&,HttpResponse&)> >* getRouteMap()
{
    static std::unordered_map<std::string,std::function<std::string(HttpRequest&,HttpResponse&)> > routeMap{
        {"get:/index",getIndexHandle},
		{"post:/index",getIndexHandle}
	};
    return &routeMap;
}
