
#include "UftClient.h"
#include "UfPacket.h"
#include "UfFile.h"
#include "logging.h"
#include "UfDef.h"
#include "tools/dump.h"

#define HEARTBIT_INTERVAL   3000

#define UFP_BUFSIZE         10000
#define UFP_WRITEINTERVAL   10
#define UFP_WATCHERINTERVAL 500

UftClient::UftClient()
    : mFileBuffer(UFP_BUFSIZE)
    , mPercentListener(NULL)
    , mGraphicsListener(NULL)
    , mMd5ErrorTimes(0)
{
    setNeedLock(true);
}

UftClient::~UftClient()
{
}

void UftClient::setListener(UfPercentListener* listener)
{
    mPercentListener = listener;
}
    
void UftClient::setListener(UfGraphicsListener* listener)
{
    mGraphicsListener = listener;
}


int UftClient::requestFile(const char * ip, int port, const char * filename, const char * localfilename, int thread_count)
{
    logging::setThreadLoggingTag(TERMC_RED "Uft-0" TERMC_NONE);

    ScopeTrace(requestFile);

    RUN_HERE() << "request file: " << ip << ":" << port << " file: " << filename;

    // thread_count = 1;

    mIp = ip;
    mPort = port;
    mLocalFile = localfilename;
    mPartFile = (mLocalFile + ".partitions").c_str();
    mRemoteFile = filename;
    mThreadCount = thread_count - 1;    // 有一个是当前线程吖~
    mPool.setMaxThreadsCount(mThreadCount);

    if (connect(ip, port) != 0) {
        ILOG() << "connect failed.";
        return UFS_TIMEOUT;
    }

    mRetryTimes = 0;
    setTimeout(0, timeout_selector(UftClient::onTimeoutRequestFileInfo));

    RunOnce(NULL);

    const char * _table[] = {
        "Running", 
        "No responose",
        "Success",
        "Truncate File Failed",
        "Failed",
        "Unknown",
    };
    RUN_HERE() << "Closing state: " << mRunning << ": " << _table[mRunning];

    if (mRunning == R_SUCCESS) {
        return UFS_SUCCESS;
    } else if (mRunning == R_FAILED) {
        return UFS_FAIL;
    } else if (mRunning == R_RUNNING) {
        return UFS_FAIL;
    } else {
        return UFS_TIMEOUT;
    }
}

#if 0
void UftClient::parsePacket(const char * buffer, int len)
{
    UfPacket    packet(buffer, len);
    RUN_HERE() << "packet.action = " << packet.action;

    // Fixme: 服务器断线还没做。
    if (packet.action == UF_HEARTBIT) {
        return;
    }

    if (packet.action == UF_FILEINFO) {
        onPacketFileInfo(buffer, len);
    }
}
#endif

bool UftClient::getRange(UfPartitions::Range& range)
{
    tools::AutoLock    lk(mRangeLock);
    if (mRanges.empty())
        return false;
    range = mRanges.front();
    mRanges.pop_front();
    return true;
}

void UftClient::putRange(UfPartitions::Range& range)
{
    tools::AutoLock    lk(mRangeLock);
    mRanges.push_back(range);
}

void UftClient::createPartitions(__int64 size, std::string md5)
{
    mPartitions.create(size);
    mPartitions.md5 = md5;
}

void UftClient::onTimeoutRequestFileInfo(void* arg)
{
    ILOG() << "send request file info.";
    mRetryTimes++;
    if (mRetryTimes >= 3) {
        ILOG() << "request file info timeout. close session.";
        Close(R_NORESPONSE);
        return;
    }
    RequestInfoPacket   rip;
    rip.filename = mRemoteFile;
    rip.piecelength = UfPartitions::PIECE_LENGTH;
    Send(rip);

    setTimeout(3000, timeout_selector(UftClient::onTimeoutRequestFileInfo));
}


