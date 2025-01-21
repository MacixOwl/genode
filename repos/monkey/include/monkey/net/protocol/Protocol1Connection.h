/*
 * Protocol V1 Connection
 *
 * Created on 2025.1.21 at Xiangzhou, Zhuhai, Guangdong
 * 
 * 
 * gongty  [at] tongji [dot] edu [dot] cn
 * 
 */

#pragma once

#include <monkey/net/protocol/ProtocolConnection.h>

namespace monkey::net {


class Protocol1Connection : public ProtocolConnection {
public:

    enum class NodeType {
        MemoryNode,
        App,
        Unknown,
        Other
    } nodeType = NodeType::Other;

    union {
        adl::int64_t memoryNodeId = -1;
        adl::int64_t appId;
    };


    static const adl::int64_t VERSION = 1;
    virtual adl::int64_t version() override { return VERSION; }


    // Connection operations

    Status sendAuth(const adl::ByteArray& challenge);
    Status recvAuth(adl::ByteArray& challenge);

    Status sendResponse(const adl::uint32_t code, const adl::ByteArray& msg);
    Status sendResponse(const adl::uint32_t code, const adl::TString& msg);
    Status sendResponse(const adl::uint32_t code, const char* msg);



    /**
    * The caller is responsible for freeing `response`  
    * when monkey::Status is SUCCESS.
    */
    Status recvResponse(protocol::Response** response);


    Status sendCheckAvailMem();


    Status sendFreeBlock(adl::uint64_t blockId);

    /**
     * Client-mode auth: Show identity to server. 
     */
    Status auth(const adl::ByteArray& key);

    /**
     * Server-mode auth: Check client's identity. 
     */
    Status auth(
        const adl::ArrayList<adl::ByteArray>& appsKeyring, 
        const adl::ArrayList<adl::ByteArray>& memoryNodesKeyring
    );

};


}
