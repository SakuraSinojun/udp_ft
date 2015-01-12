
#pragma once

#include "tools/serialize/serialization.h"
#include "tools/buffer.h"
#include "UfTypes.h"
#include <string.h>
#include <string>

#define SF_MAGIC    0x52504653  // "SFPR"

#define SF_UNKNOWN  0x0
#define SF_REQUESTINFO  0x1
#define SF_FILEINFO     0x2
#define SF_REQUESTFILE  0x3
#define SF_FILECONTENT  0x4
#define SF_ENDSESSION   0x5
#define SF_SENDOVER     0x6

class SfPacket : public Serialization {
public:
    SfPacket() : action(SF_UNKNOWN), magic(SF_MAGIC) {}
    SfPacket(int act) : action(act), magic(SF_MAGIC) {}

    int  action;

    virtual void Serialize(Archive& ar) {
        ar << magic << action;
    }
    virtual void Deserialize(Archive& ar) {
        ar >> magic >> action;
    }

    bool isValid() { return (magic == SF_MAGIC); }
    static bool isValidPacket(const char * buffer, int len) {
        if (len < (int)sizeof(int) * 2)
            return false;
        unsigned int m = *(unsigned int*)(buffer + sizeof(int));
        if (m != SF_MAGIC)
            return false;
        return true;
    }
private:
    unsigned int magic;
};

class SfRequestInfoPacket : public SfPacket {
public:
    SfRequestInfoPacket() : SfPacket(SF_REQUESTINFO) {}
    std::string     filename;

    virtual void Serialize(Archive& ar) {
        SfPacket::Serialize(ar);
        ar << filename;
    }
    virtual void Deserialize(Archive& ar) {
        SfPacket::Deserialize(ar);
        ar >> filename;
    }
};

class SfFileInfoPacket : public SfPacket {
public:
    SfFileInfoPacket() : SfPacket(SF_FILEINFO) {}
    std::string filename;
    __int64     filesize;
    std::string md5;

    virtual void Serialize(Archive& ar) {
        SfPacket::Serialize(ar);
        ar << filename << filesize << md5;
    }
    virtual void Deserialize(Archive& ar) {
        SfPacket::Deserialize(ar);
        ar >> filename >> filesize >> md5;
    }
};

class SfRequestFilePacket : public SfPacket {
public:
    SfRequestFilePacket() : SfPacket(SF_REQUESTFILE) {}
    __int64   offset;
    __int64   length;
    virtual void Serialize(Archive& ar) {
        SfPacket::Serialize(ar);
        ar << offset << length;
    }
    virtual void Deserialize(Archive& ar) {
        SfPacket::Deserialize(ar);
        ar >> offset >> length;
    }
};

class SfFileContentPacket : public SfPacket {
public:
    SfFileContentPacket() : SfPacket(SF_FILECONTENT) {}
    __int64       offset;
    tools::Buffer   buffer;
    virtual void Serialize(Archive& ar) {
        SfPacket::Serialize(ar);
        ar << offset << buffer;
    }
    virtual void Deserialize(Archive& ar) {
        SfPacket::Deserialize(ar);
        ar >> offset >> buffer;
    }
};

class SfEndSessionPacket : public SfPacket {
public:
    SfEndSessionPacket() : SfPacket(SF_ENDSESSION) {}
    int     result;

    virtual void Serialize(Archive& ar) {
        SfPacket::Serialize(ar);
        ar << result;
    }
    virtual void Deserialize(Archive& ar) {
        SfPacket::Deserialize(ar);
        ar >> result;
    }
};

class SfSendOverPacket : public SfPacket {
public:
    SfSendOverPacket() : SfPacket(SF_SENDOVER) {}
};





