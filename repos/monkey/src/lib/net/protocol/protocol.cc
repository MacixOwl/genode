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


#include <monkey/Status.h>
#include <monkey/net/TcpIo.h>
#include <monkey/net/protocol.h>
#include <adl/config.h>
#include <adl/Allocator.h>


namespace monkey::net::protocol {

const char* MAGIC = "mkOS";


Status send(TcpIo& tcpio, adl::uint32_t type, adl::uint8_t* data, adl::uint64_t dataLen) {
    Header header = makeHeader(type, dataLen);

    const adl::uint32_t headerSize = sizeof(header);
    adl::uint64_t msgSize = headerSize + dataLen;

    adl::int64_t dataWrote = tcpio.send(&header, headerSize) + tcpio.send(data, dataLen);

    return dataWrote >= 0 && adl::uint64_t(dataWrote) == msgSize ? Status::SUCCESS : Status::NETWORK_ERROR;  // Whether all msg sent.
}


Status recvHeader(TcpIo& tcpio, Header* header) {

    adl::uint64_t received = 0;
    while(received < sizeof(Header)){
        adl::int64_t num_bytes = tcpio.recv( ((char*) header) + received, sizeof(Header) - received);
        if(num_bytes == -1) {
            Genode::error("[Receive_header] Read failed.");
            return Status::NETWORK_ERROR;
        }
        received += num_bytes;
    }

    
    // Check magic.

    if ( *(const adl::int32_t*) header->magic != *(const adl::int32_t*) MAGIC) {
        Genode::error("[Receive Header] Header mismatch!");
        return Status::PROTOCOL_ERROR;
    }


    // Data from network is in network's byte-order. Convert it to native one.

    header->type = adl::ntohl(header->type);
    header->length = adl::ntohq(header->length);

    return Status::SUCCESS;
}



Status recv(TcpIo& tcpio, Msg** msg) {
    Status libResult;

    Header header;
    if ((libResult = recvHeader(tcpio, &header)) != Status::SUCCESS) {
        return libResult;
    }

    
    adl::uint64_t msgSize = (adl::uint64_t) header.length + sizeof(Header);
    auto protocolMsg = adl::defaultAllocator.alloc<Msg>(1, true, msgSize);

    if (protocolMsg == nullptr) {
        // out of resource.
        return Status::OUT_OF_RESOURCE;
    }

    adl::memcpy(protocolMsg, &header, sizeof(Header));

    adl::uint64_t received = 0;
    while(received < header.length){
        adl::int64_t num_bytes = tcpio.recv(((char*) protocolMsg) + sizeof(Header) + received, header.length - received);
        if(num_bytes == -1){ 
            Genode::error("[Recv_msg] Read failed.");
            adl::defaultAllocator.free(protocolMsg);
            return Status::NETWORK_ERROR;
        }
        received += num_bytes;
    }


    *msg = protocolMsg;
    return Status::SUCCESS;
}



Status recvResponse(TcpIo& tcpio, Response** response) {
    Msg* rawMsg = nullptr;
    Status libResult = recv(tcpio, &rawMsg);

    if (libResult != Status::SUCCESS) {
        return libResult;
    }

    if (rawMsg->header.type != (adl::uint32_t) MsgType::RESPONSE) {
        adl::defaultAllocator.free(rawMsg);
        return Status::PROTOCOL_ERROR;
    }

    *response = (Response*) rawMsg;


    (*response)->code = adl::ntohl((*response)->code);
    (*response)->msgLen = adl::ntohl((*response)->msgLen);

    return Status::SUCCESS;
}


}  // namespace monkey::net::protocol
