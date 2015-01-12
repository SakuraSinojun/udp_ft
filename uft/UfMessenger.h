
#pragma once

#include "tools/lock.h"

#include <list>

class UfMessenger
{
public:
    class Message {
    public:
        int msg;
        int arg1;
        int arg2;
        void* obj;
    };

    void postMessage(Message& msg);
    void postMessage(int msg, int arg1 = 0, int arg2 = 0, void * obj = NULL);

    bool getMessage(Message& msg, int timeout_ms = 0);
    bool peekMessage(Message& msg);

private:
    tools::CLock        mLock;
    std::list<Message>  mMessages;
};

