/*
 * Monkey Net :: TcpIo
 *
 * Created on 2024.12.27 at Minhang, Shanghai
 * 
 * 
 * feng.yt [at]  sjtu  [dot] edu [dot] cn
 * gongty  [at] tongji [dot] edu [dot] cn
 * 
 */


#pragma once

#include <adl/sys/types.h>
#include <monkey/Status.h>
#include <unistd.h>

namespace monkey::net {


class TcpIo {

public:
    virtual ~TcpIo() {}

    /**
     * 
     * @param len Buffer size.
     */
    virtual adl::int64_t recv(void* buf, adl::size_t len) = 0;


    /**
     * 
     * @param len Size of data.
     */
    virtual adl::int64_t send(const void* buf, adl::size_t len) = 0;

};


class PromisedSocketIo : public TcpIo {
public:
    int socketFd = -1;

    /**
     * If socket is detached, TcpIo won't manage socket's life cycle.
     * 
     * For example, when destructing a TcpIo, 
     * only attached socket would be closed automatically.
     */
    bool socketDetached = false;

    virtual void close();

    virtual ~PromisedSocketIo() override { 
        if (!socketDetached)
            close(); 
    }

    virtual bool valid() { return socketFd > 1; }

    virtual adl::int64_t recv(void* buf, adl::size_t len) override;

    virtual adl::int64_t send(const void* buf, adl::size_t len) override;

};



}  // namespace monkey::net
