
#pragma once

// #include "tools/socket.h"
#include "tools/reliable_udp_socket.h"
#include "UfPacket.h"
#include "UfServerNode.h"
#include "UfListener.h"

#include <list>


class UftServer : public UfServerNode::Delegate
{
public:
    UftServer();
    virtual ~UftServer();

    class ServerNodeCreator : public tools::ReliableUdpSocket::ServerSocketCreator {
    public:
        ServerNodeCreator(UftServer* s);
        virtual tools::ReliableUdpSocket::ServerSocket* create();
        virtual void remove(tools::ReliableUdpSocket::ServerSocket* s);
    private:
        ServerNodeCreator();
        UftServer* mUftServer;
    };


    int bind();
    int start(int timeout_ms = 10000);
    // int stop(int state);

    int getSocket() { return mSock.GetSocket(); }

    void setListener(UfPercentListener* listener);

    virtual void onPercent(std::string filename, __int64 downloaded, __int64 total);

private:
    tools::ReliableUdpSocket    mSock;
    std::list<UfServerNode*>    mClients;
    bool                        mServerRunning;
    UfPercentListener*          mPercentListener;
    ServerNodeCreator           mCreator;
    // int                         mState;
};

