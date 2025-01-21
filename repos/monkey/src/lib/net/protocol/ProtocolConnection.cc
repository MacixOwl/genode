/*
 * Protocol Connection Base
 *
 * Created on 2025.1.21 at Xiangzhou, Zhuhai, Guangdong
 * 
 * 
 * gongty  [at] tongji [dot] edu [dot] cn
 * 
 */

#include <monkey/net/protocol/ProtocolConnection.h>


namespace monkey::net {

using protocol::MsgType;
using protocol::Msg;
using protocol::Header;



Status ProtocolConnection::sendMsg(MsgType type, const void* data, adl::uint64_t dataLen) {
    Header header = makeHeader(adl::uint32_t(type), dataLen);

    const adl::uint32_t headerSize = sizeof(header);
    adl::uint64_t msgSize = headerSize + dataLen;

    adl::int64_t dataWrote = send(&header, headerSize);
    if (data && dataLen)
        dataWrote += send(data, dataLen);

    // Returns whether all msg sent.
    return dataWrote >= 0 && adl::uint64_t(dataWrote) == msgSize ? Status::SUCCESS : Status::NETWORK_ERROR;
}


Status ProtocolConnection::sendMsg(MsgType type, const adl::ByteArray& data) {
    return sendMsg(type, data.data(), data.size());
}




Status ProtocolConnection::recvHeader(Header* header) {
    adl::uint64_t received = 0;
    while(received < sizeof(Header)){
        adl::int64_t num_bytes = recv( ((char*) header) + received, sizeof(Header) - received);
        if(num_bytes == -1) {
            Genode::error("[Receive_header] Read failed.");
            return Status::NETWORK_ERROR;
        }
        received += uint64_t(num_bytes);
    }

    
    // Check magic.

    if ( !magicMatch(header->magic) ) {
        Genode::error("[Receive Header] Header mismatch!");
        return Status::PROTOCOL_ERROR;
    }


    // Data from network is in network's byte-order. Convert it to native one.

    header->type = adl::ntohl(header->type);
    header->length = adl::ntohq(header->length);

    return Status::SUCCESS;
}


Status ProtocolConnection::recvMsg(Msg** msg, MsgType type) {
    Status libResult;

    Header header;
    if ((libResult = recvHeader(&header)) != Status::SUCCESS) {
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
        adl::int64_t num_bytes = recv(((char*) protocolMsg) + sizeof(Header) + received, header.length - received);
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


Status ProtocolConnection::sendHello(const adl::ArrayList<adl::int64_t>& protocolVersions) {
    auto data = protocolVersions;
    for (auto& it : protocolVersions)
        it = adl::htonq(it);

    return sendMsg(MsgType::Hello, data.data(), data.size() * sizeof(adl::int64_t));
}



Status ProtocolConnection::recvHello(adl::ArrayList<adl::int64_t>& out) {
    Msg* msg;
    Status status = recvMsg(&msg, MsgType::Hello);
    if (status != Status::SUCCESS)
        return status;

    out.clear();

    for (size_t i = 0; i < msg->header.length / sizeof(int64_t); i++) {
        void* p = msg->data + i * sizeof(int64_t);
        adl::int64_t value = * (int64_t *) p;
        out.append( adl::ntohq(value) );
    }

    adl::defaultAllocator.free(msg);

    return status;
}



}
