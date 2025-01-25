/*
 * Memory Lounge : Serves Memory Node connection
 * 
 * gongty [at] tongji [dot] edu [dot] cn
 * Created on 2025.1.25 at Xiangzhou, Zhuhai, Guangdong
 */


#pragma once

#include "./SunflowerLounge.h"

struct MemoryLounge : public SunflowerLounge {

    MemoryLounge(
        ConciergeMain& context,
        monkey::net::Protocol1Connection& conn
    )
    : SunflowerLounge(context, conn)
    {}

    virtual monkey::Status serve() override;
};
