/*

    monkey tycoon : monkey system app driver.

    created on 2025.2.17 at Wujing, Minhang, Shanghai


*/


#pragma once

#include <monkey/net/protocol.h>
#include <monkey/genodeutils/config.h>

#include <base/component.h>


namespace monkey {


    
/**
 * Monkey Tycoon. Works like a daemon.
 * 
 * You activate it, and do your jobs like it doesn't exists.
 */
class Tycoon {

protected:
    Tycoon(const Tycoon&) = delete;
    void operator = (const Tycoon&) = delete;


protected:

    struct PageFaultSignalBridge : public Genode::Entrypoint {
        Tycoon& tycoon;
        Genode::Signal_handler<monkey::Tycoon::PageFaultSignalBridge> _handler;

        void handle() { tycoon.handlePageFaultSignal(); }

        PageFaultSignalBridge(Tycoon& tycoon, Genode::Env& env)
        :
        Genode::Entrypoint(
            env, 
            sizeof(Genode::addr_t) * 2048 /* todo */, 
            "Monkey Tycoon Page Fault Signal Bridge",
            Genode::Affinity::Location()
        ),
        tycoon(tycoon),
        _handler(*this, *this, &monkey::Tycoon::PageFaultSignalBridge::handle)
        {
            env.rm().fault_handler(_handler);
        }
    
    } pageFaultSignalBridge;


    struct {
        struct {
            net::IP4Addr ip;
            adl::uint16_t port;
        } concierge;

        struct {
            adl::ByteArray key;
        } app;
    } config;

    Genode::Env& env;

    struct {
        adl::uintptr_t vaddr;
        adl::size_t size;
        bool manage = false;
    } memSpace;


    struct {
        net::Protocol1Connection concierge;
        adl::HashMap<adl::int64_t, net::Protocol1Connection*> mnemosynes;
    } connections;



public:


    Tycoon(Genode::Env& env) 
    : 
    pageFaultSignalBridge(*this, env),
    env(env)
    {}

    virtual ~Tycoon();
    


    /**
     * 
     * Xml example:
     * <>
     *     <concierge>
     *         <ip>10.0.2.2</ip>
     *         <port>5555</port>
     *     </concierge>
     * </>
     */
    monkey::Status loadConfig(const Genode::Xml_node&);

    void handlePageFaultSignal();

    void disconnectConcierge();

    monkey::Status sync();

    monkey::Status start(adl::uintptr_t vaddr, adl::size_t size);
    void stop();

    monkey::Status checkAvailableMem(adl::size_t* res);

    friend PageFaultSignalBridge;

};


}  // namespace monkey

