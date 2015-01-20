
#pragma once

#include "UfPacket.h"
#include "tools/timeout.h"
#include "UfFile.h"
// #include "tools/socket.h"
#include "tools/reliable_udp_socket.h"

#include <list>

class UfServerNode : public tools::ReliableUdpSocket::ServerSocket // tools::Timeout
{
public:
    class Delegate {
    public:
        virtual void onPercent(std::string filename, __int64 downloaded, __int64 total) = 0;
    };

    UfServerNode(Delegate& d);

    virtual void onDataReceived(const char * buffer, int len);
    virtual void onClose();

    bool isValid() { return mValid; }
    int  sid() { return mSid; }

    int  makeSid();

    bool Send(UfPacket& packet, bool force = false);

    void onWritable();

private:
    int     mSid;
    char    remoteIp[100];
    int     remotePort;
    bool    mValid;
    UfFile  mFile;
    Delegate&   mDelegate;

    class FilePart {
    public:
        __int64 offset;
        __int64 length;
        __int64 piecelength;
    };

    std::list<FilePart> mSendList;

private:

    int mRetryTimes;
    int mClientPieceBuffer;
    __int64 mClientPieceLength;

    void Close();

    // 心跳
    void toHeartbit(void* arg);

    // 断线
    void toDisconnect(void* arg);

    // File
    void toSendFile(void* arg);

    // 等待SendOver的ack
    // void toWaitingSendOverAck(void* arg);

};










