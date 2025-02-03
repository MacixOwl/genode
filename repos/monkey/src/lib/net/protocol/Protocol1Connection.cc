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


Status Protocol1Connection::sendResponse(
    const adl::uint32_t code,
    const adl::size_t msgLen,
    const void* msg,
    const adl::size_t paimonLen,
    const void* paimon
) {
    adl::ByteArray data;
    data.resize(8 + msgLen + paimonLen);
    (* (uint32_t*) &data[0]) = adl::htonl(code);
    (* (uint32_t*) &data[4]) = adl::htonl(uint32_t(msgLen));
    adl::memcpy(data.data() + 8, msg, msgLen);
    adl::memcpy(data.data() + 8 + msgLen, paimon, paimonLen);
    return sendMsg(MsgType::Response, data);
}


Status Protocol1Connection::sendResponse(const uint32_t code, const adl::ByteArray& msg) {
    adl::ByteArray data;
    data.resize(8 + msg.size());
    
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

// ------ 0x1001 : Auth ------

Status Protocol1Connection::auth(
    const adl::HashMap<adl::int64_t, adl::ByteArray>& appsKeyring, 
    const adl::ArrayList<adl::ByteArray>& memoryNodesKeyring
) {
    adl::ByteArray challenge { "fyt's score is A+" };  // TODO

    Status status;
    if ( (status = sendAuth(challenge)) != Status::SUCCESS )
        return status;

    Response* response = nullptr;
    if ((status = recvResponse(&response)) != Status::SUCCESS)
        return status;

    adl::ByteArray cipher { response->msg, response->msgLen };

    bool verified = false;

    // check if app
    
    for (const auto& it : appsKeyring) {
        verified = crypto::rc4Verify(it.second, challenge, cipher);
        if (verified) {
            nodeId = it.first;
            break;
        }
    }

    // check if memory node
    
    if (!verified) {
        if (crypto::rc4Verify(memoryNodesKeyring, cipher, challenge) != -1) {
            nodeType = NodeType::MemoryNode;
            verified = true;
        }
        else {
            nodeType = NodeType::Unknown;
        }
    }

    sendResponse(!!(nodeType == NodeType::Unknown), nullptr);

    adl::defaultAllocator.free(response);
    response = nullptr;
    return Status::SUCCESS;
}



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

// ------ 0x1100 : Get Identity Keys ------

struct GetIdentityKeysKeyHeaderPacked {
    adl::int8_t nodeType;  // 0 for App, 1 for Memory
    adl::int8_t keyType;  // 0 for RC4, 1 for RSA
    adl::int8_t reserved0[2] = {0};

    adl::int64_t offset; 
    adl::int32_t len; 

    adl::int64_t id;
    adl::int8_t reserved1[16] = {0}; 
} __packed;


Status Protocol1Connection::ReplyGetIdentityKeysParams::addKey(
    NodeType nodeType,
    KeyType keyType,
    const adl::ByteArray& key,
    adl::int64_t id
) {

    // check params

    if (nodeType != NodeType::App && nodeType != NodeType::MemoryNode)
        return Status::INVALID_PARAMETERS;
    
    if (keyType != KeyType::RC4)  // only RC4 allowed.
        return Status::INVALID_PARAMETERS;
    
    if (key.size() > INT32_MAX)
        return Status::INVALID_PARAMETERS;
    

    // allocate header

    adl::size_t headerSize = sizeof(GetIdentityKeysKeyHeaderPacked);

    if ( ! keyHeaders.resize(keyHeaders.size() + headerSize) ) {
        return Status::OUT_OF_RESOURCE;
    }

    auto head = (GetIdentityKeysKeyHeaderPacked*) (keyHeaders.data() + keyHeaders.size() - headerSize);

    adl::memset(head, 0, headerSize);

    head->nodeType = (adl::int8_t) nodeType;
    head->keyType = (adl::int8_t) keyType;
    head->offset = (adl::int64_t) keys.size();
    head->len = (adl::int32_t) key.size();
    head->id = id;

    if (!keys.append(key.data(), key.size())) {
        keyHeaders.resize(keyHeaders.size() - headerSize);
        return Status::OUT_OF_RESOURCE;
    }

    return Status::SUCCESS;
}


Status Protocol1Connection::sendGetIdentityKeys() {
    return sendMsg(MsgType::GetIdentityKeys);
}


Status Protocol1Connection::replyGetIdentityKeys(const ReplyGetIdentityKeysParams& params) {

    adl::ByteArray msg;
    
    if (!msg.resize(8 + params.keyHeaders.size() + params.keys.size()))
        return Status::OUT_OF_RESOURCE;

    adl::uint64_t nkeys = params.keyHeaders.size() / sizeof(GetIdentityKeysKeyHeaderPacked);
    * (adl::uint64_t *) msg.data() = adl::htonq(nkeys);

    adl::memcpy(msg.data() + 8, params.keyHeaders.data(), params.keyHeaders.size());
    adl::memcpy(msg.data() + 8 + params.keyHeaders.size(), params.keys.data(), params.keys.size());

    auto pHeader = (GetIdentityKeysKeyHeaderPacked*) msg.data();

    while ((void*) pHeader < msg.data() + params.keyHeaders.size()) {
        pHeader->len = adl::htonl(pHeader->len);

        pHeader->offset = adl::htonq(
            pHeader->offset + adl::int64_t(sizeof(nkeys)) + adl::int64_t(params.keyHeaders.size())
        );

        pHeader->id = adl::htonq(pHeader->id);

        pHeader++;
    }

    return sendResponse(0, msg);
}


Status Protocol1Connection::appreciateGetIdentityKeys(
    protocol::Response& response,
    void* data,
    void (*record) (
        adl::int8_t nodeType, 
        adl::int8_t keyType, 
        const adl::ByteArray& key, 
        adl::int64_t id, 
        void* data
    )
) {
    if (response.code != 0)
        return Status::PROTOCOL_ERROR;

    
    auto pMsg = (const char*) response.msg;
    auto nkeys = adl::ntohq (* (adl::uint64_t *) pMsg);
    
    adl::size_t msgLen = response.header.length - 8;
    if (msgLen < 8 + nkeys * sizeof(GetIdentityKeysKeyHeaderPacked))
        return Status::PROTOCOL_ERROR;

    
    auto pHeader = (GetIdentityKeysKeyHeaderPacked*) (pMsg + 8);

    for (adl::size_t i = 0; i < nkeys; i++) {
        auto& header = pHeader[i];
        
        if (header.keyType != 0 || (header.nodeType != 0 && header.nodeType != 1))
            return Status::PROTOCOL_ERROR;

        auto offset = adl::ntohq(header.offset);
        auto len = adl::ntohl(header.len);
        auto id = adl::ntohq(header.id);
        if (offset + len > adl::int64_t(msgLen))
            return Status::PROTOCOL_ERROR;
        
        adl::ByteArray key;
        if (!key.append(pMsg + offset, (adl::size_t) len))
            return Status::OUT_OF_RESOURCE;
        
        record(header.nodeType, header.keyType, key, id, data);
    }

    return Status::SUCCESS;
}


// ------ 0x2000 : Memory Node Show ID ------


Status Protocol1Connection::sendMemoryNodeShowId(adl::int64_t id) {
    adl::int64_t netOrderId = adl::htonq(id);
    return sendMsg(
        protocol::MsgType::MemoryNodeShowId,
        &netOrderId,
        8
    );
}


Status Protocol1Connection::decodeMemoryNodeShowId(protocol::Msg* msg, adl::int64_t* id) {
    
    if (msg->header.length < sizeof(adl::int64_t))
        return Status::PROTOCOL_ERROR;

    *id = adl::ntohq( *(adl::int64_t*) msg->data);
    return Status::SUCCESS;
}


// ------ 0x2001 : Memory Node Clock In ------

Status Protocol1Connection::sendMemoryNodeClockIn(adl::uint32_t tcp4Ip, adl::uint16_t port) {
    adl::uint8_t data[12];
    *(adl::int32_t*) data = adl::htonl(4);  // TCP Protocol Version 4.
    *(adl::uint32_t*) (data + 4) = adl::htonl((adl::uint32_t) port);
    *(adl::uint32_t*) (data + 8) = tcp4Ip;
    return sendMsg(MsgType::MemoryNodeClockIn, data, sizeof(data));
}


Status Protocol1Connection::decodeMemoryNodeClockIn(
    protocol::Msg* msg, 
    adl::int32_t* tcpVer, 
    adl::uint16_t* port, 
    adl::uint8_t ip[]
) {
    if (msg->header.length < 12) {
        return Status::PROTOCOL_ERROR;
    }
    
    *tcpVer = adl::ntohl( *(adl::int32_t*) (msg->data + 0) );
    *port = (adl::uint16_t) adl::ntohl( *(adl::uint32_t*) (msg->data + 4) );

    if (*tcpVer != 4)
        return Status::PROTOCOL_ERROR;
    
    adl::memcpy(ip, msg->data + 8, (*tcpVer == 4 ? 4 : 16));
    return Status::SUCCESS;
}


// ------ 0x3004 : Check Avail Mem ------

Status Protocol1Connection::sendCheckAvailMem() {
    return sendMsg(MsgType::CheckAvailMem);
}


// ------ 0x3005 : Free Block ------

Status Protocol1Connection::sendFreeBlock(adl::uint64_t blockId) {
    auto bId_netOrder = adl::htonq(blockId);
    return sendMsg(MsgType::FreeBlock, &bId_netOrder, sizeof(bId_netOrder));
}


}
