/*
 * main.h : defines struct Main.
 *
 * gongty [at] tongji [dot] edu [dot] cn
 * Created on 2025.1.25 at Xiangzhou, Zhuhai, Guangdong
 *
 */


#pragma once

#include <adl/collections/HashMap.hpp>
#include <monkey/net/protocol.h>

#include <libc/component.h>
#include <base/heap.h>


/**
* Used on runtime.
*/
struct MemoryNodeInfo {
    adl::int64_t id;
    monkey::net::IP4Addr ip;
    adl::uint32_t port;
};


struct ClientConnection {
    int socketFd;
    struct sockaddr_in inaddr;
    socklen_t addrlen;


    inline adl::TString ip() {
        monkey::net::IP4Addr ipAddr {inaddr.sin_addr.s_addr};
        return ipAddr.toString();   
    }

    inline adl::uint16_t port() {
        return adl::ntohs(inaddr.sin_port);
    }
};



struct SocketServer : public monkey::net::PromisedSocketIo {
    monkey::Status start(adl::uint16_t port);
    ClientConnection accept(bool ignoreError = true);
};


struct ConciergeMain {

    Genode::Env& env;
    Genode::Heap heap { env.ram(), env.rm() };

    SocketServer server;
    adl::uint16_t port = 0;

    adl::HashMap<adl::int64_t, MemoryNodeInfo> memoryNodes;  // id -> info

    struct {
        adl::ArrayList<adl::ByteArray> memoryNodes;
        adl::ArrayList<adl::ByteArray> apps;
    } keyrings;


    ConciergeMain(Genode::Env&);


    adl::int64_t nextMemoryNodeId = 0;
    inline adl::int64_t genMemoryNodeId() { return ++nextMemoryNodeId; }

    void initAdlAlloc();
    monkey::Status loadConfig();
    monkey::Status init();
    void serveClient(ClientConnection&);
    monkey::Status run();
    void cleanup();


};
