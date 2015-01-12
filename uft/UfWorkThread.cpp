
#include "UfWorkThread.h"
#include "UfPacket.h"
#include "UfFile.h"
#include "logging.h"
#include "UftClient.h"
#include "tools/dump.h"

#define HEARTBIT_INTERVAL   3000

#define UFP_BUFSIZE         1000
#define UFP_WRITEINTERVAL   100

UfWorkThread::UfWorkThread()
    : mSock()
    , mSid(0)   // no more use.
    , mPieceBuffer(UFP_BUFSIZE)
    , mReceivedSendOver(false)
    , mDownloading(false)
{
}

UfWorkThread::~UfWorkThread()
{
}

void UfWorkThread::Send(UfPacket& packet)
{
    packet.sessionId = mSid;
    packet.piecebuffer = mPieceBuffer.pit();

    Archive ar;
    packet.Serialize(ar);

    char    buffer[65536] = {0};
    int ret = ar.toBuffer(buffer, sizeof(buffer));
    if (ret > 0) {
        mSock.send(buffer, ret);
    }
    // usleep(10);
}

void UfWorkThread::Close(RunState reason)
{
    mRunning = reason;
    clearTimeout(timeout_selector(UfWorkThread::onTimeoutHeartbit));
    clearTimeout(timeout_selector(UfWorkThread::onTimeoutDisconnect));
    mSock.close();
}

void UfWorkThread::doClose()
{
#if 0
    EndSessionPacket    esp;
    esp.reason = mRunning;
    Send(esp);
#endif
    //  if (reason == R_SUCCESS) {
      //   if (mPartFile.hasInited() && mPartFile.isExists()) {
      //       mPartFile.remove();
      //   }
    //  }

    struct timeval now;
    gettimeofday(&now, NULL);
    int diff = (now.tv_sec - mStartTime.tv_sec) * 1000 + (now.tv_usec - mStartTime.tv_usec) / 1000;
    ILOG() << "time costs: " << diff << " ms.";
}

int UfWorkThread::connect(const char * ip, int port)
{
    mSock.close();
    mSock.create(this);
    // mSock.SetRemoteIP(ip);
    // mSock.SetRemotePort(port);
    static int s_port = 10300;
    mSock.bind(s_port++);

    int ret = mSock.connect(ip, port, 3000);
    if (ret == tools::ReliableUdpSocket::R_SUCCESS)
        return 0;
    return -1;
}

   
bool UfWorkThread::BeforeFirstRun(void* arg)
{
    if (arg != NULL) {
        mClient = (UftClient*)arg;
        if (connect(mClient->mIp.c_str(), mClient->mPort) != 0) {
            ILOG() << "connect failed. retry in 3 sec.";
            usleep(3000000);
            // re-loop.
            return false;
        }
    }
    return true;
}

bool UfWorkThread::RunOnce(void* arg)
{
    if (arg != NULL) {
        mClient = (UftClient*)arg;
        /*
        if (!mClient->getRange(mRange)) {
            usleep(100000);
            return true;
        }
        */
        setTimeout(0, timeout_selector(UfWorkThread::onTimeoutRequestFileInfo));
    }
    gettimeofday(&mStartTime, NULL);

    setTimeout(HEARTBIT_INTERVAL, timeout_selector(UfWorkThread::onTimeoutHeartbit));
    setTimeout(UFP_WRITEINTERVAL, timeout_selector(UfWorkThread::onTimeoutWriteFile));

    mRetryTimes = 0;
    mPacketCount = 0;
    mRunning = R_RUNNING;
    // int     timeout = 500;
    while (true /* mRunning == R_RUNNING */) {
        int ret = 0;
        int timeout = getNearestMs();
        if (timeout <= 0)
            timeout = 1;

        ret = mSock.recv(timeout);
        if (ret == tools::ReliableUdpSocket::R_CLOSED)
            break;

        mPacketCount++;

        if (ret == tools::ReliableUdpSocket::R_TIMEOUT) {
            checkTimeout();
        } else if (ret <= 0) {
            // ILOG() << mSock.GetErrorMessage();
            break;
        } else {
#if 0
            if (!UfPacket::isValidPacket(buffer, ret)) {
                FATAL() << "invalid packet.";
                continue;
            }
            UfPacket    packet(buffer, ret);
            /*
            if (packet.sessionId != mSid) {
                FATAL() << "invalid sessionid.";
                continue;
            }
            */
            RUN_HERE();
            parsePacket(buffer, ret);
#endif
        }
    }
    if (mRunning == R_RUNNING) {
        ILOG() << "connection closed by remote server.";
    } else {
        ILOG() << "client end. reason = " << mRunning;
    }
    doClose();
    // FATAL();
    return false;
}


void UfWorkThread::onTimeoutRequestFileInfo(void* arg)
{
    ILOG() << "send request file info.";
    mRetryTimes++;
    if (mRetryTimes >= 3) {
        ILOG() << "request file info timeout. close session.";
        Close(R_NORESPONSE);
        return;
    }
    RequestInfoPacket   rip;
    rip.filename = mClient->mRemoteFile;
    rip.piecelength = UfPartitions::PIECE_LENGTH;
    Send(rip);

    setTimeout(3000, timeout_selector(UfWorkThread::onTimeoutRequestFileInfo));
}

void UfWorkThread::onTimeoutHeartbit(void* arg)
{
    HeartbitPacket  hbp(mSid);
    Send(hbp);
    setTimeout(HEARTBIT_INTERVAL, timeout_selector(UfWorkThread::onTimeoutHeartbit));
}

