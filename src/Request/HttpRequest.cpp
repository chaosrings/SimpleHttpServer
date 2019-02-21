#include "HttpRequest.h"
#include "Utils/Utils.h"
#include "DataVariant/FormUrlencoded.h"
#include "DataVariant/TextPlain.h"

#include <iostream>

std::string HttpRequest::getRequestFileType(){
	int dot_pos=-1;
	for(int i=static_cast<int>(url.size()-1);i>=0&&url[i]!='/';--i){
		if(url[i]=='.')
		{
			dot_pos=i;
			break;
		}
	}
	if(dot_pos!=-1&&url[dot_pos]!='/'){
		return url.substr(dot_pos+1);
	}
	return "";
}


bool HttpRequest::setParamsFromUrl(){
    using std::string;
    size_t routeEnd=url.find('?');
    if(routeEnd==string::npos&&headers.find("url")==headers.end()){
        //only contain /url
        //do nothing and return success
        return true;
    }else{
        // url=`/routeUrl?{key1=val1&}key2=val2`
        auto routeUrl=url.substr(0,routeEnd);
        auto currentPos=routeEnd+1;
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
        auto anchorPos=url.find('#',currentPos);
        if(anchorPos==string::npos)
        {
            //no anchor end with key=value
            params[url.substr(currentPos,keyValSplit-currentPos)]=url.substr(keyValSplit+1);
        }else{
            //end with key=value#name
             params[url.substr(currentPos,keyValSplit-currentPos)]=url.substr(keyValSplit+1,anchorPos-keyValSplit-1);
        }
        url=std::move(routeUrl);
    }
    return true;
}

bool HttpRequest::setState(){
	auto iter=headers.find("connection");
	if(iter!=headers.end())
	{
		if(iter->second=="keep-alive")
		{
			this->keep_alive=true;
		}
		else if(iter->second=="close")
		{
			this->keep_alive=false;
		}
		else
		{
			return false;
		}
	}
	return true;	
}
