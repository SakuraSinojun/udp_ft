
#include "UfServerNode.h"
#include "UfPacket.h"
#include "logging.h"
#include "tools/dump.h"
#include "UfTime.h"
#include <string.h>
#include <sys/time.h>

#include <unistd.h>

#define SENDFILE_INTERVAL   200

// #define __LOSTPACKETS_TEST

static int g_sessionId = 10000;

class ReadHelper : public UfFile::ReadCallback
{
public:
    ReadHelper(UfServerNode& n) : mNode(n) {}
    virtual bool onRead(__int64 offset, const char * buffer, int len) {
        if (len <= 0)
            return true;

        FileContentPacket   fcp(mNode.sid());
        fcp.offset = offset;
        fcp.buffer.assign(buffer, len);

#ifdef __LOSTPACKETS_TEST  
        int r = rand() % 100;
        if (r <= 1)
            return;
#endif
        return mNode.Send(fcp);
    }
private:
    UfServerNode&   mNode;
};



UfServerNode::UfServerNode()
    : mValid(true)
    , mClientPieceBuffer(-1)
{
    // strcpy(remoteIp, sock.GetRemoteIP());
    // remotePort = sock.GetRemotePort();
    mSid = makeSid();
}

bool UfServerNode::Send(UfPacket& packet, bool force)
{
    if (packet.sessionId == -1) {
        packet.sessionId = sid();
    }

    Archive ar;
    packet.Serialize(ar);
    char    buffer[65536];
    int ret = ar.toBuffer(buffer, sizeof(buffer));
    if (ret > 0) {
#if 0
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(mSock.GetSocket(), &fds);
        select(mSock.GetSocket() + 1, NULL, &fds, NULL, NULL);

        int bytes = mSock.SendTo(remoteIp, remotePort, buffer, ret);
        if (bytes != ret) {
            perror("sendto.");
        }
#endif
        ret = send(buffer, ret, force);
        // usleep(20);
        if (ret == tools::ReliableUdpSocket::R_SUCCESS) {
            return true;
        } else {
            // RUN_HERE() << "ret = " << ret;
        }
        return false;
    }
    return false;
}


int  UfServerNode::makeSid()
{
    return g_sessionId++;
}

void UfServerNode::onDataReceived(const char * buffer, int len)
{
    // heartbit..
    clearTimeout(timeout_selector(UfServerNode::toDisconnect));
    setTimeout(30000, timeout_selector(UfServerNode::toDisconnect));

    UfPacket    packet(buffer, len);

#if 0
    if (packet.action == UF_PIECEBUFFER) {
        mClientPieceBuffer = packet.piecebuffer;
        ILOG() << "Client piece buffer = " << mClientPieceBuffer;
    }
#endif

#if 0
    // end session.
    if (packet.action == UF_ENDSESSION) {
        RUN_HERE() << "received EndSession. close session: " << sid();
        Close();
        return;
    }
#endif

    if (packet.action == UF_HEARTBIT) {
        return;
    }

    if (packet.action == UF_REQUESTINFO) {
        RequestInfoPacket   rip(sid());
        rip.fromBuffer(buffer, len);
        ILOG() << "checking " << rip.filename;

        mFile.init(rip.filename.c_str());
        mClientPieceLength = rip.piecelength;

        FileInfoPacket  fip(sid());
        fip.filename = mFile.filename();
        fip.length = mFile.getSize();
        fip.md5 = mFile.md5();
        Send(fip);
        ILOG() << "send fileinfo.";
    } else if (packet.action == UF_REQUESTFILE) {
        RequestFilePacket   rfp(sid());
        rfp.fromBuffer(buffer, len);

        FilePart    fp;
        fp.offset = rfp.offset;
        fp.length = rfp.length;
        fp.piecelength = mClientPieceLength;

        mSendList.push_back(fp);
        setTimeout(SENDFILE_INTERVAL, timeout_selector(UfServerNode::toSendFile));
    } 
    /*
    else if (packet.action == UF_SENDOVER) {
        SendOverPacket  sop;
        sop.fromBuffer(buffer, len);
        if (sop.request == SendOverPacket::ACK) {
            clearTimeout(timeout_selector(UfServerNode::toWaitingSendOverAck));
        }
    }
    */
}

void UfServerNode::onClose()
{
    mValid = false;
    // RUN_HERE();
}

void UfServerNode::Close()
{
    clearTimeout(timeout_selector(UfServerNode::toDisconnect));
    mValid = false;
    close();
}

