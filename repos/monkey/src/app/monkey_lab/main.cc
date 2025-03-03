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

#include "./config.h"

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


    Status initTycoon() {
        Genode::Attached_rom_dataspace configDs { env, "config" };
        auto configRoot = configDs.xml().sub_node("monkey-lab");
        auto tycoonRoot = configRoot.sub_node("monkey-tycoon");

        monkey::Tycoon::InitParams params;

        // determine params.nbuf
        {
            auto availMem = env.pd().avail_ram().value;
            availMem -= MONKEY_LAB_LOCAL_HEAP_MEMORY_RESERVED;
            auto availPages = availMem / 4096;
            if (availPages < 2) {
                Genode::error("Only ", availPages, " page(s) left. R u kidding me???");
                return Status::OUT_OF_RESOURCE;
            }

            params.nbuf = availPages;

            // todo
params.nbuf = 1;  // for testing.
            // todo
        }

        tycoon.init(tycoonRoot, params);

        return Status::SUCCESS;
    }

    
    AppMain(Genode::Env& env) : env(env), tycoon(env)
    {
        Libc::with_libc([&] () {
            
            Genode::log("monkey lab main");
            initAdlAlloc();

            Status status = Status::SUCCESS;

            try {
                status = initTycoon();
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


            ((char*)0x100002000030)[2] = 'd';

            Genode::log("((char*)0x100000000030)[2]: ", Genode::Hex( ((char*)0x100000000030)[2]));
            Genode::log("((char*)0x100002000030)[2]: ", Genode::Hex(((char*)0x100002000030)[2]));

            tycoon.stop();
            Genode::log("Monkey Lab end.");
        
        });
    }
};


void Libc::Component::construct(Libc::Env& env) {
    static AppMain appMain(env);
}
