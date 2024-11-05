#ifndef _INCLUDE__PIVOT__CONNECTION_H_
#define _INCLUDE__PIVOT__CONNECTION_H_


#include <base/connection.h>
#include <timer_session/connection.h>

#include <pivot/pivot_client.h>
#include <kv/kv_connection.h>
#include <memory/memory_connection.h>

namespace MtsysPivot { 
	struct Connection; 
	struct ServiceHub;
}


struct MtsysPivot::Connection : Genode::Connection<Session>, Session_client
{
	Connection(Genode::Env &env)
	:
		/* create session */
		Genode::Connection<MtsysPivot::Session>(env, Label(),
		                                   Ram_quota { 8*1024 }, Args()),
		/* initialize RPC interface */
		Session_client(cap())
	{ }
};


struct MtsysPivot::ServiceHub{
	Genode::Env &env;
	Timer::Connection timer_obj;
	MtsysPivot::Connection pivot_obj;

	int service_main_id_cache[MAX_SERVICE] = { 0 };
	int main_id_cache_dirty = 1;
	MtsysMemory::Connection mem_obj;
	MtsysKv::Connection kv_obj;

	ServiceHub(Genode::Env &env)
	: env(env), 
	timer_obj(env), 
	pivot_obj(env),
	service_main_id_cache(),
	mem_obj(env), 
	kv_obj(env)
	{ }

	void update_service_main_id_cache(){
		if (main_id_cache_dirty){
			MtsysPivot::Service_Main_Id ids = pivot_obj.Pivot_service_mainIDs();
			for (int i = 0; i < MAX_SERVICE; i++) {
				service_main_id_cache[i] = ids.id_array[i];
			}
			main_id_cache_dirty = 0;
		}
	}

	// APIs from Timer
	void Time_usleep(int usec) { timer_obj.usleep(usec); }
	Genode::Milliseconds Time_now_ms() { 
		return timer_obj.curr_time().trunc_to_plain_ms(); }
	Genode::Microseconds Time_now_us() { 
		return timer_obj.curr_time().trunc_to_plain_us(); }

	// APIs from pivot
	void Pivot_hello() {
		pivot_obj.Pivot_hello(); 
	}
	int Pivot_App_getid() { 
		return pivot_obj.Pivot_App_getid(); 
	}
	void Pivot_IPC_stats(int appid, int num) { 
		pivot_obj.Pivot_IPC_stats(appid, num); 
	}

	// APIs from memory
	int Memory_hello() { 
		while (1){
			int res = -1;
			switch (service_main_id_cache[SID_MEMORY_SERVICE])
			{
			case 1: // only memory service
				res = mem_obj.Memory_hello(); 
				break;
			default:
				break;
			}
			if (res == -1){
				Genode::log("[INFO] Memory service not available or Main ID cache not updated");
				main_id_cache_dirty = 1;
				update_service_main_id_cache();
			}
			else{
				return res;
			}
		}
	}

	genode_uint64_t query_free_space() { return mem_obj.query_free_space(); }

	// APIs from kv
	int Kv_hello() { 
		while (1){
			int res = -1;
			switch (service_main_id_cache[SID_KV_SERVICE])
			{
			case 2: // only kv service
				res = kv_obj.Kv_hello(); 
				break;
			default:
				break;
			}
			if (res == -1){
				Genode::log("[INFO] Kv service not available or Main ID cache not updated");
				main_id_cache_dirty = 1;
				update_service_main_id_cache();
			}
			else{
				return res;
			}
		}
	}
	// MtsysKv::cid_4service get_cid_4services() { return kv_obj.get_cid_4services(); }
	void null_function() { kv_obj.null_function(); }
	int get_IPC_stats(int client_id) { return kv_obj.get_IPC_stats(client_id); }

};




#endif /* _INCLUDE__PIVOT__CONNECTION_H_ */
