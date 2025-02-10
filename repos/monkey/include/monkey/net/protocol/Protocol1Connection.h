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
#include <adl/collections/HashMap.hpp>

namespace monkey::net {


class Protocol1Connection : public ProtocolConnection {
public:
    struct MemoryNodeInfo {
        adl::int64_t id;
        adl::int32_t tcpVersion;
        adl::uint16_t port;

        adl::uint8_t ipAddr[16];
    };

    enum class NodeType {
        MemoryNode,
        App,
        Unknown,
        Other
    } nodeType = NodeType::Other;


    /**
     * Other side's id.
     */
    union {
        adl::int64_t memoryNodeId = -1;
        adl::int64_t appId;
        adl::int64_t nodeId;
    };


    static const adl::int64_t VERSION = 1;
    virtual adl::int64_t version() override { return VERSION; }


    // Connection operations


    Status sendResponse(
        const adl::uint32_t code,
        const adl::size_t msgLen,
        const void* msg,
        const adl::size_t paimonLen = 0,
        const void* paimon = nullptr
    );

    Status sendResponse(const adl::uint32_t code, const adl::ByteArray& msg);
    Status sendResponse(const adl::uint32_t code, const adl::TString& msg);
    Status sendResponse(const adl::uint32_t code, const char* msg);



    /**
     * The caller is responsible for freeing `response`  
     * when monkey::Status is SUCCESS.
     */
    Status recvResponse(protocol::Response** response);



    // ------ 0x1001 : Auth ------

    Status sendAuth(const adl::ByteArray& challenge);
    Status recvAuth(adl::ByteArray& challenge);

    /**
     * Client-mode auth: Show identity to server. 
     */
    Status auth(const adl::ByteArray& key);

    /**
     * Server-mode auth: Check client's identity. 
     */
    Status auth(
        // app id -> app key
        const adl::HashMap<adl::int64_t, adl::ByteArray>& appsKeyring, 
        const adl::ArrayList<adl::ByteArray>& memoryNodesKeyring
    );


    /**
     * Server-mode auth: Check client's identity. 
     * This method works just like the one above except: You can set keyring to nullptr
     * to ignore some type(s).
     */
    Status auth(
        // app id -> app key
        const adl::HashMap<adl::int64_t, adl::ByteArray>* appsKeyring,
        const adl::ArrayList<adl::ByteArray>* memoryNodesKeyring
    );


    // ------ 0x1100 : Get Identity Keys ------

    struct ReplyGetIdentityKeysParams {
        adl::ByteArray keyHeaders;
        adl::ByteArray keys;

        enum class NodeType : adl::int8_t {
            App = 0,
            MemoryNode = 1
        };

        enum class KeyType : adl::int8_t {
            RC4 = 0
        };

        monkey::Status addKey(
            NodeType nodeType, 
            KeyType keyType, 
            const adl::ByteArray& key,
            adl::int64_t id = 0  // ignored for Memory Node
        ); 
    };


    Status sendGetIdentityKeys();
    Status replyGetIdentityKeys(const ReplyGetIdentityKeysParams&);
    Status appreciateGetIdentityKeys(
        protocol::Response& response,
        void* data,

        /**
         * Called for each key.
         *
         * @param data Same as `data` passed to `appreciate`.
         */
        void (*record) (
            ReplyGetIdentityKeysParams::NodeType nodeType, 
            ReplyGetIdentityKeysParams::KeyType keyType, 
            const adl::ByteArray& key, 
            adl::int64_t id, 
            void* data
        )
    );


    // ------ 0x2000 : Memory Node Show ID ------

    Status sendMemoryNodeShowId(adl::int64_t id);
    Status decodeMemoryNodeShowId(protocol::Msg* msg, adl::int64_t* id);


    // ------ 0x2001 : Memory Node Clock In ------


    Status memoryNodeClockIn(adl::int64_t* id);


    /**
     * For TCP4. 
     */
    Status sendMemoryNodeClockIn(adl::uint32_t tcp4Ip, adl::uint16_t port);

    /**
     * 
     * 
     * @param ip Should be at least 16 bytes (array length at least 16).
     *           For TCP4, ip[0] to ip[3] is filled.
     *           For TCP6, ip[0] to ip[15] is filled.
     * 
     */
    Status decodeMemoryNodeClockIn(
        protocol::Msg* msg, 
        adl::int32_t* tcpVer, 
        adl::uint16_t* port, 
        adl::uint8_t ip[]
    );


    // ------ 0x2004 : Locate Memory Nodes ------

    Status sendLocateMemoryNodes(); // todo
    Status replyLocateMemoryNodes(const adl::ArrayList<MemoryNodeInfo>&);  // todo
    Status locateMemoryNodes(adl::ArrayList<MemoryNodeInfo>&);  // todo


    // ------ 0x3001 : Try Alloc ------

    // todo

    // ------ 0x3002 : Read Block ------

    // todo


    // ------ 0x3003 : Write Block ------

    // todo

    // ------ 0x3004 : Check Avail Mem ------
    
    Status sendCheckAvailMem();
    Status replyCheckAvailMem(adl::size_t availMem);  // todo
    Status checkAvailMem(adl::size_t* availMem);  // todo


    // ------ 0x3005 : Free Block ------

    Status sendFreeBlock(adl::uint64_t blockId);

};


}
