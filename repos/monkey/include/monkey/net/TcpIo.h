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

namespace monkey::net {


class TcpIo {

public:

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



}  // namespace monkey::net
