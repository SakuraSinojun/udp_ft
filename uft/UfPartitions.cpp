
#include "UfPartitions.h"
#include "logging.h"

// 每次请求的最大分片数
#define MAX_PIECE_COUNT 0

// 返回ranges时每个range的最大大小
#define MAX_RANGE_LENGTH  (2 * 1024 * PIECE_LENGTH)

UfPartitions::UfPartitions()
    : mMagic(PART_MAGIC)
    , mVersion(0x10001)
{
}

UfPartitions::~UfPartitions()
{
}

void UfPartitions::Serialize(Archive& ar)
{
    ar << mMagic << mVersion << md5 << mSize << mPieceLength << mPieces;
}

void UfPartitions::Deserialize(Archive& ar)
{
    ar >> mMagic >> mVersion >> md5 >> mSize >> mPieceLength >> mPieces;
    if (mMagic != PART_MAGIC) {
        FATAL();
    }
}

void UfPartitions::create(__int64 size)
{
    // md5.clear();

    if (size <= 0) {
        FATAL();
        return;
    }

    mSize = size;
    mPieceLength = PIECE_LENGTH;

    __int64   count = (size + PIECE_LENGTH - 1) / PIECE_LENGTH;
    mPieces.assign((count + 7) / 8, 0);
}

void UfPartitions::dump()
{
    int count = 0;

    std::string temp = "";
    for (int i = 0; i < mPieces.length(); i++) {
        char c = mPieces[i];
        for (int j = 7; j >= 0; j--) {
            int cc = (c >> j) & 0x1;
            if (cc == 1)
                temp += "o";
            else
                temp += ".";
            count++;
            if (count >= 160) {
                temp += "\r\n";
                count = 0;
            }
        }
    }
    ILOG() << "Partitions: \r\n" << temp;
}

void UfPartitions::getEmptyRanges(std::list<Range>& out)
{
    out.clear();

    Range   range;

    range.offset = -1;
    range.length = -1;

    __int64   count = (mSize + mPieceLength - 1) / mPieceLength;
    for(__int64 i = 0; i < count; i+= 8) {
        char c = mPieces[i / 8];
        for (int j = 0; j < 8; j++) {
            int cc = (c >> (7 - j)) & 0x1;

            if (range.offset == -1) {
                if (cc == 0)
                    range.offset = (i + j) * mPieceLength;
            } else {
                __int64 l = (i + j) * mPieceLength - range.offset;
                if (cc != 0 || (MAX_RANGE_LENGTH > 0 && l >= MAX_RANGE_LENGTH)) {
                    range.length = l;
                    if (range.offset + range.length > mSize)
                        range.length = mSize - range.offset;
                    out.push_back(range);
                    if (cc == 0) {
                        range.offset = (i + j) * mPieceLength;
                    } else {
                        range.offset = -1;
                    }
                    range.length = -1;
                    if (MAX_PIECE_COUNT > 0) {
                        if (out.size() >= MAX_PIECE_COUNT)
                            return;
                    }
                }
            }
#if 0
            // {{{
            if (cc == 0 && range.offset == -1) {
                range.offset = (i + j) * mPieceLength;
            } else if (cc != 0 && range.offset == -1) {
            } else if (cc == 0 && range.offset > 0) {
                if (MAX_RANGE_LENGTH > 0) {
                    __int64 l = (i + j) * mPieceLength - range.offset;
                    if (l >= MAX_RANGE_LENGTH) {
                        range.length = l;
                        if (range.offset + range.length > mSize)
                            range.length = mSize - range.offset;
                        out.push_back(range);
                        range.offset = -1;
                        range.length = -1;
                        if (MAX_PIECE_COUNT > 0) {
                            if (out.size() >= MAX_PIECE_COUNT)
                                return;
                        }
                    }
                }
            } else /* cc != 0 && range.offset > 0 */ {
                __int64 off = (i + j) * mPieceLength;
                range.length = off - range.offset;
                if (range.offset + range.length > mSize) {
                    range.length = mSize - range.offset;
                }
                out.push_back(range);
                range.offset = -1;
                range.length = -1;
                if (MAX_PIECE_COUNT > 0) {
                    if (out.size() >= MAX_PIECE_COUNT)
                        return;
                }
            }
            // }}}
#endif
        } // for j = 0 to 7
    } // for i = 0 to count
    
    if (range.offset >= 0) {
        range.length = mSize - range.offset;
        out.push_back(range);
    }
}

bool UfPartitions::hasAllReceived()
{
    __int64   count = (mSize + mPieceLength - 1) / mPieceLength;
    for (__int64 i = 0; i < count; i += 8) {
        char c = mPieces[i / 8];
        for (int j = 0; j < 8; j++) {
            if (i + j >= count)
                break;
            int cc = (c >> (7 - j)) & 0x1;
            if (cc == 0)
                return false;
        }
    }
    return true;
}

bool UfPartitions::isPieceReceived(__int64 offset)
{
    __int64 pieceindex = offset / mPieceLength;
    __int64 bindex = pieceindex / 8;
    __int64 bshift = 7 - (pieceindex % 8);
    if (bindex > mPieces.length()) {
        FATAL();
        return false;
    }
    int bit = (mPieces[bindex] >> bshift) & 0x1;
    return (bit != 0);
}

void UfPartitions::setPieceReceived(__int64 offset, bool b)
{
    __int64 pieceindex = offset / mPieceLength;
    __int64 bindex = pieceindex / 8;
    __int64 bshift = 7 - (pieceindex % 8);
    if (bindex > mPieces.length()) {
        FATAL();
        return;
    }
    char c = mPieces[bindex];
    if (b) {
        int mask = 0x1;
        mask = mask << bshift;
        c = c | mask;
    } else {
        int mask = ~0x1;
        mask = mask << bshift;
        c = c & mask;
    }
    mPieces[bindex] = c;
}

__int64 UfPartitions::downloadedSize()
{
    __int64 sum = 0;
    __int64 count = (mSize + mPieceLength - 1) / mPieceLength;
    for (__int64 i = 0; i < count; i += 8) {
        char c = mPieces[i / 8];
        for (int j = 0; j < 8; j++) {
            if (i + j >= count)
                break;
            int cc = (c >> (7 - j)) & 0x1;
            if (cc == 1)
                sum++;
        }
    }
    return sum;
}

void UfPartitions::clear()
{
    create(mSize);
}

__int64 UfPartitions::size()
{
    return (mSize + mPieceLength - 1) / mPieceLength;
}

std::string UfPartitions::dumpGraphic()
{
    std::string temp = "";
    __int64 pieces = (mSize + mPieceLength - 1) / mPieceLength;
    for (int i = 0; i < mPieces.length(); i++) {
        char c = mPieces[i];
        if (c == 0) {
            temp += UFGRAPHIC_EMPTY;
        } else if (c == -1) {
            temp += UFGRAPHIC_COMPLETED;
        } else {
            if (i == mPieces.length() - 1) {
                int count = pieces - ((i - 1) * 8);
                int sum = 0;
                for (int j = 0; j < count; j++) {
                    int d = (c >> (7 - j)) & 0x1;
                    sum += d;
                }
                if (sum == count) {
                    temp += UFGRAPHIC_COMPLETED;
                } else if (sum == 0) {
                    temp += UFGRAPHIC_EMPTY;
                } else {
                    temp += UFGRAPHIC_UNCOMPLETED;
                }
            } else {
                temp += UFGRAPHIC_UNCOMPLETED;
            }
        }
    }
    return temp;
}















