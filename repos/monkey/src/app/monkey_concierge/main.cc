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

#include <base/component.h>
#include <libc/component.h>

#include <monkey/Status.h>

#include <monkey/net/TcpIo.h>
#include <monkey/net/protocol.h>


struct Main {

    Genode::Env& env;


    monkey::Status init() {

        return monkey::Status::SUCCESS;
    }


    monkey::Status run() {


        return monkey::Status::SUCCESS;
    }


    void cleanup() {

    }


    Main(Genode::Env& env) : env {env} {
        Genode::log("Hello :D This is Monkey Concierge");
        
        monkey::Status status = init();
        if (status != monkey::Status::SUCCESS) {
            Genode::log("Something went wrong on init.. (", adl::int32_t(status), ")");
        }

        status = run();
        if (status != monkey::Status::SUCCESS) {
            Genode::log("Something went wrong when running.. (", adl::int32_t(status), ")");
        }
        
        cleanup();
        
        Genode::log("Bye :D");
    }

};  // struct Main


void Libc::Component::construct(Libc::Env& env) {
    static Main main {env};
}
