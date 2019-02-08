#pragma once
#include <string>
#include <vector>
#include <ctype.h>

//#define DEBUG
namespace Utils{
    std::string StringToLower(std::string str)
    {
        for(auto &ch:str)
            ch=tolower(ch);
        return str;
    }
    std::string trim(std::string str)
    {
        std::string res="";
        for(auto ch:str)
        {
            if(ch==' '||ch=='\n'||ch=='\t')
                continue;
            else
            {
                res+=ch;
            }
        }
        return res;
    }

    bool strEndWith(const std::string& src,const std::string& suffix){
        int src_i=src.size()-1;
        if(src_i<suffix.size()-1)
            return false;
        for(int i=static_cast<int>(suffix.size()-1);i>=0;--i,--src_i){
            if(suffix[i]!=src[src_i])
                return false;
        }
        return true;
    }
}