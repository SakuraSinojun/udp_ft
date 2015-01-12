

#include "UfMessenger.h"
#include <unistd.h>
#include <sys/time.h>


void UfMessenger::postMessage(UfMessenger::Message& msg)
{
    tools::AutoLock    lk(mLock);
    mMessages.push_back(msg);
}

void UfMessenger::postMessage(int msg, int arg1, int arg2, void * obj)
{
    Message message;
    message.msg = msg;
    message.arg1 = arg1;
    message.arg2 = arg2;
    message.obj = obj;
    postMessage(message);
}

bool UfMessenger::getMessage(UfMessenger::Message& msg, int timeout_ms)
{
    struct timeval  tv;
    gettimeofday(&tv, NULL);

    while (mMessages.empty()) {
        usleep(timeout_ms * 1000);

        if (timeout_ms <= 0)
            continue;

        struct timeval  now;
        gettimeofday(&now, NULL);

        int diff = (now.tv_sec - tv.tv_sec) * 1000 + (now.tv_usec - tv.tv_usec) / 1000;
        if (diff > timeout_ms) {
            return false;
        }
    }
    tools::AutoLock lk(mLock);
    msg = mMessages.front();
    mMessages.pop_front();
    return true;
}

bool UfMessenger::peekMessage(UfMessenger::Message& msg)
{
    tools::AutoLock lk(mLock);
    if (mMessages.empty())
        return false;
    msg = mMessages.front();
    mMessages.pop_front();
    return true;
}


