
#pragma once

#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include "UfTypes.h"

class UfFile
{
public:
    UfFile();
    UfFile(std::string filename);
    ~UfFile();

    void init(const char * filename);
    bool hasInited() { return !mFilename.empty(); }

    bool isExists();

    int remove();
    int rename(const char * newpath);

    off_t getSize();
    const char * filename() { return mFilename.c_str(); }
    std::string md5();

    int create(__int64 size);
    int open();
    int close();

    class ReadCallback {
    public:
        // true: continue;
        // false: suspend
        virtual bool onRead(__int64 offset, const char * buffer, int len) = 0;
    };
    int read(__int64 offset,  __int64 piecelen, int pieces, ReadCallback& cb);
    void write(__int64 offset, const char * buffer, int len);
    int read(__int64 offset, char* buffer, int len);

    // 可以直接当字符串用
    UfFile& operator=(const char * filename) { init(filename); return *this; }
    std::string operator+(const char * str) { return mFilename + str; }
    operator const char *() { return mFilename.c_str(); }

    static bool isExists(const char * filename);
    static off_t getSize(const char * filename);
    static std::string md5(const char * filename, __int64 offset = 0, __int64 length = -1);
    static int remove(const char* filename);
    static int rename(const char* oldpath, const char * newpath);

    std::string mExpectedMD5;
    bool checkMd5() { return mExpectedMD5 == md5(); }
private:
    std::string mFilename;
    FILE*   fp;
    int     fd;
};




