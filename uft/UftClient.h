
#pragma once

#include "UfPartitions.h"
#include "UfPieceBuffer.h"
#include "UfFile.h"
#include "UfThreadPool.h"
#include "UfMessenger.h"
#include "UfPacket.h"
#include "UfListener.h"

#include "tools/lock.h"
// #include "tools/socket.h"
#include "tools/timeout.h"

#include <sys/time.h>
#include <list>
#include <string>


class UftClient : public UfWorkThread
{
public:
    UftClient();
    virtual ~UftClient();

    void setListener(UfPercentListener* listener);
    void setListener(UfGraphicsListener* listener);
    int requestFile(const char * ip, int port, const char * filename, const char * localfilename, int thread_count = 10);

    // TODO
    bool getRange(UfPartitions::Range& range);
    void putRange(UfPartitions::Range& range);

    bool WriteFile(__int64 offset, const char * content, __int64 length);
    void SendOver();

    std::string             mIp;
    int                     mPort;
    __int64                 mPieceLength;
    UfFile                  mRemoteFile;

private:
    std::list<UfPartitions::Range>  mRanges;
    int                     mRetryTimes;
    int                     mThreadCount;
    UfThreadPool            mPool;
    tools::CLock            mRangeLock;
    UfPartitions            mPartitions;
    UfFile                  mLocalFile;
    UfFile                  mPartFile;
    UfPieceBuffer           mFileBuffer;
    tools::CLock            mFileBufferLock;
    UfPercentListener *     mPercentListener;
    UfGraphicsListener *    mGraphicsListener;
    int                     mMd5ErrorTimes;

private:

    // timeout 
    void onTimeoutRequestFileInfo(void* arg);
    void onTimeoutWriteFile(void* arg);
    void onTimeoutSendOver(void* arg);
    void onTimeoutRequestNextRange(void* arg);
    void onTimeoutThreadPoolWatcher(void* arg);

    // packet
    virtual void onFileInfo(const char* buffer, int len);

    // Override 
    // virtual void parsePacket(const char * buffer, int len);
    virtual void onTimeoutDisconnect(void* arg);
    virtual void doWritePiecesToBuffer();
    virtual void checkFile();

    // operator
    bool dispatchPartitions();
    void createPartitions(__int64 size, std::string md5);
    // void startWorkThreads();

};


