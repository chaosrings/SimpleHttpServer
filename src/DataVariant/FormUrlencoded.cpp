#include "FormUrlencoded.h"


bool FormUrlencoded::parser(std::string& body_buf){
    using std::string;
    std::string &url=body_buf;
    auto currentPos=0;
    auto splitPos=url.find('&',currentPos);
    for(;splitPos!=string::npos;currentPos=splitPos+1,splitPos=url.find('&',currentPos)){
        auto oneParamStr=url.substr(currentPos,splitPos-currentPos);
        //key=value
        auto keyValSplit=oneParamStr.find('=',0);
        auto key=oneParamStr.substr(0,keyValSplit);
        auto val=oneParamStr.substr(keyValSplit+1);
        params[key]=val;
    }
    //deal with last key=val
    auto keyValSplit=url.find('=',currentPos);
    //no anchor end with key=value
    params[url.substr(currentPos,keyValSplit-currentPos)]=url.substr(keyValSplit+1);
}