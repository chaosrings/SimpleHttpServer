#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
class HttpResponse
{
private:
  std::vector<std::pair<std::string,std::string> > headers;
public:
    HttpResponse()=default;
    ~HttpResponse()=default;
    HttpResponse& operator=(const HttpResponse& rhs)=default;
    void putHeaderValue(std::string header,std::string value)
    {
      headers.push_back(std::make_pair(header,value));
    }
    std::string toString();
};