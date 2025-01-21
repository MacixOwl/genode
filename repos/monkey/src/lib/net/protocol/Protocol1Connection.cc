/*
 * Protocol V1 Connection
 *
 * Created on 2025.1.21 at Xiangzhou, Zhuhai, Guangdong
 * 
 * 
 * gongty  [at] tongji [dot] edu [dot] cn
 * 
 */


#include <monkey/net/protocol/Protocol1Connection.h>
#include <monkey/crypto/rc4.h>

namespace monkey::net {

using protocol::Msg;
using protocol::MsgType;
using protocol::Response;


Status Protocol1Connection::sendAuth(const adl::ByteArray& challenge) {
    return sendMsg(MsgType::Auth, challenge);
}


Status Protocol1Connection::recvAuth(adl::ByteArray& challenge) {
    protocol::Msg* msg = nullptr;
    Status status = recvMsg(&msg, MsgType::Auth);
    if (status != Status::SUCCESS)
        return status;

    if (!challenge.resize(msg->header.length)) {
        adl::defaultAllocator.free(msg);
        return Status::OUT_OF_RESOURCE;
    }

    adl::memcpy(challenge.data(), msg->data, challenge.size());
    adl::defaultAllocator.free(msg);
    return Status::SUCCESS;
}


Status Protocol1Connection::sendResponse(const uint32_t code, const adl::ByteArray& msg) {
    adl::ByteArray data;
    data.reserve(8 + msg.size());
    
    (* (uint32_t*) &data[0]) = adl::htonl(code);
    (* (uint32_t*) &data[4]) = adl::htonl(uint32_t(msg.size()));
    adl::memcpy(data.data() + 8, msg.data(), msg.size());

    Status status = sendMsg(MsgType::Response, data);
    return status;
}


Status Protocol1Connection::sendResponse(const adl::uint32_t code, const adl::TString& msg) {
    return sendResponse(code, adl::ByteArray {msg.c_str()});
}


Status Protocol1Connection::sendResponse(const adl::uint32_t code, const char* msg) {
    return sendResponse(code, adl::ByteArray {msg});
}



Status Protocol1Connection::recvResponse(Response** response) {
    Msg* rawMsg = nullptr;
    Status libResult = recvMsg(&rawMsg, MsgType::Response);

    if (libResult != Status::SUCCESS) {
        return libResult;
    }


    *response = (Response*) rawMsg;


    (*response)->code = adl::ntohl((*response)->code);
    (*response)->msgLen = adl::ntohl((*response)->msgLen);

    return Status::SUCCESS;
}


Status Protocol1Connection::sendCheckAvailMem() {
    return sendMsg(MsgType::CheckAvailMem);
}


Status Protocol1Connection::sendFreeBlock(adl::uint64_t blockId) {
    auto bId_netOrder = adl::htonq(blockId);
    return sendMsg(MsgType::FreeBlock, &bId_netOrder, sizeof(bId_netOrder));
}


Status Protocol1Connection::auth(const adl::ByteArray& key) {
    adl::ByteArray challenge;
    Status status = recvAuth(challenge);
    if (status != Status::SUCCESS)
        return status;

    crypto::rc4Inplace(challenge, key);
    if ((status = sendResponse(0, challenge)) != Status::SUCCESS) {
        return status;
    }

    protocol::Response* response = nullptr;
    status = recvResponse(&response);
    if (status != Status::SUCCESS)
        return status;

    if (response->code != 0)
        status = Status::PROTOCOL_ERROR;

    adl::defaultAllocator.free(response);
    return status;
}


Status Protocol1Connection::auth(
    const adl::ArrayList<adl::ByteArray>& appsKeyring, 
    const adl::ArrayList<adl::ByteArray>& memoryNodesKeyring
) {
    adl::ByteArray challenge { "fyt's score is A+" };

    Status status;
    if ( (status = sendAuth(challenge)) != Status::SUCCESS )
        return status;

    Response* response = nullptr;
    if ((status = recvResponse(&response)) != Status::SUCCESS)
        return status;

    adl::ByteArray cipher { response->data, response->msgLen };
    if (crypto::rc4Verify(appsKeyring, cipher, challenge) != -1) {
        nodeType = NodeType::App;
    }
    else if (crypto::rc4Verify(memoryNodesKeyring, cipher, challenge) != -1) {
        nodeType = NodeType::MemoryNode;
    }
    else {
        nodeType = NodeType::Unknown;
    }

    sendResponse(!!(nodeType == NodeType::Unknown), nullptr);

    adl::defaultAllocator.free(response);
    response = nullptr;
    return Status::SUCCESS;
}


}
