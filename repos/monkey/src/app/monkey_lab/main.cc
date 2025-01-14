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
#include <base/attached_ram_dataspace.h>
#include <region_map/client.h>

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
#include <adl/collections/ArrayList.hpp>

#include <monkey/net/TcpIo.h>
#include <monkey/Status.h>
#include <monkey/net/protocol.h>
#include <monkey/crypto/rc4.h>
#include <monkey/memory/map.h>

using namespace Genode;


template<typename WriteFunc>
static void writeHex(unsigned long num, int width, int upperCase, WriteFunc out) {
    int digitWidth = 1;
    
    {
        unsigned long needle = 0xF;
        while (needle < num) {
            needle <<= 4;
            needle |= 0xF;
            digitWidth ++;
        }

        if (width > 0) {
            for (int w = digitWidth; w < width; w++)
                out('0');
        }
    }


    while (digitWidth) {
        unsigned long digit = num >> ((digitWidth - 1) * 4);
        digit &= 0xF;
        if (digit >= 0 && digit <= 9) {
            out(digit + '0');
        } else {  // digit is in [a, f]
            out(digit - 0xA + (upperCase ? 'A' : 'a'));
        }

        digitWidth --;
    }

}


template<typename WriteFunc>
static void writeLine(int addr, int buf[], int len, int upperCase, WriteFunc out) {
    writeHex(addr, 8, upperCase, out);
    out(' ');
    out(' ');

    for (int i = 0; i < 16; i++) {
        if (i == 8) {
            out(i < len ? '-' : ' ');
            out(' ');
        }
        if (i < len) {
            writeHex(buf[i] & 0xFF, 2, upperCase, out);
        } else {
            out(' ');
            out(' ');
        }
        out(' ');
    }

    out(' ');
    out(' ');
    out(' ');
    out(' ');
    for (int i = 0; i < len; i++) {
        if (buf[i] >= 33 && buf[i] <= 126)
            out(buf[i]);
        else
            out('.');
    }

    out('\n');
}


template<typename ReadFunc, typename WriteFunc>
size_t hexView(int upperCase, ReadFunc in, WriteFunc out) {
    int buf[16];
    int pos = 0;  // One after last occupied.

    size_t addr = 0;
    size_t bytesRead = 0;

    while (true) {
        int ch = in();
        if (ch == EOF)
            break;

        buf[pos++] = ch;
        if (pos == 16) {
            writeLine(addr, buf, 16, upperCase, out);
            addr += 16;
            pos = 0;
        }
    }


    if (pos)
        writeLine(addr, buf, pos, upperCase, out);
    return bytesRead;
}



class PageFaultHandler : public Entrypoint {
protected:

    Genode::Env& env;
    Region_map& rm;
    Signal_handler<PageFaultHandler> _handler;

    void handleFault() {

        Genode::log("handle fault");
        Region_map::State state = rm.state();

        auto faultType = state.type == Region_map::State::READ_FAULT  ? "READ_FAULT"  :
			       state.type == Region_map::State::WRITE_FAULT ? "WRITE_FAULT" :
			       state.type == Region_map::State::EXEC_FAULT  ? "EXEC_FAULT"  : "READY";

        Genode::log("addr: ", Hex(state.addr, Hex::PREFIX), ", type: ", faultType);

        auto attachResult = rm.attach(
            env.ram().alloc(4096),
            4096,
            0,
            true,
            state.addr
        );

        Genode::log("attached: ", (void*)(attachResult));

        Region_map_client map = Region_map_client { env.pd().address_space() };

        faultType = state.type == Region_map::State::READ_FAULT  ? "READ_FAULT"  :
			       state.type == Region_map::State::WRITE_FAULT ? "WRITE_FAULT" :
			       state.type == Region_map::State::EXEC_FAULT  ? "EXEC_FAULT"  : "READY";

        Genode::log("addr: ", Hex(state.addr, Hex::PREFIX), ", type: ", faultType);

    }


public:
    PageFaultHandler(Genode::Env& env, Region_map& rm) 
    : 
    Entrypoint {env, sizeof(addr_t) * 2048, "PageFaultHandler", Affinity::Location()},
    env {env},
    rm {rm},
    _handler {*this, *this, &PageFaultHandler::handleFault}
    {
        rm.fault_handler(_handler);
    }
};



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
    Genode::Heap heap { env.ram(), env.rm() };

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

    Main(Genode::Env& env) : env(env)
    {
        Genode::log("monkey lab main");
        initAdlAlloc();

        adl::ArrayList<monkey::memory::MemoryMapEntry> list;
        Genode::log("monkey lab main cp2");
        monkey::memory::getMemoryMap(env, list);
        Genode::log("monkey lab main cp3");

        for (auto& it : list) {
            auto type = it.type == decltype(it.type)::FREE ? "FREE" : (it.type == decltype(it.type)::OCCUPIED ? "OCCU" : "UNKNOWN");
            Genode::log("addr: ", Genode::Hex(it.addr), ", size: ", Genode::Hex(it.size), ", type: ", type);
        }

        return;



#if 1
        // about getting available ram and alloc & attach.
        try {
            Genode::log("mem avai: ", env.pd().avail_ram().value);

            Region_map_client regionMap {env.pd().address_space()};

            Genode::Ram_dataspace_capability dataspace = env.ram().alloc(8192);
            

            Genode::log("Check Point 2");
            //env.rm().attach(dataspace, 8192, 0, true, 0x17000);
            
            Genode::log("Check Point 3");
            char* p = (char*)0x17000;
            p[0] = 'x';
            Genode::log("Check Point 4");
            p[1] = 'd';
            p[3] = '\0';
            Genode::log((const char*) p);


            Genode::log("Check Point 5");


        }
        catch (Genode::Region_map::Region_conflict) {
            
            Genode::log("Region_conflict Exception.");
        }
        catch (Genode::Region_map::Invalid_dataspace) {
            
            Genode::log("Invalid_dataspace Exception.");
        } 
        catch (...) {
            Genode::log("Exception.");
        }

        return;
#endif

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
    static PageFaultHandler faultHandler(env, env.rm());
    static Main main(env);
}
