
#include <base/component.h>
#include <base/log.h>
#include <base/heap.h>
#include <root/component.h>
#include <base/rpc_server.h>
#include <dataspace/capability.h>
#include <dataspace/client.h>
#include <base/attached_ram_dataspace.h>
#include <timer_session/connection.h>
#include <cpu/atomic.h>

#include <pivot/pivot_session.h>
#include <memory/memory_connection.h>
#include <kv/kv_connection.h>
#include <fs/fs_connection.h>
#include <fs_memory/fs_memory_connection.h>

namespace MtsysPivot {
	struct Component_state;
	struct Session_component;
	struct Root_component;
	struct Main;
}


const int IPC_UPDATE_INTERVAL = 500; // update period in ms
const double IPC_STATS_FADEOUT = 0.8; // in which rate the IPC stats fade out 


struct MtsysPivot::Component_state
{	
	int cid_service2service[MAX_SERVICE][MAX_SERVICE] = { 0 };
    int pivot_appid_used[MAX_USERAPP] = { 0 };
    int pivot_ipc_app2comp[MAX_USERAPP][MAX_COMPSVC] = { 0 };
    int pivot_ipc_service2service[MAX_SERVICE][MAX_SERVICE] = { 0 };

    volatile int lock_state;
	volatile int service_main_id[MAX_SERVICE] = { 0 };

	Genode::Env &env;
	Timer::Connection timer;

	MtsysMemory::Connection mem_obj;
	MtsysKv::Connection kv_obj;
	MtsysFs::Connection fs_obj;
	MtsysFsMemory::Connection fs_memory_obj;

	Timer::Periodic_timeout<MtsysPivot::Component_state> timeout;

	int transform_service_main(int service_id, int* comp_ids, int num){
		int a = (1 << service_id);
		for (int i = 0; i < num; i++) {
			a |= (1 << comp_ids[i]);
		}
		int res = 0;
		while (!res){
			res = Genode::cmpxchg(&lock_state, 0, 1);
		} 

		// notice the services to transform their activation status
		switch (service_id) {
			case SID_MEMORY_SERVICE:
				if (a == 1) {
					mem_obj.Transform_activation(1);
				}
				break;
			case SID_KV_SERVICE:
				break;
			default:
				break;
		}

		service_main_id[service_id] = a;
		res = Genode::cmpxchg(&lock_state, 1, 0);
		return (!res);
	}

	void update_ipc_stats(Genode::Duration) {
		// Genode::log("Updating IPC stats");
		// first transform the service main id
		int comp_ids[4] = {SID_MEMORY_SERVICE, SID_KV_SERVICE, SID_BLOCK_SERVICE, 
			SID_FS_SERVICE};
		// fake transform for an example
		transform_service_main(SID_KV_SERVICE, nullptr, 0);
		transform_service_main(SID_FS_SERVICE, nullptr, 0);

		int comp_id = 0;
		for (int i = 0; i < MAX_USERAPP; i++) {
			for (int j = 0; j < MAX_SERVICE; j++) {
				comp_id = service_main_id[j];
				pivot_ipc_app2comp[i][comp_id] = 
					(int)((double)(pivot_ipc_app2comp[i][comp_id]) * IPC_STATS_FADEOUT);
				pivot_ipc_service2service[j][j] = 
					(int)((double)(pivot_ipc_service2service[j][j]) * IPC_STATS_FADEOUT);
			}
		}
		// update kv service ipc stats
		int kv_comp_id = service_main_id[SID_KV_SERVICE];
		// Genode::log("KV service main id: ", kv_comp_id);
		for (int i = 0; i < 5; i++) {
			int new_ipc = kv_obj.get_IPC_stats(i);
			pivot_ipc_app2comp[i][kv_comp_id] += new_ipc;
		}
		// log them for now
		Genode::log("IPC stats for Kv service: ", pivot_ipc_app2comp[0][kv_comp_id], 
			" ", pivot_ipc_app2comp[1][kv_comp_id], " ", pivot_ipc_app2comp[2][kv_comp_id], 
			" ", pivot_ipc_app2comp[3][kv_comp_id], " ", pivot_ipc_app2comp[4][kv_comp_id]);

		// update fs service ipc stats
		int fs_comp_id = service_main_id[SID_FS_SERVICE];
		// Genode::log("FS service main id: ", fs_comp_id);
		for (int i = 0; i < 5; i++) {
			int new_ipc = fs_obj.get_IPC_stats(i);
			pivot_ipc_app2comp[i][fs_comp_id] += new_ipc;
		}
		// log them for now
		Genode::log("IPC stats for FS service: ", pivot_ipc_app2comp[0][fs_comp_id], 
			" ", pivot_ipc_app2comp[1][fs_comp_id], " ", pivot_ipc_app2comp[2][fs_comp_id], 
			" ", pivot_ipc_app2comp[3][fs_comp_id], " ", pivot_ipc_app2comp[4][fs_comp_id]);

		return;
	}

