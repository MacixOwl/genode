/*
 * Sunflower Lounge : Base type for App Lounge and Memory Lounge.
 *
 * Name inspired by CMB.
 *   https://www.cmbchina.com/personal/allinonecard/cardinfo.aspx?guid=be48a9ab-300f-4393-b80e-02308ddf1a7f
 * 
 * gongty [at] tongji [dot] edu [dot] cn
 * Created on 2025.1.25 at Xiangzhou, Zhuhai, Guangdong
 */

#pragma once

#include "./main.h"
#include <monkey/net/protocol.h>

struct SunflowerLounge {
    virtual ~SunflowerLounge() {}

    ConciergeMain& context;
    monkey::net::Protocol1Connection& conn;

    SunflowerLounge(
        ConciergeMain& context,
        monkey::net::Protocol1Connection& conn
    )
    : context {context}, conn {conn}
    {}


    virtual monkey::Status serve() = 0;

};
