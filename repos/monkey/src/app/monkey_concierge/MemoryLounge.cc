/*
 * Memory Lounge : Serves Memory Node connection
 * 
 * gongty [at] tongji [dot] edu [dot] cn
 * Created on 2025.1.25 at Xiangzhou, Zhuhai, Guangdong
 */


#include "./MemoryLounge.h"

using namespace monkey;
using namespace net;

using net::protocol::Msg;
using net::protocol::MsgType;


monkey::Status MemoryLounge::processMemoryNodeClockIn(const monkey::net::IP4Addr& ip4Addr, adl::uint16_t port) {
    Status status = Status::SUCCESS;

    if (ip4Addr != conn.ip4Addr) {
        conn.sendResponse(1, "IP address conflict.");
        return Status::PROTOCOL_ERROR;
    }

    if (port != conn.port) {
        conn.sendResponse(1, "Port conflict.");
        return Status::PROTOCOL_ERROR;
    }

    adl::int64_t nodeId = context.genMemoryNodeId();
    
    MemoryNodeInfo info;
    info.id = nodeId;
    info.ip = ip4Addr;
    info.port = port;
    context.memoryNodes[nodeId] = info;
    
    nodeId = adl::htonq(nodeId);
    conn.sendResponse(0, sizeof(adl::int64_t), &nodeId);

    return status;
}


monkey::Status MemoryLounge::processGetIdentityKeys() {
    net::Protocol1Connection::ReplyGetIdentityKeysParams params;

    auto appType = Protocol1Connection::ReplyGetIdentityKeysParams::NodeType::App;
    auto memoryType = Protocol1Connection::ReplyGetIdentityKeysParams::NodeType::MemoryNode;
    auto rc4Type = Protocol1Connection::ReplyGetIdentityKeysParams::KeyType::RC4;

    Status status = Status::SUCCESS;

    // memory node keys

    for (auto& key : context.keyrings.memoryNodes) {
        if (status != Status::SUCCESS)
            break;
    
        status = params.addKey(memoryType, rc4Type, key);
    }

    // app keys

    for (const auto& it : context.keyrings.apps) {
        if (status != Status::SUCCESS)
            break;

        status = params.addKey(appType, rc4Type, it.second, it.first);
    }


    // reply

    if (status != Status::SUCCESS) {
        conn.sendResponse(1, "Something went wrong on our side.");
        return status;
    }
    
    return conn.replyGetIdentityKeys(params);
}



monkey::Status MemoryLounge::serve() {
    Genode::log("====== Welcome client (Memory) to Sunflower Lounge ======");
    Status status = Status::SUCCESS;

    while (true) {
        Msg* msg = nullptr;
        status = conn.recvMsg(&msg);
        if (status != Status::SUCCESS)
            return status;

        
        switch ((MsgType) msg->header.type) {

            case MsgType::MemoryNodeClockIn: {
                Genode::log("> Message: Memory Node Clock In");
                net::IP4Addr ip;
                adl::uint8_t ipBuf[16];
                adl::int32_t tcpVer;
                adl::uint16_t port;
                status = conn.decodeMemoryNodeClockIn(msg, &tcpVer, &port, ipBuf);
                
                if (status != Status::SUCCESS) {            
                    conn.sendResponse(1, "Failed to decode message.");
                    break;
                }

                if (tcpVer != 4) {
                    conn.sendResponse(1, "Only IPv4 supported yet.");
                    status = Status::PROTOCOL_ERROR;
                    break;
                }

                ip.i32 = * (adl::int32_t *) ipBuf;
                Genode::log("  TCP ", tcpVer, ", port ", port, ", ip ", ip.toString());
                status = processMemoryNodeClockIn(ip, port);
                break;
            }

            case MsgType::GetIdentityKeys: {
                Genode::log("> Message: Get Identity Keys");
                status = processGetIdentityKeys();
                break;
            }

            default: {
                Genode::warning("> Message Type NOT SUPPORTED");
                status = Status::PROTOCOL_ERROR;
                conn.sendResponse(1, "Msg Type not supported.");
                break;
            }
        }


        adl::defaultAllocator.free(msg);
        if (status != Status::SUCCESS)
            break;
    }

    return status;
}
