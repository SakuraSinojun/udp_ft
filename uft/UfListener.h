
#pragma once

#include "UfDef.h"

#include <string>

class UfPercentListener
{
public:
    virtual void onPercent(const char * filename, __int64 downloaded, __int64 total) = 0;
    
};

class UfGraphicsListener
{
public:
    // 文件下载的图像。。 
    virtual void onGraphic(const char * filename, std::string graphics) {}
};

