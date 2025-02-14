/*
 * 
 * gongty [at] tongji [dot] edu [dot] cn
 * Created on 2025.2.10 at Yushan, Shangrao
 */

#include "./AppLounge.h"

using namespace monkey;

using monkey::net::IP4Addr;
using monkey::net::protocol::Msg;
using monkey::net::protocol::MsgType;
using monkey::net::protocol::Header;

Status AppLounge::processTryAlloc(adl::size_t blockSize) {
    Status status = Status::SUCCESS;

    // todo

    return status;
}


Status AppLounge::processReadBlock(adl::int64_t blockId) {
    Status status = Status::SUCCESS;

    // todo

    return status;
}


Status AppLounge::processWriteBlock(adl::int64_t blockId, const void* data) {
    Status status = Status::SUCCESS;

    // todo

    return status;
}


Status AppLounge::processCheckAvailMem() {
    Status status = Status::SUCCESS;

    // todo

    return status;
}


Status AppLounge::processFreeBlock(adl::int64_t blockId) {
    Status status = Status::SUCCESS;

    // todo

    return status;
}



Status AppLounge::serve() {

    Genode::log("====== Welcome client (APP) to Sunflower Lounge ======");

    Status status = Status::SUCCESS;


    while (true) {
        Msg* msg = nullptr;
        status = client.recvMsg(&msg);
        if (status != Status::SUCCESS) {
            return status;
        }


        switch ((MsgType) msg->header.type) {
            // todo

            default: {
                Genode::warning("> Message Type NOT SUPPORTED");
                status = Status::PROTOCOL_ERROR;
                client.sendResponse(1, "Msg Type not supported.");
                break;
            }
        }


        adl::defaultAllocator.free(msg);
        if (status != Status::SUCCESS) {
            break;
        }
    }


    return status;
}
