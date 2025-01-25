/*
 * Memory Lounge : Serves Memory Node connection
 * 
 * gongty [at] tongji [dot] edu [dot] cn
 * Created on 2025.1.25 at Xiangzhou, Zhuhai, Guangdong
 */


#include "./MemoryLounge.h"

using namespace monkey;

using net::protocol::Msg;
using net::protocol::MsgType;

monkey::Status MemoryLounge::serve() {
    Status status = Status::SUCCESS;

    while (true) {
        Msg* msg = nullptr;
        status = conn.recvMsg(&msg);
        if (status != Status::SUCCESS)
            return status;

        // todo

        adl::defaultAllocator.free(msg);
    }

    return status;
}
