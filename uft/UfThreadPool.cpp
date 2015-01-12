
#include "UfThreadPool.h"
#include "tools/logging.h"

UfThreadPool::UfThreadPool() 
    : mThreadId(1)
{
}

UfThreadPool::~UfThreadPool()
{
}

void UfThreadPool::initAll(int thread_count)
{
    mMaxCount = thread_count;
    for (int i = 0; i < thread_count; i++) {
        tools::Thread* thread = new tools::Thread(new UfWorkThread());
        mThreads.push_back(thread);
    }
}

void UfThreadPool::startAll(void* arg)
{
    std::list<tools::Thread*>::iterator it;
    for (it = mThreads.begin(); it != mThreads.end(); it++) {
        tools::Thread * thread = (*it);
        thread->Start(arg);
    }
}

void UfThreadPool::closeAll(UfWorkThread::RunState reason)
{
    checkRunning();
    std::list<tools::Thread*>::iterator it;
    for (it = mThreads.begin(); it != mThreads.end(); it++) {
        tools::Thread * thread = (*it);
        UfWorkThread* uft = (UfWorkThread*)thread->delegate();
        uft->Close(reason);
        thread->Stop();
        delete uft;
    }
}

void UfThreadPool::addThreadAndRun(void* arg)
{
    RUN_HERE() << "start new thread...";
    checkRunning();
    if ((int)mThreads.size() < mMaxCount) {
        tools::Thread* thread = new tools::Thread(new UfWorkThread());
        mThreads.push_back(thread);

        char    name[4096] = {0};
        snprintf(name, sizeof(name), TERMC_RED"Uft-%d"TERMC_NONE, mThreadId++);

        thread->Start(arg, name);
    }
    RUN_HERE() << "total thread count: " << mThreads.size();
}

int UfThreadPool::getThreadsCount()
{
    checkRunning();
    return mThreads.size();
}

int UfThreadPool::getDownloadingThreadsCount()
{
    checkRunning();
    if (mThreads.empty())
        return 0;

    int count = 0;
    std::list<tools::Thread*>::iterator it;
    for (it = mThreads.begin(); it != mThreads.end(); it++) {
        tools::Thread * thread = (*it);
        UfWorkThread* uft = (UfWorkThread*)thread->delegate();
        if (uft->isDownloading()) {
            count++;
        }
    }
    return count;
}

int UfThreadPool::getIdleThreadsCount()
{
    checkRunning();
    if (mThreads.empty())
        return 0;

    int count = 0;
    std::list<tools::Thread*>::iterator it;
    for (it = mThreads.begin(); it != mThreads.end(); it++) {
        tools::Thread * thread = (*it);
        UfWorkThread* uft = (UfWorkThread*)thread->delegate();
        if (!uft->isDownloading()) {
            count++;
        }
    }
    return count;
}

void UfThreadPool::checkRunning()
{
    std::list<tools::Thread*>::iterator it;
    for (it = mThreads.begin(); it != mThreads.end();) {
        tools::Thread * thread = (*it);
        if (thread->isRunning()) {
            it++;
        } else {
            it = mThreads.erase(it);
            thread->Stop();
            delete thread;
        }
    }
}







