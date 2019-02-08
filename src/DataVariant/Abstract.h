#pragma once 
#include <string>

class Abstract{
public:
    Abstract()=default;
    virtual bool parser(std::string &strbuf)=0;
};