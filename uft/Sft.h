
#pragma once

class UfFile;
class SfPacket;
class Sft
{
public:
    class Delegate {
    public:
        virtual void send(const char * buffer, int len) = 0;
        virtual void onResult(int result) = 0;
        virtual void onPercent(float percent);
        void onDataReceived(const char * buffer, int len);
    private:
        friend class Sft;
        Sft*    mSft;
    };

    Sft(Delegate& d);
    ~Sft();

    int requestFile(const char * filename, const char * localfilename);

    static const int R_SUCCESS = 0;
    static const int R_CREATE_FAILED = 1;
    static const int R_READ_FAILED = 2;
    static const int R_OPEN_FAILED = 3;
    static const int R_INVALID_STATE = 4;
    static const int R_NO_SUCH_FILE = 5;

private:
    Delegate&   mDelegate;
    void onDataReceived(const char * buffer, int len);
    void send(const char * buffer, int len);
    void send(SfPacket& packet);

    UfFile*     mRecvFile;
    UfFile*     mSendFile;

    void onRequestInfo(const char* buffer, int len);
    void onFileInfo(const char* buffer, int len);
    void onRequestFile(const char* buffer, int len);
    void onFileContent(const char* buffer, int len);
    void onEndSession(const char* buffer, int len);
    void onSendOver();

    void endSession(int result);
    
    long long mDownloadedBytes;
    long long mTotalBytes;
};