	Component_state(Genode::Env &env)
	: lock_state(0),
	env(env),
	timer(env),
	mem_obj(env),
	kv_obj(env),
	fs_obj(env),
	fs_memory_obj(env),
	timeout(timer, *this, &Component_state::update_ipc_stats, 
		Genode::Microseconds{IPC_UPDATE_INTERVAL * 1000})
    {
		// transform all service main ids to itself, must at first
		for (int i = 0; i < MAX_SERVICE; i++) {
			transform_service_main(i, nullptr, 0);
		}
		
		// init the cid service2service table
		MtsysKv::cid_4service cids = kv_obj.get_cid_4services();
		Genode::log("KV service cids: ", cids.cid_4memory, " ", cids.cid_fake);	
		cid_service2service[SID_MEMORY_SERVICE][SID_KV_SERVICE] = cids.cid_4memory;

    }
};


struct MtsysPivot::Session_component : Genode::Rpc_object<Session>
{	
	int client_id;
	Component_state &state;

	void Pivot_hello() override {
		Genode::log("Hi, Mtsys Pivot for client ", client_id); 
        Genode::log("Pivot init lock state: ", state.lock_state);
	}

    int Pivot_App_getid() override {
        return client_id;
    }

    void Pivot_IPC_stats(int appid, int num) override {
        Genode::log("Pivot IPC stats: appid ", appid, " num ", num);
        state.pivot_ipc_app2comp[appid][num]++; // increment the IPC count
    }

	MtsysPivot::Service_Main_Id Pivot_service_mainIDs() override {
		Genode::log("Getting service main IDs");
		MtsysPivot::Service_Main_Id ids;
		for (int i = 0; i < MAX_SERVICE; i++) {
			ids.id_array[i] = state.service_main_id[i];
		}
		return ids;
	}

	Session_component(int id, Component_state &s) 
	: client_id(id),
	state(s)
	{ }

};


class MtsysPivot::Root_component
:
	public Genode::Root_component<Session_component>
{	
	private:
		Component_state stat;
		int client_used[MAX_USERAPP] = { 0 };
	protected:

		Session_component *_create_session(const char *) override
		{
			Genode::log("Creating MtsysPivot session");
			int new_client_id = -1;
			for (int i = 0; i < MAX_USERAPP; i++) {
				if (client_used[i] == 0) {
					client_used[i] = 1;
					new_client_id = i;
					break;
				}
			}
			if (new_client_id == -1) {
				Genode::log("[[ERROR]]No more apps can be created");
				return nullptr;
			}
            stat.pivot_appid_used[new_client_id] = 1;
			return new (md_alloc()) Session_component(new_client_id, stat);
		}


		virtual void _destroy_session(Session_component* session) override {
			// we should free client id in client_used
			auto& cid = session->client_id;

			if (cid < 0 || cid >= MAX_USERAPP || !client_used[cid] || !stat.pivot_appid_used[cid]) {
				Genode::log("[Critical] _destroy_session: Bad client id: %d\n", cid);
				goto END;
			}

			client_used[cid] = stat.pivot_appid_used[cid] = 0;

END:
			// call super method
			Genode::Root_component<Session_component>::_destroy_session(session);
		}

	public:

		Root_component(Genode::Entrypoint &ep,
		               Genode::Allocator &alloc, 
                       Genode::Env &env)
		:
			Genode::Root_component<Session_component>(ep, alloc),
			stat(env),
			client_used()
		{
			Genode::log("Creating MtsysPivot root component");
		}
};


struct MtsysPivot::Main
{
	Genode::Env &env;

	/*
	 * A sliced heap is used for allocating session objects - thereby we
	 * can release objects separately.
	 */
	Genode::Sliced_heap sliced_heap { env.ram(), env.rm() };

	MtsysPivot::Root_component root { env.ep(), sliced_heap, env };

	Main(Genode::Env &env) 
    : 
    env(env)
	{
		/*
		 * Create a RPC object capability for the root interface and
		 * announce the service to our parent.
		 */
		env.parent().announce(env.ep().manage(root));
        Genode::log("MtsysPivot service is ready");
	}
};


void Component::construct(Genode::Env &env)
{
	static MtsysPivot::Main main(env);

}
