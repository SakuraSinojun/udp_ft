
#include "UfPieceBuffer.h"
#include "tools/logging.h"


UfPieceBuffer::UfPieceBuffer(int size)
    : mSize(size)
{
}

UfPieceBuffer::~UfPieceBuffer()
{
    std::list<Piece*>::iterator it;
    for (it = mBuffers.begin(); it != mBuffers.end(); it++) {
        delete *it;
    }
    mBuffers.clear();
}

bool UfPieceBuffer::push(__int64 offset, const char * buffer, int len)
{
    // RUN_HERE() << "size: " << mBuffers.size() << " / " << mSize;
    if ((int)mBuffers.size() >= mSize)
        return false;
    Piece* p = new Piece();
    p->offset = offset;
    p->buffer.assign(buffer, len);
    mBuffers.push_back(p);
    return true;
}

bool UfPieceBuffer::pop(__int64& offset, tools::Buffer& buf)
{
    offset = -1;
    buf.assign(0);
    if (mBuffers.empty())
        return false;
    Piece* b = mBuffers.front();
    mBuffers.pop_front();
    offset = b->offset;
    buf = b->buffer;
    delete b;
    return true;
}


void UfPieceBuffer::clear()
{
    std::list<Piece*>::iterator it;
    for (it = mBuffers.begin(); it != mBuffers.end(); it++) {
        delete *it;
    }
    mBuffers.clear();
}


UfPieceBuffer::Piece::Piece()
    : offset(-1)
    , buffer(0)
{
}

UfPieceBuffer::Piece::~Piece()
{
}

UfPieceBuffer::Piece::Piece(const UfPieceBuffer::Piece& o)
{
    offset = o.offset;
    buffer = o.buffer;
}

UfPieceBuffer::Piece& UfPieceBuffer::Piece::operator=(const Piece& o)
{
    offset = o.offset;
    buffer = o.buffer;
    return *this;
}












