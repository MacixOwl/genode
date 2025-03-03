/*
    TCP/IPv4 Socket

    created on 2025.2.6 at Xiangzhou, Zhuhai, Guangdong

    gongty [at] tongji [dot] edu [dot] cn
*/

#include <monkey/net/Socket4.h>

#include <sys/socket.h>

#include <base/log.h>
#include <adl/TString.h>


using namespace monkey;


namespace monkey::net {


Status Socket4::connect() {
    close();

    Genode::log("[Socket4] Connecting to ", ip.toString().c_str(), " : ", port);

    socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketFd == -1) {
        return Status::NETWORK_ERROR;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = adl::htons(port);
    addr.sin_addr.s_addr = ip.ui32;

    if (::connect(socketFd, (struct sockaddr*)&addr, sizeof(addr))) {
        return Status::NETWORK_ERROR;
    }

    return Status::SUCCESS;
}


Status Socket4::start(adl::size_t maxClients) {
    if (maxClients > INT32_MAX) {
        return Status::INVALID_PARAMETERS;
    }
    
    close();
    
    socketFd = socket(AF_INET, SOCK_STREAM, 0);

    if (socketFd == 0) {
        return Status::NETWORK_ERROR;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip.ui32;
    addr.sin_port = adl::htons(port);

    if (bind(socketFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close();
        return Status::NETWORK_ERROR;
    }

    if (listen(socketFd, adl::int32_t(maxClients)) < 0) {
        close();
        return Status::NETWORK_ERROR;
    }

    Genode::log("Server started. Addr: ", ip.toString().c_str(), " : ", port);
    return Status::SUCCESS;
}


Socket4 Socket4::accept(bool ignoreError) {
    while (true) {
        Genode::log("Waiting for connection...");

        struct sockaddr_in addr;
        socklen_t addrlen = sizeof(addr);
        auto fd = ::accept(socketFd, (struct sockaddr*)&addr, &addrlen);

        if (fd != -1) {
            Socket4 s;
            s.socketFd = fd;
            s.ip.ui32 = addr.sin_addr.s_addr;
            s.port = adl::ntohs(addr.sin_port);
            return s;
        }
        else if (!ignoreError) {
            return Socket4 {};
        }
    }
}


}  // namespace monkey::net
