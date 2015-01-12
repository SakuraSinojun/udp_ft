
#pragma once

#include "UfWorkThread.h"
#include "tools/thread.h"
#include <list>

class UfThreadPool
{
public:
    UfThreadPool();
    virtual ~UfThreadPool();

    void initAll(int thread_count = 1);
    void startAll(void* arg = NULL);

    void closeAll(UfWorkThread::RunState reason);

    // 
    void setMaxThreadsCount(int thread_count = 1) { mMaxCount = thread_count; }
    int  getMaxThreadsCount() { return mMaxCount; }

    void addThreadAndRun(void* arg = NULL);

    int  getThreadsCount();
    int  getDownloadingThreadsCount();
    int  getIdleThreadsCount();


private:
    std::list<tools::Thread*>    mThreads;
    int     mMaxCount;
    int     mThreadId;

    void  checkRunning();
};



