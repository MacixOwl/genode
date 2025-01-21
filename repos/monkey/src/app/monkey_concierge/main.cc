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

#define _BYTEORDER_FUNC_DEFINED

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



Status checkWhetherCanUseProtocolV1(net::ProtocolConnection& client) {
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


struct ClientConnection {
    int socketFd;
    struct sockaddr_in inaddr;
    socklen_t addrlen;


    adl::TString ip() {
        net::IP4Addr ipAddr {inaddr.sin_addr.s_addr};
        return ipAddr.toString();   
    }

    adl::uint16_t port() {
        return adl::ntohs(inaddr.sin_port);
    }
};


class SocketServer : public net::PromisedSocketIo {

public:

    Status start(adl::uint16_t port) {
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


    ClientConnection accept(bool ignoreError = true) {
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


};


/**
 * Used on runtime.
 */
struct MemoryNodeInfo {
    int64_t id;
    net::IP4Addr ip;
    adl::uint32_t port;

};



struct Main {

    Genode::Env& env;
    Genode::Heap heap { env.ram(), env.rm() };
    adl::HashMap<int, net::ProtocolConnection*> clients;  // client's socketFd -> struct

    SocketServer server;
    adl::uint16_t port = 0;

    adl::ArrayList<MemoryNodeInfo> memoryNodes;

    struct {
        adl::ArrayList<adl::ByteArray> memoryNodes;
        adl::ArrayList<adl::ByteArray> apps;
    } keyrings;

    void initAdlAlloc() {
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


    Status loadConfig() {
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
        

        conciergeNode.for_each_sub_node("app", [&] (const Genode::Xml_node& node) {
            auto keyNode = node.sub_node("key");
            adl::ByteArray arr;
            
            keyNode.with_raw_content([&] (const char* content, size_t contentSize) {
                if (!arr.resize(contentSize)) {
                    throw Status::OUT_OF_RESOURCE;  // bad way.. but.. anyway..
                }

                adl::memcpy(arr.data(), content, contentSize);
            });

            this->keyrings.apps.append(arr);
        });

        return Status::SUCCESS;
    }


    Status init() {
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

                for (auto& it : keyrings.apps)
                    Genode::log("> client  app key: ", it.toString().c_str());
            }
        }
        catch (...) {
            Genode::error("Failed on loading config. Check your .run file.");
            status = Status::INVALID_PARAMETERS;
        }

        return status;
    }


    void serveApp(net::ProtocolConnection& conn) {
        // TODO
    }


    void serveMemoryNode(net::ProtocolConnection& conn) {
        // TODO
    }


    void serveClient(ClientConnection& conn) {
        Genode::log("Client connected: ", conn.ip().c_str(), " [", conn.port(), "]");

        net::ProtocolConnection protocolConn;
        protocolConn.socketFd = conn.socketFd;

        // Determine protocol version
        if (checkWhetherCanUseProtocolV1(protocolConn) != Status::SUCCESS)
            return;
        Genode::log("> Protocol Version: ", 1);
        protocolConn.unmanageSocketFd();

        net::Protocol1Connection client;
        client.socketFd = conn.socketFd;

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
            serveApp(client);
        }
        else if (client.nodeType == net::Protocol1Connection::NodeType::MemoryNode) {
            serveMemoryNode(client);
        }
    }


    Status run() {
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


    void clearClients() {
        for (auto it : clients) {
            adl::defaultAllocator.free(it.second);
        }

        clients.clear();
    }


    void cleanup() {
        clearClients();
    }


    Main(Genode::Env& env) : env {env} {

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

};  // struct Main


void Libc::Component::construct(Libc::Env& env) {
    static Main main {env};
}
