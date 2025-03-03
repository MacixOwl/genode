/*

    customized protocol v1 connection for monkey tycoon.

    created on 2025.2.25 at Wujing, Minhang, Shanghai


*/

#pragma once

#include <monkey/net/protocol/Protocol1Connection.h>
#include <monkey/dock/Connection.h>

#include <base/env.h>

namespace monkey::tycoon {


class Protocol1ConnectionDock : public net::Protocol1Connection
{
public:
    dock::Connection* dock = nullptr;

    
    virtual void close() override {
        dock->close(socketFd);
    }


    virtual monkey::Status connect() override {
        close();
        Genode::log("[Socket4 Dock] Connecting to ", ip.toString().c_str(), " : ", port);
        socketFd = dock->socket();

        if (socketFd == -1) {
            return Status::NETWORK_ERROR;
        }

        if (dock->connect(socketFd, ip, port)) {
            return Status::NETWORK_ERROR;
        }
    
        return Status::SUCCESS;
    }


    virtual adl::int64_t recv(void* buf, adl::size_t len) override {
        return dock->recv(socketFd, buf, len);
    }


    virtual adl::int64_t send(const void* buf, adl::size_t len) override {
        return dock->send(socketFd, buf, len);
    }


};


}