void UftClient::onFileInfo(const char* buffer, int len)
{
    clearTimeout(timeout_selector(UftClient::onTimeoutRequestFileInfo));
    FileInfoPacket  fip;
    fip.fromBuffer(buffer, len);
    ILOG() << "Download file: name = " << fip.filename;
    ILOG() << "Download file: size = " << fip.length;
    ILOG() << "Download file: md5  = " << fip.md5;

    // 文件长度为0时直接创建文件就好。
    if (fip.length > 0) {
        if (mLocalFile.isExists() && mPartFile.isExists()) {
            mPartitions.LoadFromFile(mPartFile);
            if (mPartitions.md5 != fip.md5) {
                createPartitions(fip.length, fip.md5);
            } else {
                // mPartitions.dump();
            }
        } else if (mLocalFile.isExists()) {
            // ILOG() << "check md5: server = " << fip.md5;
            ILOG() << "check md5: local  = " << mLocalFile.md5();
            if (mLocalFile.md5() == fip.md5) {
                ILOG() << "not changed.";
                Close(R_SUCCESS);
                return;
            }
            ILOG() << mLocalFile.filename() << " changed. re-download it.";
            // FATAL();
            mLocalFile.remove();
            createPartitions(fip.length, fip.md5);
        } else if (mPartFile.isExists()) {
            mPartFile.remove();
            createPartitions(fip.length, fip.md5);
        } else {
            createPartitions(fip.length, fip.md5);
        }
        ILOG() << "Download file, partitions: " << mPartitions.size();
        // mPartitions.dump();
    } else if (fip.length == 0) {
        mLocalFile.remove();
    } else {
        // length < 0;
        Close(R_SUCCESS);
        if (mPartFile.hasInited() && mPartFile.isExists())
            mPartFile.remove();
        return;
    }
    if (!mLocalFile.isExists()) {
        ILOG() << "create file: " << mLocalFile.filename() << " with " << fip.length << " bytes.";
        int ret = mLocalFile.create(fip.length);
        if (ret != 0) {
            ErrorPacket ep(mSid);
            ep.error = R_TRUNCATE;
            Send(ep);
            Close(R_TRUNCATE);
            return;
        }
    }

    mLocalFile.open();
    if (!dispatchPartitions())
        return;
    // startWorkThreads();
    setTimeout(UFP_WRITEINTERVAL, timeout_selector(UftClient::onTimeoutWriteFile));

    setTimeout(0, timeout_selector(UftClient::onTimeoutRequestNextRange));
}

bool UftClient::dispatchPartitions()
{
    if (mRanges.empty()) {
        tools::AutoLock    lk(mRangeLock);
        mPartitions.getEmptyRanges(mRanges);
    }

    if (mRanges.empty()) {
        ILOG() << "nothing to download...";
        mPool.closeAll(R_SUCCESS);
        Close(R_SUCCESS);
        if (mPartFile.hasInited() && mPartFile.isExists())
            mPartFile.remove();
        return false;
    }

    ILOG() << "dispatch " << mRanges.size() << " ranges.";

    if (!isScheduled(timeout_selector(UftClient::onTimeoutThreadPoolWatcher))) {
        setTimeout(100, timeout_selector(UftClient::onTimeoutThreadPoolWatcher));
    }

    return true;
}

void UftClient::onTimeoutThreadPoolWatcher(void* arg)
{
    // 一个线程也没有时
    ILOG() << "ranges = " << mRanges.size();
    ILOG() << "threads = " << mPool.getThreadsCount() << " (" << mPool.getIdleThreadsCount() << " idle, " << mPool.getDownloadingThreadsCount() << " downloading)";

    if (mPool.getThreadsCount() == 0) {
        int count = mRanges.size();
        if (count > mThreadCount)
            count = mThreadCount;
        for (int i = 0; i < count; i++) {
            mPool.addThreadAndRun(this);
        }
    } else {
        int ranges = (int)mRanges.size();
        int idles = mPool.getIdleThreadsCount();
        if (ranges > idles) {
            int count = ranges - idles;
            int left = mPool.getMaxThreadsCount() - mPool.getThreadsCount();
            if (count > left)
                count = left;
            ILOG() << "ranges = " << ranges;
            ILOG() << "idles  = " << idles;
            ILOG() << "current= " << mPool.getThreadsCount();
            for (int i = 0; i < count; i++) {
                mPool.addThreadAndRun(this);
            }
        }
    }
    setTimeout(UFP_WATCHERINTERVAL, timeout_selector(UftClient::onTimeoutThreadPoolWatcher));
}

bool UftClient::WriteFile(__int64 offset, const char * content, __int64 length)
{
    tools::AutoLock lk(mFileBufferLock);
    return mFileBuffer.push(offset, content, length);
}

