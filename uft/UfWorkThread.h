
#pragma once

#include "tools/socket.h"
#include "tools/lock.h"
#include "tools/thread.h"
#include "tools/timeout.h"
#include "tools/reliable_udp_socket.h"
#include "UfPacket.h"
#include "UfPartitions.h"
#include "UfPieceBuffer.h"
#include "UfFile.h"

#include <sys/time.h>
#include <list>
#include <string>

class UftClient;
class UfWorkThread : public tools::Timeout, public tools::Thread::Delegate, public tools::ReliableUdpSocket::DataReceiver
{
public:
    UfWorkThread();
    virtual ~UfWorkThread();

    int connect(const char * ip, int port);

    typedef enum {
        R_RUNNING = 0,
        R_NORESPONSE, 
        R_SUCCESS,
        R_TRUNCATE,
        R_DISCONNECT,
        R_FAILED,
        R_UNKNOWN,
    } RunState;

    virtual bool BeforeFirstRun(void* arg);
    virtual bool RunOnce(void* arg);

    virtual void parsePacket(const char * buffer, int len);
    virtual void onDataReceived(const char * buffer, int len);

    void Send(UfPacket& packet);
    void Close(RunState reason);

    virtual void checkFile();

    bool isDownloading() { return mDownloading; }

protected:
    tools::ReliableUdpSocket  mSock;
    int             mSid;
    int             mRetryTimes;
    UftClient*      mClient;
    UfPartitions::Range mRange;
    RunState        mRunning;
    UfPieceBuffer   mPieceBuffer;
    bool            mReceivedSendOver;
    int             mPacketCount;
    struct timeval  mStartTime;
    bool            mDownloading;

protected:
    void onTimeoutRequestFileInfo(void* arg);
    void onTimeoutHeartbit(void* arg);
    void onTimeoutWriteFile(void* arg);
    // void onTimeoutFilecontent(void* arg);
    void onTimeoutGetRange(void* arg);
    void onTimeoutRequestNextRange(void* arg);

    virtual void onTimeoutDisconnect(void* arg);
    virtual void onFileInfo(const char * buffer, int len);

    void onFileContent(const char* buffer, int len);
    void onSendOver(const char * buffer, int len);

    virtual void doWritePiecesToBuffer();
    void doClose();
};


