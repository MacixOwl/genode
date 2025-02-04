/*
 * Monkey Mnemosyne : Memory provider.
 *
 * Created on 2025.2.4 at Xiangzhou, Zhuhai, Guangdong
 * 
 * 
 * gongty  [at] tongji [dot] edu [dot] cn
 * 
 */


#include "./main.h"

#include <adl/sys/types.h>
#include <libc/component.h>

#include <base/attached_rom_dataspace.h>

#include <monkey/Status.h>

#include <monkey/net/protocol.h>

#include <monkey/genodeutils/config.h>

using namespace monkey;


static const adl::TString memSizeToHumanReadable(adl::size_t size) {
    const char* levels[] = {
        " B", " KB", " MB", " GB", " TB"
    };

    for (adl::size_t i = 0; i < sizeof(levels) / sizeof(levels[0]); i++) {
        if (size < 8192)
            return adl::TString::to_string(size) + levels[i];
        size /= 1024;
    }

    return "+oo";  // Treat as infinity.
}


static void initAdlAlloc(Genode::Heap& heap) {
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


Status MnemosyneMain::loadConfig() {
    Genode::Attached_rom_dataspace configDs { env, "config" };
    auto configRoot = configDs.xml().sub_node("monkey-mnemosyne");
    auto conciergeNode = configRoot.sub_node("concierge");
    auto mnemosyneNode = configRoot.sub_node("mnemosyne");

    Status status = Status::SUCCESS;

#define IF_STATUS_NOT_SUCCESS_THEN_RETURN() do { if (status != Status::SUCCESS) return status; } while (0)

    // concierge
    {
        auto ipNode = conciergeNode.sub_node("ip");
        status = config.concierge.ip.set(genodeutils::config::getText(ipNode));
        IF_STATUS_NOT_SUCCESS_THEN_RETURN();

        auto portNode = conciergeNode.sub_node("port");
        config.concierge.port = (adl::uint16_t) genodeutils::config::getText(portNode).toInt64();

    }

    // mnemosyne
    {
        auto ipNode = mnemosyneNode.sub_node("ip");
        status = config.mnemosyne.ip.set(genodeutils::config::getText(ipNode));
        IF_STATUS_NOT_SUCCESS_THEN_RETURN();
        
        auto portNode = mnemosyneNode.sub_node("port");
        config.mnemosyne.port = (adl::uint16_t) genodeutils::config::getText(portNode).toInt64();
        
        auto listenPortNode = mnemosyneNode.sub_node("listen-port");
        config.mnemosyne.listenPort = (adl::uint16_t) genodeutils::config::getText(listenPortNode).toInt64();
    }


#undef IF_STATUS_NOT_SUCCESS_THEN_RETURN

    return status;
}


Status MnemosyneMain::init() {
    initAdlAlloc(heap);
    Status status = Status::SUCCESS;

    try {
        status = loadConfig();
    } 
    catch (...) {
        Genode::error("Exception on loading config. Check your .run file.");
        status = Status::INVALID_PARAMETERS;
    }

    if (status != Status::SUCCESS)
        return status;
    
    return status;
}


Status MnemosyneMain::run() {
    Status status = Status::SUCCESS;

    // todo

    return status;
}


void MnemosyneMain::cleanup() {
    
}


MnemosyneMain::MnemosyneMain(Genode::Env& env) : env {env} {
    
    Genode::log("Welcome to Monkey Mnemosyne.");

    Libc::with_libc([&] () {

        Status status = init();
        if (status != Status::SUCCESS) {
            Genode::error("Failed on init.");
            cleanup();
            return;
        }

        
        adl::size_t ramAvai = env.pd().avail_ram().value;
        adl::size_t ramTotal = env.pd().ram_quota().value;
        Genode::log(
            "Ram Total: ", memSizeToHumanReadable(ramTotal).c_str(),
            ", Ram Avai: ", memSizeToHumanReadable(ramAvai).c_str()
        );


        status = run();
        if (status != Status::SUCCESS) {
            Genode::warning("Something went wrong on runtime.");
        }


        cleanup();
    });

    Genode::log("Bye :D");
}


void Libc::Component::construct(Libc::Env& env) {
    static MnemosyneMain main {env}; 
}
