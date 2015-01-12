
#pragma once

#include "serialization.h"
#include "tools/buffer.h"
#include "UfDef.h"
#include <list>
#include <string>

class UfPartitions : public Serialization
{
public:
    UfPartitions();
    virtual ~UfPartitions();

    static const int PIECE_LENGTH = 1024;
    static const unsigned int PART_MAGIC = 0x54524150;  // 'PART'

    virtual void Serialize(Archive& ar);
    virtual void Deserialize(Archive& ar);


    void create(__int64 size);

    void dump();
    void clear();
    __int64 size(); // 文件分片数。
    __int64 downloadedSize();   // 已下载分片数。

    std::string dumpGraphic();

public:
    class Range {
    public:
        __int64 offset;
        __int64 length;
    };
    void getEmptyRanges(std::list<Range>& out);
    bool isPieceReceived(__int64 offset);
    void setPieceReceived(__int64 offset, bool b = true);
    bool hasAllReceived();

public:
    unsigned int    mMagic;
    unsigned int    mVersion;
    std::string     md5;
    __int64         mSize;
    __int64         mPieceLength;
    tools::Buffer   mPieces;

};

