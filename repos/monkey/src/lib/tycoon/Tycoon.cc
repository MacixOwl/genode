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

    if (force) { // close connection if `force`.
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


    // ------ open new connection. ------

    Status status = Status::SUCCESS;
    net::ProtocolConnection conn;

    // determine ip and port.
    if (concierge) {
        conn.ip = config.concierge.ip;
        conn.port = config.concierge.port;
    }
    else {
        status = Status::NOT_FOUND;
        for (auto& it : memoryNodesInfo) {
            if (it.id == id) {
                status = Status::SUCCESS;
                conn.ip = it.ip;
                conn.port = it.port;
                break;
            }
        }
        if (status != Status::SUCCESS)
            return status;
    }
    

    // connect
    if ((status = conn.connect()) != Status::SUCCESS) {
        Genode::error("Failed to connect to ", conn.ip.toString().c_str(), ":", conn.port);
        return status;
    }

    // hello
    if ((status = conn.hello(net::Protocol1Connection::VERSION, false)) != Status::SUCCESS) {
        Genode::error("Failed on Hello.");
        conn.close();
        return status;
    }

    net::Protocol1Connection connV1;
    connV1.ip = conn.ip;
    connV1.port = conn.port;
    connV1.socketFd = conn.socketFd;

    // auth
    if ((status = connV1.auth(config.app.key)) != Status::SUCCESS) {
        Genode::error("Failed on auth.");
        connV1.close();
        return status;
    }

    // save this connection
    net::Protocol1Connection* pConn = nullptr;
    if (concierge) {
        pConn = &connections.concierge;
    }
    else {
        auto newConn = adl::defaultAllocator.alloc<net::Protocol1Connection>();
        if (!newConn) {
            connV1.close();
            return Status::OUT_OF_RESOURCE;
        }

        connections.mnemosynes[id] = newConn;
        pConn = newConn;
    }

    pConn->ip = connV1.ip;
    pConn->port = connV1.port;
    pConn->socketFd = connV1.socketFd;
    return Status::SUCCESS;
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

    Status status = openConnection(true);
    if (status != Status::SUCCESS) {
        return status;
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
    adl::size_t sum = 0;

    for (auto& it : memoryNodesInfo) {
        Status status = openConnection(false, it.id);
        if (status != Status::SUCCESS) {
            continue;
        }

        adl::size_t mem;
        status = connections.mnemosynes[it.id]->checkAvailMem(&mem);
        if (status == Status::SUCCESS) {
            Genode::log(
                "Tycoon: Discovered ", mem, " bytes of RAM on ", 
                it.ip.toString().c_str(), ":", it.port
            );
            sum += mem;
        }
        else {
            Genode::error("Something went wrong when connecting to mnemosyne ", it.id);
        }
    }

    *res = sum;

    return Status::SUCCESS;
}
