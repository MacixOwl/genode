/*
 * Monkey Concierge : Remembers where all nodes are.
 *
 * Created on 2025.1.6 at Minhang, Shanghai
 * 
 * 
 * gongty  [at] tongji [dot] edu [dot] cn
 * feng.yt [at]  sjtu  [dot] edu [dot] cn
 * 
 */


#include "./main.h"
#include "./AppLounge.h"
#include "./MemoryLounge.h"


#include <adl/stdint.h>
#include <adl/config.h>
#include <adl/collections/HashMap.hpp>
#include <base/component.h>
#include <libc/component.h>
#include <base/heap.h>
#include <base/attached_rom_dataspace.h>

#include <monkey/Status.h>

#include <monkey/net/TcpIo.h>
#include <monkey/net/protocol.h>
#include <monkey/net/IP4Addr.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <netinet/in.h>


using namespace monkey;


Status checkCanUseProtocolV1(net::ProtocolConnection& client) {
    adl::ArrayList<adl::int64_t> versions;
    Status status;
    if ((status = client.recvHello(versions)) != Status::SUCCESS) {
        return status;
    }

    bool versionMatched = false;
    for (auto& it : versions) {
        if (it == net::Protocol1Connection::VERSION) {
            versionMatched = true;
            break;
        }
    }

    if (!versionMatched)
        return status = Status::PROTOCOL_ERROR;

    versions.clear();
    versions.append(net::Protocol1Connection::VERSION);
    if ((status = client.sendHello(versions)) != Status::SUCCESS)
        return status;
    

    if ((status = client.recvHello(versions)) != Status::SUCCESS)
        return status;

    if (versions.size() == 1 && versions[0] == net::Protocol1Connection::VERSION) {
        return Status::SUCCESS;
    }
    
    return Status::PROTOCOL_ERROR;
}



