
#pragma once

#include <string>
#include "UfDef.h"
#include "UfListener.h"

class UftServer;
class Uft
{
public:
    Uft();
    ~Uft();

    // type: 0: 255.255.255.255
    // type: 1: default (nat first, then eth0)
    // default port: 3478
    int nat(const char * natserverip, int natserverport, std::string& mapip, int& mapport, int type = 1);
    int bindServer();
    int startServer();
    int requestFile(const char * ip, int port, const char * filename, const char * localfilename, int max_thread_count);
    void setListener(UfPercentListener* listener);
    void setListener(UfGraphicsListener* listener);

private:
    UftServer* s;
    int nat1(std::string& mapip, int& mapport);

    UfPercentListener* mPercentListener;
    UfGraphicsListener* mGraphicsListener;
};

