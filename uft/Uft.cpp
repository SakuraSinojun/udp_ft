
#include "Uft.h"

#include "UftServer.h"
#include "UftClient.h"

#include "ministun_port.h"
#include "logging.h"

#include <string>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <string>


Uft::Uft()
    : s(NULL)
    , mPercentListener(NULL)
    , mGraphicsListener(NULL)
{
}

Uft::~Uft()
{
    if (s != NULL) {
        delete s;
    }
}

static int getIpAddress(const char *iface_name, char *ip_addr, int len)
{
    int sockfd = -1;
    struct ifreq ifr;
    struct sockaddr_in addr;

    memset(&ifr, 0, sizeof(struct ifreq));
    strcpy(ifr.ifr_name, iface_name);
    ifr.ifr_addr.sa_family = AF_INET;

    // addr = (struct sockaddr_in *)&ifr.ifr_addr;
    // addr->sin_family = AF_INET;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("create socket error!\n");
        return -1;
    }

    if (ioctl(sockfd, SIOCGIFADDR, &ifr) == 0) {
        memcpy(&addr, &ifr.ifr_addr, sizeof(addr));
        strncpy(ip_addr, inet_ntoa(addr.sin_addr), len);
        close(sockfd);
        return 0;
    }

    close(sockfd);

    return -1;
}

int Uft::nat(const char * natserverip, int natserverport, std::string& mapip, int& mapport, int type)
{
    if (s == NULL) {
        s = new UftServer();
        s->bind();
    }

    if (type == 0) {
        mapip = "255.255.255.255";
        mapport = 10234;
    } else {
        if (ministun_request(s->getSocket(), natserverip, natserverport, mapip, mapport) != 0) {
            return -1;
            // nat1(mapip, mapport);
        }
    }
    return 0;
}

int Uft::nat1(std::string& mapip, int& mapport)
{
    char    addr[100];
    getIpAddress("eth0", addr, (int)sizeof(addr));
    mapip = addr;
    mapport = 10234;
    return 0;
}

int Uft::requestFile(const char * ip, int port, const char * filename, const char * localfilename, int max_thread_count)
{
    UftClient   c;
    c.setListener(mPercentListener);
    c.setListener(mGraphicsListener);
    return c.requestFile(ip, port, filename, localfilename, max_thread_count);
}

int Uft::bindServer()
{
    if (s != NULL) {
        return -1;
    }
    s = new UftServer();
    s->setListener(mPercentListener);
    return s->bind();
}

int Uft::startServer()
{
    if (s == NULL) {
        s = new UftServer();
        s->setListener(mPercentListener);
        s->bind();
    }
    return s->start();
}

void Uft::setListener(UfPercentListener* listener)
{
    mPercentListener = listener;
}

void Uft::setListener(UfGraphicsListener* listener)
{
    mGraphicsListener = listener;
}



