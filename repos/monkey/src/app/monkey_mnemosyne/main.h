/*
 * Monkey Mnemosyne : Memory provider.
 *
 * Created on 2025.2.4 at Xiangzhou, Zhuhai, Guangdong
 * 
 * 
 * gongty  [at] tongji [dot] edu [dot] cn
 * 
 */


#pragma once

#include <adl/sys/types.h>
#include <adl/collections/ArrayList.hpp>
#include <adl/collections/HashMap.hpp>

#include <libc/component.h>
#include <base/heap.h>

#include <monkey/Status.h>
#include <monkey/net/IP4Addr.h>
#include <monkey/net/Socket4.h>


struct MnemosyneMain {

    Genode::Env& env;
    Genode::Heap heap { env.ram(), env.rm() };
    adl::int64_t nodeId = 0;

    adl::HashMap<adl::int64_t, adl::ByteArray> appKeys;

    struct {
        struct {
            monkey::net::IP4Addr ip;
            adl::uint16_t port;
        } concierge;

        struct {
            monkey::net::IP4Addr ip;
            adl::uint16_t port;
            adl::uint16_t listenPort;
            adl::ByteArray key;
        } mnemosyne;
    } config;

    MnemosyneMain(Genode::Env&);


    monkey::Status loadConfig();
    monkey::Status init();

    monkey::Status clockIn();
    void serveClient(monkey::net::Socket4& conn);
    monkey::Status runServer();

    monkey::Status run();
    void cleanup();


};


