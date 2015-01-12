

#include "UftServer.h"
#include "UfDef.h"
#include "UfPacket.h"
#include "UfServerNode.h"
// #include "UfTime.h"
#include "tools/logging.h"
#include "tools/dump.h"
#include <sys/time.h>

class ServerNodeCreator : public tools::ReliableUdpSocket::ServerSocketCreator {
public:
    virtual tools::ReliableUdpSocket::ServerSocket* create() {
        UfServerNode* s = new UfServerNode();
        // RUN_HERE() << "create ServerNode: " << s;
        return s;
    }
    virtual void remove(tools::ReliableUdpSocket::ServerSocket* s) {
        // RUN_HERE() << "delete ServerNode: " << s;
        delete s;
    }
};

static ServerNodeCreator gServerNodeCreator;

UftServer::UftServer()
    // : mState(UfS_SUCCESS)
{
}

UftServer::~UftServer()
{
}

#if 0
int UftServer::stop(int state)
{
    mServerRunning = false;
    // mState = state;
    return 0;
}
#endif

int UftServer::bind()
{
    mSock.registerCreator(&gServerNodeCreator);
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
    ILOG() << "server stop at: " << ret;
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


