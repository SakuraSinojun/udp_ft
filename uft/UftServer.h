
#pragma once

// #include "tools/socket.h"
#include "tools/reliable_udp_socket.h"
#include "UfPacket.h"
#include "UfServerNode.h"

#include <list>

class UftServer
{
public:
    UftServer();
    virtual ~UftServer();

    int bind();
    int start(int timeout_ms = 10000);
    // int stop(int state);

    int getSocket() { return mSock.GetSocket(); }

private:
    tools::ReliableUdpSocket    mSock;
    std::list<UfServerNode*>    mClients;
    bool                        mServerRunning;
    // int                         mState;
};

