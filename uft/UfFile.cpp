
#include "UfFile.h"
#include "UfConvert.h"
#include "tools/buffer.h"
#include "md5.h"
#include "tools/logging.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// #define USE_SYSOPEN

UfFile::UfFile()
    : fp(NULL)
    , fd(-1)
{
}

UfFile::UfFile(std::string filename)
    : fp(NULL)
{ 
    init(filename.c_str()); 
}

UfFile::~UfFile()
{
    close();
}

void UfFile::init(const char * filename)
{
    mFilename = filename;
    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }
}

bool UfFile::isExists()
{
    return isExists(mFilename.c_str());
}

bool UfFile::isExists(const char * filename)
{
    struct stat st;
    int ret = stat(filename, &st);
    if (ret == 0) {
        return true;
    }
    return false;
}


off_t UfFile::getSize()
{
    return getSize(mFilename.c_str());
}

off_t UfFile::getSize(const char * filename)
{
    struct stat st;
    int ret = stat(filename, &st);
    if (ret == 0) {
        return st.st_size;
    }
    return -1;
}

std::string UfFile::md5(const char * filename, __int64 offset, __int64 length)
{
    off_t sz = getSize(filename);
    if (sz < 0 || sz < offset || offset + length > sz) {
        return "";
    }
    FILE* fp = fopen(filename, "rb");
    if(fp == NULL) {
        return "";
    }
    fseek(fp, offset, SEEK_SET);

    if (length < 0) {
        length = sz - offset;
    }

    char    result[16] = {0};

    md5_state_t state;
    md5_init(&state);

    for (__int64 i = 0; i < length; /* */) {
        char    buffer[1024];
        size_t l = sizeof(buffer);
        if ((__int64)l > length - i) {
            l = (size_t)(length - i);
        }
        size_t ret = fread(buffer, 1, l, fp);
        if (ret <= 0) {
            break;
        }
        md5_append(&state, (const md5_byte_t *)buffer, (int)ret);
        i += ret;
    }
    md5_finish(&state, (md5_byte_t*)result);
    fclose(fp);
    return UfConvert::data2Hex(result, 16);
}

std::string UfFile::md5()
{
    return md5(mFilename.c_str());
}

int UfFile::remove()
{
    return remove(mFilename.c_str());
}

int UfFile::remove(const char* filename)
{
    return unlink(filename);
}

int UfFile::rename(const char * newpath)
{
    return UfFile::rename(mFilename.c_str(), newpath);
}

int UfFile::rename(const char* oldpath, const char * newpath)
{
    return ::rename(oldpath, newpath);
}

int UfFile::create(__int64 size)
{
    if (isExists()) {
        return EEXIST;
    }
    FILE* fp = fopen(mFilename.c_str(), "w+");
    if (fp == NULL) {
        perror("fopen");
        return errno;
    }
    fclose(fp);

    int ret = truncate(mFilename.c_str(), size);
    if (ret != 0) {
        perror("truncate");
    }
    return ret;
}

int UfFile::open()
{
#ifdef USE_SYSOPEN
    if (fd != -1)
        return -1;
    fd = ::open(mFilename.c_str(), O_RDWR);
    if (fd == -1) {
        perror("open");
        return errno;
    }
    return 0;
#else
    if (fp != NULL)
        return -1;

    fp = fopen(mFilename.c_str(), "rb+");
    if (fp == NULL) {
        perror("fopen");
        return errno;
    }
#endif
    return 0;
}

int UfFile::close()
{
#ifdef USE_SYSOPEN
    if (fd != -1) {
        ::close(fd);
        fd = -1;
    }
#else
    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }
#endif
    return 0;
}

int UfFile::read(__int64 offset, char* buffer, int len)
{
#ifdef USE_SYSOPEN
    if (fd == -1) {
        FATAL();
        return -1;
    }
    lseek(fd, offset, SEEK_SET);
    int total = 0;
    while (total < len) {
        int ret = ::read(fd, (char*)buffer + total, len - total);
        if (ret <= 0)
            break;
        total += ret;
    }
    return total;
#else
    if (fp == NULL) {
        FATAL();
        return -1;
    }
    fseek(fp, offset, SEEK_SET);
    int total = 0;
    while (total < len) {
        int ret = fread((char *)buffer + total, 1, len - total, fp);
        if (ret <= 0)
            break;
        total += ret;
    }
    return total;
#endif
}

int UfFile::read(__int64 offset, __int64 piecelen, int pieces, ReadCallback& cb)
{
    if ((offset % piecelen) != 0) {
        FATAL() << "not a partitions request.";
        return -1;
    }

#ifdef USE_SYSOPEN
    if (fd == -1) {
        FATAL();
        return -1;
    }
    lseek(fd, offset, SEEK_SET);
#else
    if (fp == NULL) {
        FATAL();
        return -1;
    }
    fseek(fp, offset, SEEK_SET);
#endif

    int times = pieces;
    __int64 off = offset;
    while (times > 0) {
        tools::Buffer    buf;
        buf.assign(piecelen);
        int count = 0;
        while (count < piecelen) {
#ifdef USE_SYSOPEN
            int ret = ::read(fd, (char*)buf + count, piecelen - count);
#else
            int ret = fread((char *)buf + count, 1, piecelen - count, fp);
#endif
            if (ret <= 0) {
                perror("fread");
                break;
            }
            count += ret;
        }
        if (count > 0) {
            if (!cb.onRead(off, buf, count)) {
                break;
            }
        }
        times--;
        if (count < piecelen)
            break;
        off += count;
    }
    return pieces - times;
}

void UfFile::write(__int64 offset, const char * buffer, int len)
{
#ifdef USE_SYSOPEN
    if (fd == -1) {
        FATAL();
        return;
    }
    lseek(fd, offset, SEEK_SET);
    if(::write(fd, buffer, len) == -1) {
        perror("write");
    }
#else
    if (fp == NULL) {
        FATAL();
        return;
    }
    fseek(fp, offset, SEEK_SET);
    if (fwrite(buffer, 1, len, fp) < (size_t)len) {
        perror("fwrite");
    }
    fflush(fp);
#endif
}



















