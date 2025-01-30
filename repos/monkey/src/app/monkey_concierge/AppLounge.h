/*
 * App Lounge : Serves App connection
 * 
 * gongty [at] tongji [dot] edu [dot] cn
 * Created on 2025.1.25 at Xiangzhou, Zhuhai, Guangdong
 */


#pragma once

#include "./SunflowerLounge.h"

struct AppLounge : public SunflowerLounge {

    AppLounge(
        ConciergeMain& context,
        monkey::net::Protocol1Connection& conn
    )
    : SunflowerLounge(context, conn)
    {}

    monkey::Status processLocateMemoryNodes();

    virtual monkey::Status serve() override;
};
