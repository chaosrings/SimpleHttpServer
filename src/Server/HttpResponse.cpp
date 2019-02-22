#include "HttpResponse.h"

std::string HttpResponse::toString(){
    std::string res="";
    for(auto &entry:headers){
       res+=entry.first+":"+entry.second+"\r\n";
    }
    res+="\r\n";
    return res;
}