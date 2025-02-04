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

#include <libc/component.h>
#include <base/heap.h>

#include <monkey/Status.h>
#include <monkey/net/IP4Addr.h>


struct MnemosyneMain {

    Genode::Env& env;
    Genode::Heap heap { env.ram(), env.rm() };

    struct {
        struct {
            monkey::net::IP4Addr ip;
            adl::uint16_t port;
        } concierge;

        struct {
            monkey::net::IP4Addr ip;
            adl::uint16_t port;
            adl::uint16_t listenPort;
        } mnemosyne;
    } config;

    MnemosyneMain(Genode::Env&);


    monkey::Status loadConfig();
    monkey::Status init();
    monkey::Status run();
    void cleanup();


};


