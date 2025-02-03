/*
 * Memory Lounge : Serves Memory Node connection
 * 
 * gongty [at] tongji [dot] edu [dot] cn
 * Created on 2025.1.25 at Xiangzhou, Zhuhai, Guangdong
 */


#pragma once

#include "./SunflowerLounge.h"
#include <monkey/net/IP4Addr.h>

struct MemoryLounge : public SunflowerLounge {

    MemoryLounge(
        ConciergeMain& context,
        monkey::net::Protocol1Connection& conn
    )
    : SunflowerLounge(context, conn)
    {}


    monkey::Status processMemoryNodeClockIn(const monkey::net::IP4Addr& ip4Addr, adl::uint16_t port);
    monkey::Status processGetIdentityKeys();


    virtual monkey::Status serve() override;
};
