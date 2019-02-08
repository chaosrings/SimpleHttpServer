#include "Resource.h"
#include <unistd.h>

bool Resource::isExist(std::string requestPath){
    auto realPath=absoluteDir+"/"+requestPath;
    if(access(realPath.c_str(),F_OK|R_OK)!=-1)
        return true;
    return false;
}