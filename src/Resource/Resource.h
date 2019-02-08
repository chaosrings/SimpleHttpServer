#pragma once
#include <string>
#include <vector>
//request resource 

class Resource{
private:
  std::string absoluteDir;
public:
  //is file exist
  Resource(const std::string absDir):absoluteDir(absDir){};
  Resource(const Resource& rhs)=default;
  ~Resource()=default;
  bool isExist(std::string requestPath);
  std::string getDir(){return absoluteDir;}
};