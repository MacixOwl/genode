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


#include <adl/sys/types.h>

#include <libc/component.h>
#include <base/heap.h>

#include <adl/collections/HashMap.hpp>

#include <monkey/genodeutils/memory.h>
#include <monkey/tycoon/Tycoon.h>

using namespace Genode;
using namespace monkey;


struct AppMain {
    Genode::Env& env;
    Genode::Heap heap { env.ram(), env.rm() };
    Tycoon tycoon;

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
        auto configRoot = configDs.xml().sub_node("monkey-lab");
        auto tycoonRoot = configRoot.sub_node("monkey-tycoon");
        tycoon.loadConfig(tycoonRoot);
        return Status::SUCCESS;
    }

    
    AppMain(Genode::Env& env) : env(env), tycoon(env)
    {
        Libc::with_libc([&] () {
            
            Genode::log("monkey lab main");
            initAdlAlloc();

            Status status = Status::SUCCESS;

            try {
                status = loadConfig();
            }
            catch (...) {
                Genode::error("Something went wrong loading config. Exit.");
                return;
            }

            if (status != Status::SUCCESS) {
                return;
            }

            // probe vaddr for Tycoon
            {
                adl::ArrayList<monkey::genodeutils::MemoryMapEntry> list;
                monkey::genodeutils::getMemoryMap(env, list);

                bool found = false;
                adl::uintptr_t addr;
                adl::size_t size;
                for (auto& it : list) {
                    auto type = it.type == decltype(it.type)::FREE ? 
                        "FREE" : (it.type == decltype(it.type)::OCCUPIED ? "OCCU" : "UNKNOWN");
                    Genode::log(
                        "Mem Probe: ",
                        "addr: ", 
                        Genode::Hex(it.addr), 
                        ", size: ", 
                        Genode::Hex(it.size), 
                        ", type: ", 
                        type
                    );

                    if (!found && it.size >= 8ull * 1024 * 1024 * 1024 && it.type == decltype(it.type)::FREE) {
                        Genode::log("> Selected.");
                        found = true;
                        size = it.size;
                        addr = it.addr;
                    }
                }

                if (!found) {
                    Genode::error("No place for Tycoon to manage.");
                    return;
                }
                
                tycoon.start(addr, size);
                adl::size_t tycoonRam = 0;
                tycoon.checkAvailableMem(&tycoonRam);
                Genode::log("There are ", tycoonRam, " bytes ram on monkey memory network.");
            } // end of memory probing

            ((char*)0x100000000030)[2] = 'c';
            Genode::log("Page fault return");
        
        });
    }
};


void Libc::Component::construct(Libc::Env& env) {
    static AppMain appMain(env);
}
