
#pragma once


#include "tools/buffer.h"
#include "UfTypes.h"
#include <list>

class UfPieceBuffer
{
public:
    UfPieceBuffer(int size);
    virtual ~UfPieceBuffer();

    class Piece {
    public:
        Piece();
        ~Piece();
        Piece(const Piece& o);
        Piece& operator=(const Piece& o);

        __int64     offset;
        tools::Buffer    buffer;
    };

    bool push(__int64 offset, const char * buffer, int len);
    bool pop(__int64& offset, tools::Buffer& buf);

    int  pit() { return mSize - (int)mBuffers.size(); }
    int  size() { return (int)mBuffers.size(); }
    void clear();

private:
    std::list<Piece*>   mBuffers;
    int                 mSize;
};

