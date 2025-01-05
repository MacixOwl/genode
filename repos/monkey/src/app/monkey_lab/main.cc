/*
 * Monkey Lab : For some experimental coding.
 *
 * Created on 2024.12.25 at Minhang, Shanghai
 * 
 * 
 * feng.yt [at]  sjtu  [dot] edu [dot] cn
 * gongty  [at] tongji [dot] edu [dot] cn
 * 
 */


#define _BYTEORDER_FUNC_DEFINED
#define _BYTEORDER_PROTOTYPED

#include <base/component.h>
#include <libc/component.h>
#include <base/heap.h>
#include <base/allocator.h>

#include <terminal_session/connection.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>


#include <adl/string.h>
#include <adl/TString.h>
#include <adl/Allocator.h>
#include <adl/arpa/inet.h>
#include <adl/config.h>

#include <monkey/net/TcpIo.h>
#include <monkey/Status.h>
#include <monkey/net/protocol.h>


using namespace Genode;


class LabTcpIo : public monkey::net::TcpIo {
public:
    virtual adl::int64_t recv(void* buf, adl::size_t len) override {
        return read(fd, buf, len);
    };

    virtual adl::int64_t send(void* buf, adl::size_t len) override {
        return write(fd, buf, len);
    };

    int fd;

};


struct Main {
    Genode::Env& env;
    Genode::Sliced_heap slicedHeap { env.ram(), env.rm() };

    void initAdlAlloc() {
        adl::defaultAllocator.init({

            .alloc = [] (adl::size_t size, void* data) {
                return reinterpret_cast<Genode::Sliced_heap*>(data)->alloc(size);
            },
            
            .free = [] (void* addr, adl::size_t size, void* data) {
                reinterpret_cast<Genode::Sliced_heap*>(data)->free(addr, size);
            },
            
            .data = &slicedHeap
        });
    }

    Main(Genode::Env& env) : env(env)
    {
        initAdlAlloc();


        Genode::log("Check Point 1");

        Libc::with_libc([] () {

        
            int s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
            Genode::log("Check Point 2");
            if (s == -1) {
                Genode::error("socket creation failed");
                return ;
            }
            Genode::log("Check Point 3");

            sockaddr_in sockaddr;
            sockaddr.sin_family = PF_INET;
            const uint16_t port = 20024u;
            sockaddr.sin_port = adl::htons (port);
            sockaddr.sin_addr.s_addr = inet_addr("202.120.37.25");

            if (connect(s, (struct sockaddr *)&sockaddr, sizeof(sockaddr))) {
                Genode::error("connect to ", "202.120.37.25", ":", 20024, " failed");
                return ;
            }

            LabTcpIo tcpio;
            tcpio.fd = s;

            monkey::net::protocol::send(tcpio, 0x0101u, nullptr, 0);
            monkey::net::protocol::Response* response;
            monkey::net::protocol::recvResponse(tcpio, &response);

            auto len = response->header.length;

            Genode::log("Vesper response len: ", len );

            close(s);
        });

    }
};


void Libc::Component::construct(Libc::Env& env) {
    static Main main(env);
}
