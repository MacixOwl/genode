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

#include <monkey/Status.h>
#include <monkey/net/TcpIo.h>

namespace monkey::net::protocol {

extern const char* MAGIC;
const adl::size_t MAGIC_LEN = 4;

enum class MsgType : adl::uint32_t {
    RESPONSE = 0xA001
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
    adl::uint32_t type, 
    adl::uint8_t* data, 
    adl::uint64_t dataLen
);


Status recvHeader(TcpIo&, Header*);


/**
 * The caller is responsible for freeing `msg`
 * when LibResult is SUCCESS.
 */
Status recv(TcpIo& tcpio, Msg** msg);


/**
 * The caller is responsible for freeing `response`  
 * when LibResult is SUCCESS.
 */
Status recvResponse(TcpIo& tcpio, Response** response);

    

}