void UfServerNode::toHeartbit(void* arg)
{
    HeartbitPacket  hp(sid());
    Send(hp);
    setTimeout(10000, timeout_selector(UfServerNode::toHeartbit));
}

void UfServerNode::toDisconnect(void* arg)
{
    ILOG() << "client timedout.";
    Close();
}


void UfServerNode::toSendFile(void* arg)
{
    setTimeout(SENDFILE_INTERVAL, timeout_selector(UfServerNode::toSendFile));

    if (mSendList.empty()) {
        return;
    }

#if 0
    if (mClientPieceBuffer <= 0) {
        // RUN_HERE() << "client buffer full.";
        setTimeout(SENDFILE_INTERVAL, timeout_selector(UfServerNode::toSendFile));
        return;
    }
#endif

    FilePart    fp = mSendList.front();
    mSendList.pop_front();

    int allpieces = (fp.length + fp.piecelength - 1) / fp.piecelength;
    int sp = allpieces;
    /*
    if (sp > 1000) {
        sp = 1000;
    }
    */
    // ILOG() << "sp = " << sp;

    ReadHelper  helper(*this);
    mFile.open();


    int pieces = mFile.read(fp.offset, fp.piecelength, sp, helper);
    if (pieces == 0) {
        mSendList.push_front(fp);
        return;
    }

    // ILOG() << "Send " << pieces << " pieces. " << (allpieces - pieces) << " left.";

    if (pieces < allpieces) {
        fp.offset += pieces * fp.piecelength;
        fp.length -= pieces * fp.piecelength;
        mSendList.push_front(fp);
    } else {
        if (mSendList.empty()) {
            ILOG() << "Send over.";
            mRetryTimes = 0;
            SendOverPacket  sdp(sid());
            sdp.request = SendOverPacket::REQUEST;
            Send(sdp, true);
        }
    }

#if 0
    // {{{
    // ILOG() << "send from " << fp.offset << ", length " << fp.length;
    int allpieces = (fp.length + fp.piecelength - 1) / fp.piecelength;

    int sp = allpieces;
    if (sp > mClientPieceBuffer)
        sp = mClientPieceBuffer;
#if 0
    // 每1ms发10个1K数据
    int threshold = 10;
    static struct timeval   last = {0, 0};
    struct timeval now;
    gettimeofday(&now, NULL);
    if (last.tv_sec != 0) {
        RUN_HERE();
        __int64 diff = (now.tv_sec - last.tv_sec) * 1000000 + (now.tv_usec - last.tv_usec);

        int spd = 10 * 1024 * 1024 / fp.piecelength;
        threshold = spd * diff / 1000000;
    }
    last = now;
    if (threshold < 1)
        threshold = 1;
    ILOG() << "threshold = " << threshold;
    if (sp > threshold) {
        sp = threshold;
    }
#endif

    struct timeval t1, t2;

    ReadHelper  helper(*this);
    mFile.open();

    gettimeofday(&t1, NULL);
    int pieces = mFile.read(fp.offset, fp.length, fp.piecelength, sp, helper);
    gettimeofday(&t2, NULL);

    int diff = (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);
    float speed = (float)pieces * 1000 * fp.piecelength / diff;
    
    mClientPieceBuffer -= pieces;
    ILOG() << "Send " << pieces << " pieces. " << (allpieces - pieces) << " left. speed: " << speed << " K/s";

    if (allpieces > pieces) {
        fp.offset += pieces * fp.piecelength;
        fp.length -= pieces * fp.piecelength;
        mSendList.push_front(fp);
    } else {
        if (mSendList.empty()) {
            RUN_HERE() << "Send over.";
            mRetryTimes = 0;
            SendOverPacket  sdp(sid());
            sdp.request = SendOverPacket::REQUEST;
            Send(sdp);
            // setTimeout(3000, timeout_selector(UfServerNode::toWaitingSendOverAck));
        }
    }
    // }}}
#endif
}

#if 0
void UfServerNode::toWaitingSendOverAck(void* arg)
{
    /*
    mRetryTimes++;
    if (mRetryTimes >= 3) {
        // Close();
    }
    */
    RUN_HERE() << "SendOver timedout. resent.";
    SendOverPacket  sdp;
    sdp.request = SendOverPacket::REQUEST;
    Send(sdp);
    setTimeout(3000, timeout_selector(UfServerNode::toWaitingSendOverAck));
}
#endif




















