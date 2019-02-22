#pragma once
#include "Server/HttpRequest.h"
#include "Server/HttpResponse.h"
#include <string>
#include <functional>
using namespace std;

std::unordered_map<std::string,std::function<std::string(HttpRequest&,HttpResponse&)> >* getRouteMap();