Status SocketServer::start(adl::uint16_t port) {
    struct sockaddr_in addr;
    socketFd = socket(AF_INET, SOCK_STREAM, 0);

    if (socketFd == 0) {
        return Status::NETWORK_ERROR;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = adl::htons(port);

    if (bind(socketFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { 
        goto BIND_OR_LISTEN_ERROR;
    }

    if (listen(socketFd, 20) < 0) {
        goto BIND_OR_LISTEN_ERROR;
    }

    Genode::log("Server ready.");

    return Status::SUCCESS;


BIND_OR_LISTEN_ERROR:

    close();
    socketFd = -1;
    return Status::NETWORK_ERROR;

}


ClientConnection SocketServer::accept(bool ignoreError) {
    ClientConnection client;
    while (true) {
        client.socketFd = ::accept(
            socketFd, 
            (struct sockaddr*) &client.inaddr, 
            (socklen_t*) &client.addrlen
        );

        if (client.socketFd != -1 || !ignoreError) {
            return client;
        }
    }
}



void ConciergeMain::initAdlAlloc() {
    adl::defaultAllocator.init({
        .alloc = [] (adl::size_t size, void* data) {
            return reinterpret_cast<Genode::Heap*>(data)->alloc(size);
        },
        
        .free = [] (void* addr, adl::size_t size, void* data) {
            reinterpret_cast<Genode::Heap*>(data)->free(addr, size);
        },
        
        .data = &heap
    });
}


Status ConciergeMain::loadConfig() {
    Genode::Attached_rom_dataspace configDs { env, "config" };
    auto config = configDs.xml();
    auto conciergeNode = config.sub_node("monkey-concierge");
    auto serverNode = conciergeNode.sub_node("server");
    auto portNode = serverNode.sub_node("port");
    {
        adl::ByteArray portByteArr;

        if (!portByteArr.resize(portNode.content_size())) {
            return Status::OUT_OF_RESOURCE;
        }
        
        portNode.decoded_content((char*) portByteArr.data(), portByteArr.size());
        port = 0;
        
        for (auto ch : portByteArr) {
            
            if (ch >= '0' && ch <= '9') {
                port *= 10;
                port += uint16_t(ch - '0');
            }
            else {
                
                return Status::INVALID_PARAMETERS;
            }
        }
    }
    

    // load memory nodes' keys

    conciergeNode.for_each_sub_node("memory-node", [&] (const Genode::Xml_node& node) {
        auto keyNode = node.sub_node("key");
        adl::ByteArray arr;

        keyNode.with_raw_content([&] (const char* content, size_t contentSize) {
            if (!arr.resize(contentSize)) {
                throw Status::OUT_OF_RESOURCE;  // bad way.. but.. anyway..
            }

            adl::memcpy(arr.data(), content, contentSize);
        });
        
        this->keyrings.memoryNodes.append(arr);
    });
    

    // load apps' keys

    conciergeNode.for_each_sub_node("app", [&] (const Genode::Xml_node& node) {
        auto keyNode = node.sub_node("key");
        auto idNode = node.sub_node("id");
        adl::ByteArray key;
        adl::TString idStr;
        
        keyNode.with_raw_content([&] (const char* content, size_t contentSize) {
            if (!key.resize(contentSize)) {
                throw Status::OUT_OF_RESOURCE;  // bad way.. but.. anyway..
            }

            adl::memcpy(key.data(), content, contentSize);
        });

        idNode.with_raw_content([&] (const char* content, size_t contentSize) {
            for (adl::size_t idx = 0; idx < contentSize; idx++) {
                idStr += content[idx];
            }
        });

        this->keyrings.apps[idStr.toInt64()] = key;
    });

    return Status::SUCCESS;
}


Status ConciergeMain::init() {
    initAdlAlloc();
    Status status = Status::SUCCESS;
    
    try {
        status = loadConfig();
        if (status == Status::SUCCESS) {
            Genode::log("Config loaded.");
            Genode::log("> port: ", port);
            for (auto& it : keyrings.memoryNodes) {
                Genode::log("> memory node key: ", it.toString().c_str());
            }

            for (const auto& it : keyrings.apps)
                Genode::log("> client  app key: ", it.first, " -> ", it.second.toString().c_str());
        }
    }
    catch (...) {
        Genode::error("Failed on loading config. Check your .run file.");
        status = Status::INVALID_PARAMETERS;
    }

    return status;
}


void ConciergeMain::serveClient(ClientConnection& conn) {
    Genode::log("Client connected: ", conn.ip().c_str(), " [", conn.port(), "]");

    net::ProtocolConnection protocolConn;
    protocolConn.socketFd = conn.socketFd;

    // Determine protocol version
    if (checkCanUseProtocolV1(protocolConn) != Status::SUCCESS)
        return;
    Genode::log("> Protocol Version: ", 1);
    protocolConn.socketDetached = true;

    net::Protocol1Connection client;
    client.socketFd = conn.socketFd;
    client.port = conn.port();
    client.ip4Addr = conn.inaddr.sin_addr.s_addr;

    // Auth
    if (client.auth(keyrings.apps, keyrings.memoryNodes) != Status::SUCCESS)
        return;
    adl::TString nodeTypeStr;
    switch (client.nodeType) {
        case net::Protocol1Connection::NodeType::MemoryNode:
            nodeTypeStr = "MemoryNode";
            break;
        case net::Protocol1Connection::NodeType::Other:
            nodeTypeStr = "Other";
            break;
        case net::Protocol1Connection::NodeType::App:
            nodeTypeStr = "App";
            break;
        default:
            nodeTypeStr = "Unknown";
            break;
    }
    Genode::log("> Client Type: ", nodeTypeStr.c_str());
    
    if (client.nodeType == net::Protocol1Connection::NodeType::App) {
        AppLounge lounge {*this, client};
        Status status = lounge.serve();
        Genode::log("App left with status: ", adl::int32_t(status));
    }
    else if (client.nodeType == net::Protocol1Connection::NodeType::MemoryNode) {
        MemoryLounge lounge {*this, client};
        Status status = lounge.serve();
        Genode::log("Memory left with status: ", adl::int32_t(status));
    }
    
}


Status ConciergeMain::run() {
    Status status = server.start(this->port);
    if (status != Status::SUCCESS) {
        return status;
    }

    Genode::log("Server started on port: ", port);

    while (true) {
        ClientConnection client = server.accept();
        serveClient(client);
        Genode::log("Closed: ", client.ip().c_str(), " [", client.port(), "]");
    }

    return status;
}


void ConciergeMain::cleanup() {

}


ConciergeMain::ConciergeMain(Genode::Env& env) : env {env} {

    Libc::with_libc([&] () {

        Genode::log("Hello :D This is Monkey Concierge");
        
        Status status = init();
        if (status != monkey::Status::SUCCESS) {
            Genode::log("Something went wrong on init.. (", adl::int32_t(status), ")");
            goto END;
        }

        status = run();
        if (status != monkey::Status::SUCCESS) {
            Genode::log("Something went wrong when running.. (", adl::int32_t(status), ")");
        }
        
END:
        cleanup();        
        Genode::log("Bye :D");
    });
}



void Libc::Component::construct(Libc::Env& env) {
    static ConciergeMain main {env};
}
