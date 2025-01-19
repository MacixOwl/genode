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

#include <monkey/Status.h>

#include <monkey/net/TcpIo.h>
#include <monkey/net/protocol.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <netinet/in.h>


using namespace monkey;


class PromisedIo : public net::TcpIo {
public:
    int socketFd = -1;

    virtual bool valid() { return socketFd > 1; }

    virtual adl::int64_t recv(void* buf, adl::size_t len) override {
        adl::size_t sum = 0;
        while (sum < len) {
            adl::int64_t curr = read(socketFd, ((char*) buf) + sum, len - sum);

            if (curr == -1) {
                return -1;
            }

            sum += size_t(curr);
        }
        
        return adl::int64_t(sum);
    }

    virtual adl::int64_t send(const void* buf, adl::size_t len) override {
        return ::write(socketFd, buf, len);
    }

};



/**
 * You should close this connection manually by calling `disconnect()`.
 */
class SocketClient : public PromisedIo {

public:
    struct sockaddr_in inaddr;
    int addrlen = sizeof(inaddr);

    enum class Type {
        Unknown,
        MemoryNode,
        App
    } type = Type::Unknown;


    void disconnect() {
        if (valid()) {
            close(socketFd);
            socketFd = -1;
        }
    }


    adl::TString ip() {
        auto ipUint32 = inaddr.sin_addr.s_addr;
        adl::TString ipStr;
        
        ipStr += adl::TString::to_string(ipUint32 & 0xffu);
        ipStr += '.';
        ipStr += adl::TString::to_string((ipUint32 >> 8u) & 0xffu);
        ipStr += '.';
        ipStr += adl::TString::to_string((ipUint32 >> 16u) & 0xffu);
        ipStr += '.';
        ipStr += adl::TString::to_string((ipUint32 >> 24u) & 0xffu);
        return ipStr;
    }


    adl::uint16_t port() {
        return adl::ntohs(inaddr.sin_port);
    }
};


class SocketServer : public PromisedIo {

public:
    void stop() {
        if (valid()) {
            close(socketFd);
            socketFd = -1;
        }
    }


    ~SocketServer() {
        stop();
    }


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

        close(socketFd);
        socketFd = -1;
        return Status::NETWORK_ERROR;

    }


    SocketClient accept(bool ignoreError = true) {
        SocketClient client;
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




struct Main {

    Genode::Env& env;
    Genode::Heap heap { env.ram(), env.rm() };
    adl::HashMap<int, SocketClient*> clients;  // client's socketFd -> struct

    SocketServer server;

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


    Status init() {
        initAdlAlloc();
        

        return Status::SUCCESS;
    }


    void serveClient(SocketClient& client) {
        Genode::log("Client connected: ", client.ip().c_str(), " [", client.port(), "]");



    }


    Status run() {
        
        adl::uint16_t port = 10000;  // todo
        Status status = server.start(port);
        if (status != Status::SUCCESS) {
            server.stop();
            return status;
        }

        Genode::log("Server started on port: ", port);

        while (true) {
            SocketClient client = server.accept();
            serveClient(client);
            client.disconnect();
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
            }

            status = run();
            if (status != monkey::Status::SUCCESS) {
                Genode::log("Something went wrong when running.. (", adl::int32_t(status), ")");
            }
            
            cleanup();
            
            Genode::log("Bye :D");
        });
    }

};  // struct Main


void Libc::Component::construct(Libc::Env& env) {
    static Main main {env};
}
