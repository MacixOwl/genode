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
#include <adl/TString.h>


using namespace adl;


namespace monkey::net::protocol {

const char* MAGIC = "mkOS";


Status send(TcpIo& tcpio, MsgType type, const void* data, adl::uint64_t dataLen) {
    Header header = makeHeader(adl::uint32_t(type), dataLen);

    const adl::uint32_t headerSize = sizeof(header);
    adl::uint64_t msgSize = headerSize + dataLen;

    adl::int64_t dataWrote = tcpio.send(&header, headerSize);
    if (data && dataLen)
        dataWrote += tcpio.send(data, dataLen);

    // Returns whether all msg sent.
    return dataWrote >= 0 && adl::uint64_t(dataWrote) == msgSize ? Status::SUCCESS : Status::NETWORK_ERROR;
}


Status send(TcpIo& tcpio, MsgType type, const adl::ByteArray& data) {
    return send(tcpio, type, data.data(), data.size());
}



Status sendHello(TcpIo& tcpio, const adl::ArrayList<adl::int64_t>& protocolVersions) {
    auto data = protocolVersions;
    for (auto& it : protocolVersions)
        it = adl::htonq(it);

    return send(tcpio, MsgType::Hello, data.data(), data.size() * sizeof(adl::int64_t));
}



Status recvHello(TcpIo& tcpio, adl::ArrayList<adl::int64_t>& out) {
    Msg* msg;
    Status status = recv(tcpio, &msg, MsgType::Hello);
    if (status != Status::SUCCESS)
        return status;

    out.clear();

    for (size_t i = 0; i < msg->header.length / sizeof(int64_t); i++) {
        void* p = msg->data + i * sizeof(int64_t);
        auto value = * (int64_t *) p;
        out.append( ntohq(value) );
    }

    adl::defaultAllocator.free(msg);

    return status;
}



Status sendAuth(TcpIo& tcpio, const adl::ByteArray& challenge) {
    return send(tcpio, MsgType::Auth, challenge);
}


Status sendResponse(TcpIo& tcpio, const uint32_t code, const adl::ByteArray& msg) {
    ByteArray data;
    data.reserve(8 + msg.size());
    
    (* (uint32_t*) &data[0]) = htonl(code);
    (* (uint32_t*) &data[4]) = htonl(uint32_t(msg.size()));
    memcpy(data.data() + 8, msg.data(), msg.size());

    Status status = send(tcpio, MsgType::Response, data);
    return status;
}


Status sendResponse(TcpIo& tcpio, const adl::uint32_t code, const adl::TString& msg) {
    return sendResponse(tcpio, code, adl::ByteArray {msg.c_str()});
}


Status sendResponse(TcpIo& tcpio, const adl::uint32_t code, const char* msg) {
    return sendResponse(tcpio, code, adl::ByteArray {msg});
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



Status recv(TcpIo& tcpio, Msg** msg, MsgType type) {
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
        received += adl::uint64_t(num_bytes);
    }



    if (type != MsgType::None && protocolMsg->header.type != adl::uint32_t(type)) {
        adl::defaultAllocator.free(protocolMsg);
        return Status::PROTOCOL_ERROR;
    }


    *msg = protocolMsg;
    return Status::SUCCESS;
}



Status recvResponse(TcpIo& tcpio, Response** response) {
    Msg* rawMsg = nullptr;
    Status libResult = recv(tcpio, &rawMsg, MsgType::Response);

    if (libResult != Status::SUCCESS) {
        return libResult;
    }


    *response = (Response*) rawMsg;


    (*response)->code = adl::ntohl((*response)->code);
    (*response)->msgLen = adl::ntohl((*response)->msgLen);

    return Status::SUCCESS;
}


Status sendCheckAvailMem(TcpIo& tcpio) {
    return send(tcpio, MsgType::CheckAvailMem);
}


Status sendFreeBlock(TcpIo& tcpio, adl::uint64_t blockId) {
    auto bId_netOrder = htonq(blockId);
    return send(tcpio, MsgType::FreeBlock, &bId_netOrder, sizeof(bId_netOrder));
}

}  // namespace monkey::net::protocol
