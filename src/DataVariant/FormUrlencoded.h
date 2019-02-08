#include "Abstract.h"
#include <unordered_map>
#include <string>
class FormUrlencoded:public Abstract{
private:
    std::unordered_map<std::string,std::string> params;
public:
    FormUrlencoded():Abstract(){}
    bool parser(std::string& body_buf) override;
    std::unordered_map<std::string,std::string> getParams(){
        return params;
    }
};