void UftClient::onTimeoutWriteFile(void* arg)
{
    if (mFileBuffer.size() > 0) {
        ILOG() << "write " << mFileBuffer.size() << " pieces to file.";

        tools::Buffer   buf;
        __int64         offset;
        while (true) {
            tools::AutoLock lk(mFileBufferLock);
            if (!mFileBuffer.pop(offset, buf))
                break;
            mLocalFile.write(offset, buf, buf.length());
            mPartitions.setPieceReceived(offset);
        }

        mPartitions.SaveToFile(mPartFile); 
        if (mPercentListener != NULL) {
            mPercentListener->onPercent(mLocalFile, mPartitions.downloadedSize(), mPartitions.size());
        }
        if (mGraphicsListener != NULL) {
            mGraphicsListener->onGraphic(mLocalFile, mPartitions.dumpGraphic());
        }
        sendPercent(mLocalFile.filename(), mPartitions.downloadedSize(), (__int64)mPartitions.size());
    }
    setTimeout(UFP_WRITEINTERVAL, timeout_selector(UftClient::onTimeoutWriteFile));
}

void UftClient::sendPercent(std::string filename, __int64 down, __int64 total)
{
    PercentPacket   pp;
    pp.filename = filename;
    pp.downloaded = down;
    pp.total = total;
    Send(pp);
}

void UftClient::onTimeoutSendOver(void* arg)
{
    if (!mRanges.empty())
        return;

    if (mPool.getDownloadingThreadsCount() > 0) {
        return;
    }
 
    if (!mPartitions.hasAllReceived()) {
        ILOG() << "not complete.";
        // mPartitions.dump();
        dispatchPartitions();
        // FATAL();
        return;
    }

    ILOG() << "check md5: server = " << mPartitions.md5;
    ILOG() << "check md5: local  = " << mLocalFile.md5();

    // 收全了但是md5不一致。。。重新下载。
    if (mPartitions.md5 != mLocalFile.md5()) {
        mMd5ErrorTimes++;
        if (mMd5ErrorTimes >= 3) {
            RUN_HERE() << "md5 check failed. tried 3 times... Terminated.";
            clearTimeout(timeout_selector(UftClient::onTimeoutWriteFile));
            clearTimeout(timeout_selector(UftClient::onTimeoutThreadPoolWatcher));
            mPool.closeAll(R_FAILED);
            Close(R_FAILED);
            if (mPartFile.hasInited() && mPartFile.isExists())
                mPartFile.remove();
            mLocalFile.close();
            mLocalFile.remove();
            return;
        }

        // FATAL() << "md5 diffent. re-download.";
        RUN_HERE() << "md5 check failed. re-download.";
        mPartitions.clear();
        mPieceBuffer.clear();
        mFileBuffer.clear();
        dispatchPartitions();
        // requestFilePartitions();
        return;
    }

    clearTimeout(timeout_selector(UftClient::onTimeoutWriteFile));
    clearTimeout(timeout_selector(UftClient::onTimeoutThreadPoolWatcher));
    mPool.closeAll(R_SUCCESS);
    Close(R_SUCCESS);
    if (mPartFile.hasInited() && mPartFile.isExists())
        mPartFile.remove();
    mLocalFile.close();
}

void UftClient::SendOver()
{
    if (mRunning != R_RUNNING) {
        return;
    }
    clearTimeout(timeout_selector(UftClient::onTimeoutSendOver));
    setTimeout(0, timeout_selector(UftClient::onTimeoutSendOver));
}

void UftClient::onTimeoutDisconnect(void* arg)
{
    ILOG() << "server timedout. close all threads and exit.";
    mPool.closeAll(R_SUCCESS);
    Close(R_NORESPONSE);
}

void UftClient::onTimeoutRequestNextRange(void* arg)
{
    if (!getRange(mRange)) {
        setTimeout(100, timeout_selector(UftClient::onTimeoutRequestNextRange));
    } else {
        setTimeout(0, timeout_selector(UftClient::onTimeoutGetRange));
    }
}

void UftClient::doWritePiecesToBuffer()
{
    if (mPieceBuffer.size() == 0)
        return;
    tools::Buffer   buf;
    __int64         offset;

    // ILOG() << "write pieces: " << mPieceBuffer.size();

    while (mPieceBuffer.pop(offset, buf)) {
        if (!WriteFile(offset, buf, buf.length())) {
            mPieceBuffer.push(offset, buf, buf.length());
            break;
        }
    }
    if (mReceivedSendOver) {
        checkFile();
        mReceivedSendOver = false;
    }
}

void UftClient::checkFile()
{
    SendOver();
    setTimeout(0, timeout_selector(UftClient::onTimeoutRequestNextRange));
}









