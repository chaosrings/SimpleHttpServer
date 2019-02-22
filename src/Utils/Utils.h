#pragma once
#include <string>
#include <vector>
#include <ctype.h>

namespace Utils{
    std::string StringToLower(std::string str);
    std::string trim(std::string str);
    bool strEndWith(const std::string& src,const std::string& suffix);
}