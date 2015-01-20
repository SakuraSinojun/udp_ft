

#include "UftServer.h"
#include "UfDef.h"
#include "UfPacket.h"
#include "UfServerNode.h"
// #include "UfTime.h"
#include "tools/logging.h"
#include "tools/dump.h"
#include <sys/time.h>


UftServer::ServerNodeCreator::ServerNodeCreator(UftServer* s)
    : mUftServer(s)
{
}

UftServer::ServerNodeCreator::ServerNodeCreator()
    : mUftServer(NULL)
{
}

tools::ReliableUdpSocket::ServerSocket* UftServer::ServerNodeCreator::create()
{
    UfServerNode* s = new UfServerNode(*mUftServer);
    return s;
}
void UftServer::ServerNodeCreator::remove(tools::ReliableUdpSocket::ServerSocket* s)
{
    delete s;
}

// static ServerNodeCreator gServerNodeCreator;

UftServer::UftServer()
    : mPercentListener(NULL)
    , mCreator(this)
    // : mState(UfS_SUCCESS)
{
}

UftServer::~UftServer()
{
    mSock.registerCreator(NULL);
    mSock.close();
}

#if 0
int UftServer::stop(int state)
{
    mServerRunning = false;
    // mState = state;
    return 0;
}
#endif

void UftServer::setListener(UfPercentListener* listener)
{
    mPercentListener = listener;
}

void UftServer::onPercent(std::string filename, __int64 downloaded, __int64 total)
{
    if (mPercentListener != NULL) {
        mPercentListener->onPercent(filename.c_str(), downloaded, total);
    }
    RUN_HERE() << "percent: " << filename << ": " << downloaded * 100.0 / total;
}

int UftServer::bind()
{
    mSock.registerCreator(&mCreator);
    mSock.create();

    int port = 10234;
    while (mSock.bind(port) != 0) {
        port++;
        if (port > 10400)
            return -1;
    }
    return port;
}

int UftServer::start(int timeout_ms)
{
    ILOG() << "server started.";
    mServerRunning = true;

    // bool    bConnected = false;
    // char    buffer[65536];
    // int     timeout = UfTime::now() + timeout_ms;
    
    mSock.listen();
    int ret = tools::ReliableUdpSocket::R_SUCCESS;
    while (mServerRunning) {
        ret = mSock.accept(timeout_ms);

        // Fixme: 只要还有一个连着就不按超时算。
        if (ret == tools::ReliableUdpSocket::R_TIMEOUT) {
            if (mSock.getNodeCount() > 1) {
                continue;
            }
        }

        if (ret != tools::ReliableUdpSocket::R_SUCCESS) {
            break;
        }
    }
    const char* _table[] = {
        "Success.",
        "Check Errno.",
        "Illegal State.",
        "Timeout.",
        "Bogus Packet.",
        "Invalid Packet.",
        "Accepted.",
        "No Buffer.",
        "Closed.",
        "Max Retry.",
    };
    ILOG() << "server stop at: " << ret << ": " << _table[-ret];
    mSock.close();
    if (ret == tools::ReliableUdpSocket::R_TIMEOUT) {
        return UFS_TIMEOUT;
    }
    if (ret == tools::ReliableUdpSocket::R_CLOSED) {
        return UFS_SUCCESS;
    }
    return UFS_OTHER;
#if 0
        ret = mSock.Recv(buffer, sizeof(buffer), timeout);
        if (ret < 0) {
            ILOG() << mSock.GetErrorMessage();
            break;
        }
        if (!bConnected) {
            struct timeval now;
            gettimeofday(&now, NULL);
            int diffms = (now.tv_sec - st.tv_sec) * 1000 + (now.tv_usec - st.tv_usec) / 1000;
            if (diffms > timeout_ms && mClients.size() == 0) {
                stop(UfS_TIMEOUT);
                break;
            }
        }

        if (mClients.size() > 0) {
            std::list<UfServerNode*>::iterator  it;
            for (it = mClients.begin(); it != mClients.end();) {
                if ((*it)->isValid()) {
                    (*it)->checkTimeout();
                }
                // check again.
                if ((*it)->isValid()) {
                    int ms = (*it)->getNearestMs();
                    if (timeout == 0) {
                        timeout = ms;
                    } else {
                        if (timeout > ms)
                            timeout = ms;
                    }
                    it++;
                } else {
                    ILOG() << "erase client. " << ((*it)->sid());
                    delete *it;
                    it = mClients.erase(it);
                    ILOG() << mClients.size() << " client left.";
                    if (mClients.size() == 0) {
                        stop(UfS_SUCCESS);
                    }
                }
            }
        }

        if (ret == 0) {
            continue;
        }

        // ret > 0
        if (!UfPacket::isValidPacket(buffer, ret)) {
            // tools::dump(buffer, ret);
            // ignore.
            ILOG() << "Unknown packet. ignore.";
            continue;
        }

        Archive ar;
        ar.fromBuffer(buffer, ret);
        UfPacket packet(ar);
        // tools::dump(&packet, sizeof(packet));

        int sid = packet.sessionId;
        std::list<UfServerNode*>::iterator  it;
        for (it = mClients.begin(); it != mClients.end(); it++) {
            if ((*it)->sid() == sid)
                break;
        }

        if (it != mClients.end() && (*it)->isValid()) {
            (*it)->parsePacket(buffer, ret);
        } else {
            if (it != mClients.end()) {
                ILOG() << "erase client. " << ((*it)->sid());
                delete *it;
                mClients.erase(it);
                ILOG() << mClients.size() << " clients left.";
                if (mClients.size() == 0) {
                    stop(UfS_SUCCESS);
                }
            }

            if (packet.action == UF_HELLO) {
                ILOG() << "received HELLO";
                UfServerNode* client = new UfServerNode(mSock);
                client->parsePacket(buffer, ret);
                mClients.push_back(client);
                ILOG() << "new client arrival.";
                bConnected = true;
            } else {
                // ignore.
                ILOG() << "invalid packet. ignore.";
            }
        }
    RUN_HERE() << "stop server. " << mState;
    return mState;
#endif
}


