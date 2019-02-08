#include "HttpRequest.h"
#include "../Utils/Utils.h"
#include "../DataVariant/FormUrlencoded.h"
#include "../DataVariant/TextPlain.h"

#include <iostream>

std::string HttpRequest::getRequestFileType(){
	int dot_pos=-1;
	for(int i=static_cast<int>(url.size()-1);i>=0;--i){
		if(url[i]=='.')
		{
			dot_pos=i;
			break;
		}
	}
	if(dot_pos!=0){
		return url.substr(dot_pos+1);
	}
	return "";
}


bool HttpRequest::parserUrl(){
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
bool HttpRequest::parserHeader(std::string & strbuf,long &untreated){
   using std::string;
	if (strbuf.empty())
		return false;
	//end of http headers
	size_t headers_end = strbuf.find("\r\n\r\n");
	//headers_end+=2 to end loop(cur_pos!=header_end)
	headers_end+=2;
	if (headers_end == std::string::npos)
		return false;
	//METHOD URL VERSION\r\n
	size_t cur_pos = 0;
	//end of one line
	size_t line_end = strbuf.find("\r\n");
	if (line_end == string::npos)
		return false;
	strbuf[line_end] = '\0';
	size_t method_end = strbuf.find(' ',cur_pos);
	if (method_end== string::npos)
		return false;
	method = Utils::StringToLower(std::move(strbuf.substr(cur_pos, method_end - cur_pos)));
	
	cur_pos = method_end + 1;
	size_t url_end = strbuf.find(' ', cur_pos);
	if (url_end == string::npos)
        return false;
	url = strbuf.substr(cur_pos, url_end - cur_pos);


	cur_pos = url_end + 6;
	// HTTP/x.x
	size_t version_end = line_end;
	if (cur_pos>version_end)
		return false;
	version = strbuf.substr(cur_pos, version_end - cur_pos);

	//skip \r\n and set cur_pos begin of a new line
	cur_pos = line_end + 2;
	line_end = strbuf.find("\r\n", cur_pos);
	strbuf[line_end] = 0;
	for (; cur_pos != headers_end; cur_pos = line_end + 2, line_end = strbuf.find("\r\n", cur_pos))
	{
		auto headername_end = strbuf.find(':', cur_pos);
		string header_name = Utils::StringToLower(std::move(strbuf.substr(cur_pos, headername_end - cur_pos)));
		string header_value = Utils::StringToLower(std::move(strbuf.substr(headername_end + 1, line_end - (headername_end + 1))));
		header_value = Utils::trim(std::move(header_value));
		//move instead of copy
		headers.emplace(std::move(header_name), std::move(header_value));
	}
	#ifdef DEBUG
	{
		std::cout<<"----------------------it is headers------------------------"<<std::endl;
		//debug cout
		std::cout<<method<<" "<<url<<" "<<"HTTP/"<<version<<std::endl;
        //cout headers
		for(auto &entry:headers)
		{
			std::cout<<entry.first<<":"<<entry.second<<std::endl;
		}
		std::cout<<"current buf size is "<<strbuf.size()<<std::endl;
	}
	#endif
	//erase space used by current request's headers
	strbuf.erase(0, headers_end + 2);
	untreated-=(headers_end+2);
    //parser url
    if(method=="get")
        parserUrl();
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

bool HttpRequest::parserBody(std::string &strbuf,long &untreated,Socket::Socket &socket){
	auto iter=headers.find("content-type");
	
	if(iter!=headers.end())
	{
		//set content Type;
		contentType=iter->second;
	}else{
		return false;
	}
	long body_length=0;
	iter=headers.find("content-length");
	if(iter!=headers.end())
	{
		body_length=std::stol(iter->second);
	}
	else
		false;
	//body_buf: buffer for request body 
	std::string body_buf;
	//recv_total: count for request body recived
	long recv_total=0;
	if(body_length<strbuf.size())
	{
		//body_length smaller than strbuf, strbuf contains all data
		//move request body from strbuf to body_buf
		body_buf.assign(strbuf,0,body_length);
		strbuf.erase(0,body_length);
		untreated-=body_length;
	}
	else
	{
		//body_length bigger than strbuf,strbuf contains part of data
		recv_total=strbuf.length();
		//move strbuf to body_buf and reset strbuf
		body_buf=move(strbuf);
		untreated=0;
		strbuf.resize(4096);
	}
	recv_total=static_cast<long>(body_buf.size());
	body_buf.resize(body_length);
	void * raw_body_buf=reinterpret_cast<void*>(&(*(body_buf.begin()+recv_total)));
	bool process_result=true;
	while(recv_total<body_length)
	{
		long recv_size=socket.nonblock_recv(raw_body_buf,body_length-recv_total,timeout);
		if(recv_size<=0)
		{
			process_result=false;
			break;
		}
		recv_total+=recv_size;
	}
	//switch case?
	std::string contentType=headers["content-type"];
	if(contentType=="application/x-www-form-urlencoded"){
		data.reset(new FormUrlencoded());
		data->parser(body_buf);
		this->params=dynamic_cast<FormUrlencoded*>(data.get())->getParams();
	}
	else if (contentType=="text/plain"){
		data.reset(new TextPlain());
		data->parser(body_buf);
	}else{
		//multipart/form-data
	}
	//debug cout
	#ifdef DEBUG
	{
		std::cout<<"----------------------it is params------------------------"<<std::endl;
		if(params.size()!=0)
		{
			for(auto& entry:params){
				std::cout<<entry.first<<" = "<<entry.second<<std::endl;
			}
		}
		else{
			std::cout<<body_buf<<std::endl;
		}
	}
	#endif
	if(process_result==false)
        return false;
	return true;
}