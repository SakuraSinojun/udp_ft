
#include "tools/socket.h"
#include "logging.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "tools/buffer.h"
#include "UftClient.h"
#include "UftServer.h"
#include "UfListener.h"

// #include <mcheck.h>

// 测试把200M数据全扔到网络上的速度
void test1()
{

    tools::CSocket  sock(tools::CSocket::eSocketType_UDP);
    sock.Create();
    sock.Bind(10234);

    int i;
    for (i = 0; i < 1024 * 200; i++) {
        char    buffer[1024];
        int ret = sock.SendTo("10.0.0.253", 10234, buffer, 1024);
        if (ret != 1024) {
            printf("ret = %d\n", ret);
        }
    }
}


// 多线程扔200M数据到网络上。
static tools::CSocket  sock2(tools::CSocket::eSocketType_UDP);
static void* _proc(void* arg)
{
    int i;
    for (i = 0; i < 1024 * 100; i++) {
        char    buffer[1024];
        int ret = sock2.SendTo("10.0.0.253", 10234, buffer, 1024);
        if (ret != 1024) {
            printf("ret = %d\n", ret);
        }
    }
    return NULL;
}

void test2()
{
    sock2.Create();
    sock2.Bind(10234);
    pthread_t   thread;
    pthread_create(&thread, NULL, _proc, NULL);

    int i;
    for (i = 0; i < 1024 * 100; i++) {
        char    buffer[1024];
        int ret = sock2.SendTo("10.0.0.253", 10234, buffer, 1024);
        if (ret != 1024) {
            printf("ret = %d\n", ret);
        }
    }

    pthread_join(thread, NULL);
}

void test_ufbuffer()
{
    const char a[] = {'\x55', '\x32', '\x33', '\x3', '\x4', '\x35', '\x0'};
    printf("%s\n", a);
    tools::Buffer    buf(a, 7);
    RUN_HERE() << buf;
}

class Listener : public UfPercentListener
{
public:
    virtual void onPercent(const char* filename, __int64 downloaded, __int64 total) {
        double pert = (double)downloaded * 100.0 / total;
        // if (system("clear") == 0) {
        // }
        printf("<%s> %0.2f%%\n", filename, pert);
    }
};

class GListener : public UfGraphicsListener
{
public:
    virtual void onGraphic(const char * filename, std::string graphics) {
        printf("%s\n", graphics.c_str());
        // usleep(100000);
    }
};

static void * _client_proc(void* arg)
{
    // mtrace();
    logging::setThreadLoggingTag(TERMC_RED "client" TERMC_NONE);
    RUN_HERE();

    // logging::setThreadLogging(false);

    Listener    l;
    // GListener   gl;

    UftClient c;
    c.setListener(&l);
    // c.setListener(&gl);

    unlink("aaa.tmp");
    int ret = 0;

    long f = (long)arg;
    RUN_HERE() << "f = " << f;
    if (f == 0) {
        ret = c.requestFile("10.0.0.1", 10234, "/home/kazuki/projects/Trunk.rar", "aaa.tmp", 10);
    } else if (f == 1) {
        ret = c.requestFile("10.0.0.1", 10234, "Uft.cpp", "aaa.tmp", 1);
    } else if (f == 2) {
        ret = c.requestFile("255.255.255.255", 10234, "Uft.cpp", "aaa.tmp", 1);
    } else {
        ret = c.requestFile("10.0.0.1", 10234, "/home/kazuki/projects/Trunk.rar", "aaa.tmp", 1);
    }

    RUN_HERE() << "client end at: " << ret;
    // exit(0);
    return NULL;
}

int main(int argc, char* argv[])
{
    // setenv("MALLOC_TRACE", "tracer.txt", 1);

    if (argc == 1) {
        UftServer s;

        pthread_t   thread;
        pthread_create(&thread, NULL, _client_proc, NULL);

        s.bind();
        s.start();
        pthread_join(thread, NULL);
    } else {
        if (argv[1][0] == 's') {
            logging::setThreadLoggingTag(TERMC_BLUE "server" TERMC_NONE);
            UftServer s;
            s.bind();
            int ret = s.start();
            RUN_HERE() << "server end at: " << ret;
        } else if (argv[1][0] == 'c') {
            long f = 0;
            if (argc > 2) {
                f = atoi(argv[2]);
            }
            _client_proc((void*)f);
        }
    }

    // ScopeTrace(ReadAndThrow);
    // test2();
    return 0;
}









