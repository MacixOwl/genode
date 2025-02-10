/*
 * App Lounge : Serves App connection
 * 
 * gongty [at] tongji [dot] edu [dot] cn
 * Created on 2025.1.25 at Xiangzhou, Zhuhai, Guangdong
 */


#include "./AppLounge.h"

using namespace monkey;

using net::protocol::Msg;
using net::protocol::MsgType;


Status AppLounge::processLocateMemoryNodes() {
    Status status = Status::SUCCESS;

    struct MemoryNodeInfoPacked {
        adl::int64_t id;
        adl::int32_t tcpVersion;  // must be 4 (in net order)
        adl::uint32_t port;

        adl::uint32_t inet4addr;
        adl::int8_t padding[12];
    } __packed;

    adl::ArrayList<MemoryNodeInfoPacked> payload;
    for (const auto& it : context.memoryNodes) {
        MemoryNodeInfoPacked info;
        info.id = adl::htonq(it.second.id);
        info.tcpVersion = adl::htonl(4);
        info.port = adl::htonl(it.second.port);
        info.inet4addr = it.second.ip.ui32;
        payload.append(info);
    }


    status = client.sendResponse(0, payload.size() * sizeof(MemoryNodeInfoPacked), payload.data());

    return status;
}


Status AppLounge::serve() {

    Genode::log("====== Welcome client (APP) to Sunflower Lounge ======");

    Status status = Status::SUCCESS;


    while (true) {
        Msg* msg = nullptr;
        status = client.recvMsg(&msg);
        if (status != Status::SUCCESS)
            return status;


        switch ((MsgType) msg->header.type) {
            case MsgType::LocateMemoryNodes: {
                Genode::log("> Message: Locate Memory Nodes");
                status = processLocateMemoryNodes();
                break;
            }
            default: {
                Genode::warning("> Message Type NOT SUPPORTED");
                status = Status::PROTOCOL_ERROR;
                client.sendResponse(1, "Msg Type not supported.");
                break;
            }
        }


        adl::defaultAllocator.free(msg);
        if (status != Status::SUCCESS)
            break;
    }

    return status;
}
