
#include "Sft.h"
#include "SfPacket.h"
#include "UfFile.h"
#include "tools/dump.h"
#include "tools/logging.h"

///////////////////////////////////////////////////////////////////////////////////

void Sft::Delegate::onDataReceived(const char * buffer, int len)
{
    if (mSft != NULL)
        mSft->onDataReceived(buffer, len);
}

void Sft::Delegate::onPercent(float percent)
{
}

///////////////////////////////////////////////////////////////////////////////////

Sft::Sft(Delegate& d)
    : mDelegate(d)
    , mRecvFile(NULL)
    , mSendFile(NULL)
{
    mDelegate.mSft = this;
    mRecvFile = new UfFile();
    mSendFile = new UfFile();
}

Sft::~Sft()
{
    if (mRecvFile != NULL) {
        delete mRecvFile;
    }
    if (mSendFile != NULL) {
        delete mSendFile;
    }
}

void Sft::send(const char * buffer, int len)
{
    mDelegate.send(buffer, len);
}

void Sft::send(SfPacket& packet)
{
    Archive ar;
    packet.Serialize(ar);
    char    buffer[65536] = {0};
    int ret = ar.toBuffer(buffer, sizeof(buffer));
    if (ret > 0) {
        send(buffer, ret);
    }
}

int Sft::requestFile(const char * filename, const char * localfilename)
{
    SfRequestInfoPacket pack;
    pack.filename = filename;
    mRecvFile->init(localfilename);
    send(pack);
    return 0;
}

void Sft::onDataReceived(const char * buffer, int len)
{
    if (!SfPacket::isValidPacket(buffer, len)) {
        return;
    }

    SfPacket    packet;
    packet.fromBuffer(buffer, len);
    
    switch (packet.action) {
    case SF_REQUESTINFO:
        onRequestInfo(buffer, len);
        break;
    case SF_FILEINFO:
        onFileInfo(buffer, len);
        break;
    case SF_REQUESTFILE:
        onRequestFile(buffer, len);
        break;
    case SF_FILECONTENT:
        onFileContent(buffer, len);
        break;
    case SF_ENDSESSION:
        onEndSession(buffer, len);
        break;
    case SF_SENDOVER:
        onSendOver();
        break;
    default:
        break;
    }
}

void Sft::onRequestInfo(const char* buffer, int len)
{
    SfRequestInfoPacket packet;
    packet.fromBuffer(buffer, len);

    mSendFile->init(packet.filename.c_str());

    SfFileInfoPacket    sip;
    sip.filename = packet.filename;
    sip.filesize = mSendFile->getSize();
    sip.md5      = mSendFile->md5();
    send(sip);

    mTotalBytes = mSendFile->getSize();
    mDownloadedBytes = 0;

    ILOG();
}

void Sft::onFileInfo(const char* buffer, int len)
{
    SfFileInfoPacket    sip;
    sip.fromBuffer(buffer, len);
    
    ILOG() << "file info: ";
    ILOG() << "\tfilename = " << sip.filename;
    ILOG() << "\tfilesize = " << sip.filesize;
    ILOG() << "\tmd5      = " << sip.md5;

    if (sip.filesize == -1) {
        ILOG() << "file not exists. endSession.";
        endSession(R_NO_SUCH_FILE);
        return;
    }

    if (mRecvFile->isExists()) {
        if (mRecvFile->md5() == sip.md5) {
            ILOG() << "file not change. endSession.";
            endSession(R_SUCCESS);
            return;
        }
        mRecvFile->remove();
    }
    if (mRecvFile->create(sip.filesize) != 0) {
        ILOG() << "create file failed. endSession.";
        endSession(R_CREATE_FAILED);
        return;
    }
    if (mRecvFile->open() != 0) {
        ILOG() << "open file failed. endSession.";
        endSession(R_OPEN_FAILED);
        return;
    }

    if (sip.filesize == 0) {
        ILOG() << "file size is zero. endSession.";
        endSession(R_SUCCESS);
        return;
    }

    mRecvFile->mExpectedMD5 = sip.md5;

    mDownloadedBytes = 0;
    mTotalBytes = sip.filesize;
    RUN_HERE() << "mTotalBytes = " << mTotalBytes;

    SfRequestFilePacket srp;
    srp.offset = 0;
    srp.length = sip.filesize;
    send(srp);

}

void Sft::onRequestFile(const char* buffer, int len)
{
    if (mSendFile->open() != 0) {
        ILOG() << "open file failed. endSession.";
        endSession(R_READ_FAILED);
        return;
    }

    SfRequestFilePacket srp;
    srp.fromBuffer(buffer, len);

    __int64 offset = srp.offset;
    while (offset < srp.offset + srp.length) {
        char buf[4096] = {0};
        int ret = mSendFile->read(offset, buf, sizeof(buf));
        if (ret <= 0) {
            break;
        }
        
        if (offset + ret > srp.offset + srp.length) {
            ret = srp.offset + srp.length - offset;
        }

        SfFileContentPacket scp;
        scp.offset = offset;
        scp.buffer.assign(buf, ret);
        send(scp);

        // ILOG() << "send " << ret << " bytes.";

        mDownloadedBytes += ret;
        float percent = mDownloadedBytes * 100.0 / mTotalBytes;
        RUN_HERE() << "percent = " << percent;
        mDelegate.onPercent(percent);

        offset += ret;
    }
    mSendFile->close();

    SfSendOverPacket    sop;
    send(sop);
    // endSession(R_SUCCESS);
}

void Sft::onFileContent(const char* buffer, int len)
{
    SfFileContentPacket scp;
    scp.fromBuffer(buffer, len);

    // ILOG() << "write " << scp.buffer.length() << " bytes.";
    mRecvFile->write(scp.offset, scp.buffer, scp.buffer.length());

    mDownloadedBytes += scp.buffer.length();

    // RUN_HERE() << "mDownloadedBytes = " << mDownloadedBytes;
    // RUN_HERE() << "mTotalBytes = " << mTotalBytes;
    float percent = mDownloadedBytes * 100.0 / mTotalBytes;
    RUN_HERE() << "percent = " << percent;
    mDelegate.onPercent(percent);
}

void Sft::onEndSession(const char* buffer, int len)
{
    SfEndSessionPacket  sep;
    sep.fromBuffer(buffer, len);
    mDelegate.onResult(sep.result);

    const char* _table[] = {
        "Success.",
        "Create File Failed.",
        "Read File Failed.",
        "Open File Failed.",
        "Invalid State.",
        "No Such File."
    };
    ILOG() << "received endSession: result = " << sep.result << ": " << _table[sep.result];
}

void Sft::onSendOver()
{
    ILOG() << "onSendOver.";
    if (!mRecvFile->hasInited()) {
        ILOG() << "file not opened. endSession.";
        endSession(R_INVALID_STATE);
        return;
    }
    ILOG() << "local  md5: " << mRecvFile->md5();
    ILOG() << "server md5: " << mRecvFile->mExpectedMD5;

    if (!mRecvFile->checkMd5()) {
        FATAL();
    }
    ILOG() << "download success. endSession.";
    endSession(R_SUCCESS);
}

void Sft::endSession(int result)
{
    SfEndSessionPacket  sep;
    sep.result = result;
    send(sep);

    const char* _table[] = {
        "Success.",
        "Create File Failed.",
        "Read File Failed.",
        "Open File Failed.",
        "Invalid State.",
        "No Such File."
    };
    ILOG() << "endSession: reason = " << result << ": " << _table[result];
    mDelegate.onResult(result);
}


















