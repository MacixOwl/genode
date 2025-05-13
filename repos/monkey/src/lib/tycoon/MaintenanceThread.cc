/*

    Backgound maintenance thread for monkey tycoon.

    created on 2025.5.13 at Wujing, Minhang, Shanghai


*/

#include <monkey/tycoon/MaintenanceThread.h>

using namespace monkey;


tycoon::MaintenanceThread::MaintenanceThread(
    Genode::Env& env,
    Tycoon& tycoon
) :
    Genode::Thread(env, "Tycoon Maintenance Thread", 16 * 1024),
    tycoon(tycoon)
{

}


void tycoon::MaintenanceThread::doMaintenance() {
    // TODO
}


void tycoon::MaintenanceThread::entry() {
    Genode::log("[Tycoon Maintenance Thread] Started.");
    
    while (true) {
        doMaintenance();
    }

    // TODO 1: how to stop this loop?
    // TODO 2: how to sleep during each maintenance?


    Genode::log("[Tycoon Maintenance Thread] Stopped.");
}


void tycoon::MaintenanceThread::stop() {
    // TODO
}



void tycoon::MaintenanceThread::stopAndJoin() {
    this->stop();
    this->join();
}

