/*
 * Monkey Network Protocol
 *
 * Created on 2024.12 at Minhang, Shanghai
 * 
 * 
 * feng.yt [at]  sjtu  [dot] edu [dot] cn
 * gongty  [at] tongji [dot] edu [dot] cn
 * 
 */


#pragma once

#include <adl/sys/types.h>
#include <adl/string.h>
#include <adl/TString.h>
#include <adl/arpa/inet.h>

#include <adl/Allocator.h>
#include <adl/collections/ArrayList.hpp>

#include <monkey/Status.h>
#include <monkey/net/TcpIo.h>


namespace monkey::net::protocol {

const adl::int64_t VERSION = 1;

extern const char* MAGIC;
const adl::size_t MAGIC_LEN = 4;

enum class MsgType : adl::uint32_t {
    None = 0x0000,

    Response = 0xA001,

    Hello = 0x1000,
    Auth = 0x1001,

    MemoryNodeClockIn = 0x2001,
    MemoryNodeClockOut = 0x2002,
    MemoryNodeHandover = 0x2003,
    LocateMemoryNodes = 0x2004,

    TryAlloc = 0x3001,
    ReadBlock = 0x3002,
    WriteBlock = 0x3003,
    CheckAvailMem = 0x3004,
    FreeBlock = 0x3005,

    PingPong = 0x4001
};


inline bool magicMatch(adl::uint8_t magic[]) {
    return * (adl::uint32_t*) MAGIC == * (adl::uint32_t*) magic;
}


struct Header {
    union {
        adl::uint8_t magic[4];

        // You can treat it as 32-bit integer.
        adl::uint32_t magicI32 = [] () constexpr {return * (adl::uint32_t*) MAGIC;} ();  
    } __packed;

    adl::uint32_t type;
    adl::uint64_t length;  // Tell byte-order whenever use.
} __packed;


/**
 * Common message.
 */
struct Msg {
    Header header;
    adl::uint8_t data[0];
} __packed;


struct Response {
    Header header;
    adl::uint32_t code;
    adl::uint32_t msgLen;

    union {
        const char msg[0];  // Not null-terminated.
        adl::uint8_t data[0];
    };

    adl::uint8_t msgData[0];  // Optional.
} __packed;


inline void makeHeader(
    Header* header, 
    adl::uint32_t type, 
    adl::uint64_t length, 
    bool netOrder = true,
    bool copyMagic = true
) {
    if (copyMagic) {
        adl::memcpy(header->magic, MAGIC, MAGIC_LEN);
    }

    header->type = netOrder ? adl::htonl(type) : type;
    header->length = netOrder ? adl::htonq(length) : length;
}


inline Header makeHeader(adl::uint32_t type, adl::uint64_t length, bool netOrder = true) {
    Header header;
    makeHeader(&header, type, length, netOrder, false);
    return header;  // We believe NRV would optimize this.
}


Status send(
    TcpIo&,
    MsgType type, 
    const adl::uint8_t* data, 
    adl::uint64_t dataLen
);


Status send(
    TcpIo&,
    MsgType type, 
    const adl::ByteArray& data
);


Status sendAuth(TcpIo&, const adl::ByteArray& challenge);

Status sendResponse(TcpIo&, const adl::uint32_t code, const adl::ByteArray& msg);
Status sendResponse(TcpIo&, const adl::uint32_t code, const adl::TString& msg);
Status sendResponse(TcpIo&, const adl::uint32_t code, const char* msg);


Status recvHeader(TcpIo&, Header*);


/**
 * The caller is responsible for freeing `msg`
 * when monkey::Status is SUCCESS.
 *
 * @param type Ensure type. Set to MsgType::None to disable this check.
 */
Status recv(TcpIo& tcpio, Msg** msg, MsgType type = MsgType::None);


/**
 * The caller is responsible for freeing `response`  
 * when monkey::Status is SUCCESS.
 */
Status recvResponse(TcpIo& tcpio, Response** response);

    



}