void UfWorkThread::onDataReceived(const char * buffer, int len)
{
    if (!UfPacket::isValidPacket(buffer, len)) {
        FATAL() << "invalid packet.";
        return;
    }
    parsePacket(buffer, len);
}

void UfWorkThread::parsePacket(const char * buffer, int len)
{
    UfPacket    pack(buffer, len);
    // if (pack.action != UF_FILECONTENT)
    //     RUN_HERE() << "pack.action = " << pack.action;

    clearTimeout(timeout_selector(UfWorkThread::onTimeoutDisconnect));
    setTimeout(30000, timeout_selector(UfWorkThread::onTimeoutDisconnect));

    // Fixme: 服务器断线。
    if (pack.action == UF_HEARTBIT) {
        return;
    }

    if (pack.action == UF_FILEINFO) {
        onFileInfo(buffer, len);
    } else if (pack.action == UF_FILECONTENT) {
        onFileContent(buffer, len);
    } else if (pack.action == UF_SENDOVER) {
        onSendOver(buffer, len);
    }
}


void UfWorkThread::onFileContent(const char* buffer, int len)
{
    FileContentPacket fcp(mSid);
    fcp.fromBuffer(buffer, len);

    // RUN_HERE() << "buffer = " << (void*)buffer;
    // RUN_HERE() << "fcp.buffer = " << (void*)fcp.buffer;
    // RUN_HERE() << "fcp.buffer.length() = " << fcp.buffer.length();
    // tools::dump(buffer, len);
    // tools::dump(fcp.buffer, fcp.buffer.length());

    while (!mPieceBuffer.push(fcp.offset, fcp.buffer, fcp.buffer.length())) {
        // 一般不会走到这里来。
        // ILOG() << "force doWritePiecesToBuffer.";
        doWritePiecesToBuffer();
    }

    // RUN_HERE() << "pit = " << mPieceBuffer.pit();
    if (mPieceBuffer.pit() == 0) {
        // ILOG() << "force doWritePiecesToBuffer.";
        doWritePiecesToBuffer();
    }

    // clearTimeout(timeout_selector(UfWorkThread::onTimeoutFilecontent));
    // setTimeout(3000, timeout_selector(UfWorkThread::onTimeoutFilecontent));
}

// void UfWorkThread::onTimeoutFilecontent(void* arg)
// {
//     // checkFile();
// }

void UfWorkThread::onTimeoutWriteFile(void* arg)
{
    if (mPieceBuffer.size() > 0) {
        doWritePiecesToBuffer();
    }
    setTimeout(UFP_WRITEINTERVAL, timeout_selector(UfWorkThread::onTimeoutWriteFile));
}

void UfWorkThread::doWritePiecesToBuffer()
{
    if (mClient == NULL) {
        return;
    }
    if (mPieceBuffer.size() == 0)
        return;
    
    tools::Buffer   buf;
    __int64         offset;
    while (mPieceBuffer.pop(offset, buf)) {
        if (!mClient->WriteFile(offset, buf, buf.length())) {
            mPieceBuffer.push(offset, buf, buf.length());
            break;
        }
    }

    if (mReceivedSendOver) {
        checkFile();
        mReceivedSendOver = false;
    }
}

void UfWorkThread::onSendOver(const char * buffer, int len)
{
    // RUN_HERE();
    // SendOverPacket  sop;
    // sop.request = SendOverPacket::ACK;
    // Send(sop);

    // 以免接收到结束消息但是还没写文件的情况下出现对文件末端额外请求的情况。
    mReceivedSendOver = true;
    if (mPieceBuffer.size() == 0) {
        checkFile();
    }
}

void UfWorkThread::checkFile()
{
    if (mClient != NULL)
        mClient->SendOver();

    setTimeout(0, timeout_selector(UfWorkThread::onTimeoutRequestNextRange));

    mDownloading = false;
}

void UfWorkThread::onTimeoutGetRange(void* arg)
{
    SLOG() << "download range: " << mRange.offset << ", " <<  mRange.length << ", that is " << mRange.length / UfPartitions::PIECE_LENGTH << " pieces.";
    RequestFilePacket   rfp;
    rfp.offset = mRange.offset;
    rfp.length = mRange.length;
    Send(rfp);

    // RUN_HERE() << "send pit: " << pbp.piecebuffer;


    // clearTimeout(timeout_selector(UfWorkThread::onTimeoutFilecontent));
    // setTimeout(3000, timeout_selector(UfWorkThread::onTimeoutFilecontent));

    mDownloading = true;
}

void UfWorkThread::onFileInfo(const char * buffer, int len)
{
    setTimeout(0, timeout_selector(UfWorkThread::onTimeoutRequestNextRange));
    // setTimeout(0, timeout_selector(UfWorkThread::onTimeoutGetRange));
    clearTimeout(timeout_selector(UfWorkThread::onTimeoutRequestFileInfo));
}

void UfWorkThread::onTimeoutRequestNextRange(void* arg)
{
    if (mClient == NULL)
        return;
    if (!mClient->getRange(mRange)) {
        setTimeout(100, timeout_selector(UfWorkThread::onTimeoutRequestNextRange));
    } else {
        setTimeout(0, timeout_selector(UfWorkThread::onTimeoutGetRange));
    }
}

void UfWorkThread::onTimeoutDisconnect(void* arg)
{
    Close(R_NORESPONSE);
}
















