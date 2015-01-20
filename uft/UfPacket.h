
#pragma once

#include "serialization.h"
#include "UfTypes.h"
#include "tools/buffer.h"
#include <string>

#define UF_MAGIC    "UFPR"

#define UF_UNKNOWN  0x0
#define UF_HELLO    0x1
#define UF_HELLOACK 0x2
#define UF_HEARTBIT 0x3
#define UF_REQUESTINFO  0x4
#define UF_FILEINFO     0x5
#define UF_ENDSESSION   0x6
#define UF_ERROR        0x7
#define UF_REQUESTFILE  0x8
#define UF_FILECONTENT  0x9
#define UF_SENDOVER     0xA
#define UF_PIECEBUFFER  0xB
#define UF_PERCENT      0xC

class UfPacket : public Serialization {
public:
    UfPacket(Archive& ar);
    UfPacket(const char* buffer, int len);
    UfPacket(int act, int sid = -1);
    UfPacket(int act);
    virtual ~UfPacket();

    int     magic;
    int     sessionId;
    int     action;
    int     piecebuffer;

    bool isValid();
    static bool isValidPacket(const char * buffer, int len);

    virtual void Serialize(Archive& ar);
    virtual void Deserialize(Archive& ar);

private:
    UfPacket() {}
};


class HelloPacket : public UfPacket {
public:
    HelloPacket() : UfPacket(UF_HELLO, 0) {}

};

class HelloAckPacket : public UfPacket {
public:
    HelloAckPacket(int sid = -1) : UfPacket(UF_HELLOACK, sid) {}
};

class HeartbitPacket : public UfPacket {
public:
    HeartbitPacket(int sid = -1) : UfPacket(UF_HEARTBIT, sid) {}
};

class RequestInfoPacket : public UfPacket {
public:
    RequestInfoPacket(int sid = -1) : UfPacket(UF_REQUESTINFO, sid) {}
    std::string filename;
    __int64     piecelength;

    virtual void Serialize(Archive& ar);
    virtual void Deserialize(Archive& ar);
};

class FileInfoPacket : public UfPacket {
public:
    FileInfoPacket(int sid = -1) : UfPacket(UF_FILEINFO, sid) {}
    std::string filename;
    __int64     length;
    std::string md5;
    virtual void Serialize(Archive& ar) {
        UfPacket::Serialize(ar);
        ar << filename << length << md5;
    }
    virtual void Deserialize(Archive& ar) {
        UfPacket::Deserialize(ar);
        ar >> filename >> length >> md5;
    }
};

class ErrorPacket : public UfPacket {
public:
    ErrorPacket(int sid = -1) : UfPacket(UF_ERROR, sid) {}
    int     error;
    virtual void Serialize(Archive& ar) {
        UfPacket::Serialize(ar);
        ar << error;
    }
    virtual void Deserialize(Archive& ar) {
        UfPacket::Deserialize(ar);
        ar >> error;
    }
};

class RequestFilePacket : public UfPacket {
public:
    RequestFilePacket(int sid = -1) : UfPacket(UF_REQUESTFILE, sid) {}
    // __int64 partid; // 暂时还不确定有没有用。
    __int64 offset;
    __int64 length;
    virtual void Serialize(Archive& ar) {
        UfPacket::Serialize(ar);
        ar << offset << length;
    }
    virtual void Deserialize(Archive& ar) {
        UfPacket::Deserialize(ar);
        ar >> offset >> length;
    }
};

class FileContentPacket : public UfPacket {
public:
    FileContentPacket(int sid = -1) : UfPacket(UF_FILECONTENT, sid) {}
    __int64         offset;
    tools::Buffer   buffer;
    virtual void Serialize(Archive& ar) {
        UfPacket::Serialize(ar);
        ar << offset << buffer;
    }
    virtual void Deserialize(Archive& ar) {
        UfPacket::Deserialize(ar);
        ar >> offset >> buffer;
    }
};

class SendOverPacket : public UfPacket {
public:
    SendOverPacket(int sid = -1) : UfPacket(UF_SENDOVER, sid) {}
    int     request;        // 0 = request, 1 = ack

    static const int REQUEST = 0;
    static const int ACK     = 1;

    virtual void Serialize(Archive& ar) {
        UfPacket::Serialize(ar);
        ar << request;
    }
    virtual void Deserialize(Archive& ar) {
        UfPacket::Deserialize(ar);
        ar >> request;
    }
};

class PieceBufferPacket : public UfPacket {
public:
    PieceBufferPacket(int sid = -1) : UfPacket(UF_PIECEBUFFER, sid) {}
};

class EndSessionPacket : public UfPacket {
public:
    EndSessionPacket(int sid = -1) : UfPacket(UF_ENDSESSION, sid) {}
    int reason;
    virtual void Serialize(Archive& ar) {
        UfPacket::Serialize(ar);
        ar << reason;
    }
    virtual void Deserialize(Archive& ar) {
        UfPacket::Deserialize(ar);
        ar >> reason;
    }
};

class PercentPacket : public UfPacket {
public:
    PercentPacket(int sid = -1) : UfPacket(UF_PERCENT, sid) {}
    std::string filename;
    __int64     downloaded;
    __int64     total;
    virtual void Serialize(Archive& ar) {
        UfPacket::Serialize(ar);
        ar << filename << downloaded << total;
    }
    virtual void Deserialize(Archive& ar) {
        UfPacket::Deserialize(ar);
        ar >> filename >> downloaded >> total;
    }
};








