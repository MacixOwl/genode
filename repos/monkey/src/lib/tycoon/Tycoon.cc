/*

    monkey tycoon : monkey system app driver.

    created on 2025.2.17 at Wujing, Minhang, Shanghai


*/


#include <monkey/tycoon/Tycoon.h>

using namespace monkey;


Tycoon::~Tycoon() {
    stop();
}


monkey::Status Tycoon::loadConfig(const Genode::Xml_node& xml) {
    Status status = Status::SUCCESS;
    
    try {
        const auto& conciergeNode = xml.sub_node("concierge");
        const auto& ipNode = conciergeNode.sub_node("ip");
        const auto& portNode = conciergeNode.sub_node("port");
        const auto& appNode = xml.sub_node("app");
        const auto& keyNode = appNode.sub_node("key");
        
        status = config.concierge.ip.set(genodeutils::config::getText(ipNode));
        if (status != Status::SUCCESS) {
            throw status;
        }

        config.concierge.port = (adl::uint16_t) genodeutils::config::getText(portNode).toInt64();

        config.app.key = genodeutils::config::getText(keyNode);

        Genode::log(
            "Tycoon: Config loaded.\n"
            "> app key       : ", config.app.key.toString().c_str(), "\n"
            "> concierge ip  : ", config.concierge.ip.toString().c_str(), "\n"
            "> concierge port: ", config.concierge.port
        );
    }
    catch (...) {
        Genode::error("Tycoon: Something went wrong parsing config.");
        status = Status::INVALID_PARAMETERS;
    }

    return status;
}


monkey::Status Tycoon::openConnection(bool concierge, adl::int64_t id, bool force) {

    // close connection if `force`.

    if (force) {
        if (concierge) {
            connections.concierge.close();
        }
        else if (connections.mnemosynes.hasKey(id)) {
            connections.mnemosynes[id]->close();
            adl::defaultAllocator.free(connections.mnemosynes[id]);
            connections.mnemosynes.removeKey(id);
        }
    }
    else {  // just return if connection already exists.
        if (concierge) {
            if (connections.concierge.valid())
                return Status::SUCCESS;
        }
        else if (connections.mnemosynes.hasKey(id)) {
            if (connections.mnemosynes[id]->valid())
                return Status::SUCCESS;
            else {
                adl::defaultAllocator.free(connections.mnemosynes[id]);
                connections.mnemosynes.removeKey(id);
            }
        }
    }
    // todo
}


void Tycoon::handlePageFaultSignal() {
    if (!memSpace.manage) {
        return;
    }

    Genode::Region_map& rm = env.rm();
    Genode::Region_map::State state = rm.state();
    auto addr = state.addr;
    if (addr < memSpace.vaddr && addr >= memSpace.vaddr + memSpace.size) {
        // address not managed by Tycoon.
        return;
    }

    Genode::log("Tycoon: Page fault on ", Genode::Hex(state.addr), " caught.");
    Genode::log(
        "> Type: ",
        (
            state.type == Genode::Region_map::State::READ_FAULT  ? "READ_FAULT"  :
            state.type == Genode::Region_map::State::WRITE_FAULT ? "WRITE_FAULT" :
            state.type == Genode::Region_map::State::EXEC_FAULT  ? "EXEC_FAULT"  : "READY"
        )
    );

    if (state.type == Genode::Region_map::State::EXEC_FAULT) {
        Genode::error("EXEC_FAULT not supported by Tycoon.");
        return;
    }

#if 0
// todo
auto attachResult = rm.attach(
    env.ram().alloc(4096),
    4096,
    0,
    true,
    state.addr
);
//todo
#endif

    auto pageAddr = state.addr & ~0xffful;
    if (pages.hasKey(pageAddr)) {
        // todo
    }
    else {
        Status status = allocPage(pageAddr);
        // todo
    }
}


monkey::Status Tycoon::allocPage(adl::uintptr_t addr) {
    adl::uintptr_t pageAddr = addr & ~0xffful;
    // todo

    
}



monkey::Status Tycoon::freePage(adl::uintptr_t addr) {
    adl::uintptr_t pageAddr = addr & ~0xffful;
    // todo
}


void Tycoon::disconnectConcierge() {
    connections.concierge.close();
}


monkey::Status Tycoon::sync() {
    // todo
}


monkey::Status Tycoon::start(adl::uintptr_t vaddr, adl::size_t size) {
    stop();

    Genode::log(
        "Tycoon: Starting... Addr to be managed: [", 
        Genode::Hex(vaddr), ", ", Genode::Hex(vaddr + size), ") (", size, " bytes)."
    );

    Status status;

    // hello
    {
        net::ProtocolConnection conn;
        conn.ip = config.concierge.ip;
        conn.port = config.concierge.port;
        status = conn.connect();
        if (status != Status::SUCCESS) {
            Genode::error("Failed to connect to concierge.");
            return status;
        }

        if ((status = conn.hello(connections.concierge.version(), false)) != Status::SUCCESS) {
            Genode::error("Failed on Hello.");
            conn.close();
            return status;
        }

        connections.concierge.socketFd = conn.socketFd;
        connections.concierge.ip = conn.ip;
        connections.concierge.port = conn.port;
    }  // end of hello

    
    // auth

    if ((status = connections.concierge.auth(config.app.key)) != Status::SUCCESS) {
        Genode::error("Failed on auth.");
        goto ERROR;
    }

    // get memory nodes

    if ((status = connections.concierge.locateMemoryNodes(memoryNodesInfo)) != Status::SUCCESS) {
        Genode::error("Failed to locate memory nodes.");
        goto ERROR;
    }

    for (auto& it : memoryNodesInfo) {
        Genode::log(
            "Memory node ", it.id, " : ", it.ip.toString().c_str(), " ", it.port,
            " TCP", it.tcpVersion
        );
    }


    memSpace.manage = true; memSpace.vaddr = vaddr; memSpace.size = size;
    Genode::log("Tycoon: Started.");

ERROR:
    disconnectConcierge();
    return status;
}


void Tycoon::stop() {
    if (!memSpace.manage) {
        return;
    }

    Status status = sync();
    if (status != Status::SUCCESS) {
        Genode::error("Tycoon: Something went wrong syncing data.");
    }

    // Close connections.

    Genode::log("Tycoon: Closing connections...");
    disconnectConcierge();
    for (auto it : connections.mnemosynes) {
        it.second->close();
        adl::defaultAllocator.free(it.second);
    }
    connections.mnemosynes.clear();
}



monkey::Status Tycoon::checkAvailableMem(adl::size_t* res) {
    // todo
}
