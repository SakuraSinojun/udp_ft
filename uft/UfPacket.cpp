
#include "UfPacket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

UfPacket::UfPacket(Archive& ar)
{
    Deserialize(ar);
}

UfPacket::UfPacket(const char* buffer, int len)
{
    fromBuffer(buffer, len);
}

UfPacket::UfPacket(int act, int sid)
    : sessionId(sid)
    , action(act)
    , piecebuffer(-1)
{
    memcpy((char*)&magic, UF_MAGIC, strlen(UF_MAGIC));
}

UfPacket::~UfPacket()
{
}

bool UfPacket::isValid()
{
    if (memcmp(&magic, UF_MAGIC, strlen(UF_MAGIC)) == 0)
        return true;
    return false;
}

bool UfPacket::isValidPacket(const char * buffer, int len)
{
    if (len < (int)strlen(UF_MAGIC))
        return false;

    if (memcmp(buffer + sizeof(int), UF_MAGIC, strlen(UF_MAGIC)) != 0)
        return false;

    return true;
}

void UfPacket::Serialize(Archive& ar)
{
    ar << magic << sessionId << action << piecebuffer;
}

void UfPacket::Deserialize(Archive& ar)
{
    ar >> magic >> sessionId >> action >> piecebuffer;
}

void RequestInfoPacket::Serialize(Archive& ar)
{
    UfPacket::Serialize(ar);
    ar << filename << piecelength;
}

void RequestInfoPacket::Deserialize(Archive& ar)
{
    UfPacket::Deserialize(ar);
    ar >> filename >> piecelength;
}




















