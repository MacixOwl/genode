
#include <base/component.h>
#include <base/log.h>
#include <base/heap.h>
#include <root/component.h>
#include <base/rpc_server.h>
#include <cpu/atomic.h>

#include <pivot/pivot_session.h>
#include <memory/memory_session.h>
#include <memory/local_allocator.h>
#include <kv/kv_connection.h>

namespace MtsysMemory {
	struct Component_state;
	struct Session_component;
	struct Root_component;
	struct Main;
}


const int MEM_LEVEL_SIZE = (1 << 27);
const int MAX_MEM_SIZE = MEM_LEVEL_SIZE * DS_SIZE_LEVELS;
const int SINGLE_DS_NUM = (1 << 9);
const int MAX_MEM_CAP = MEM_LEVEL_SIZE / DS_MIN_SIZE * 2 + SINGLE_DS_NUM;
const int MEM_HASH_SIZE = 8192;
const int MEM_HASH_CAPACITY = 32;


int start_idxof_level(int level) {
	int x = 0;
	for (int i = 0; i < level; i++) {
		x += (MEM_LEVEL_SIZE / DS_MIN_SIZE) >> i;
	}
	return x;
}


struct MtsysMemory::Component_state
{	
	Genode::Env &env;
	Genode::Sliced_heap sliced_heap { env.ram(), env.rm() };
	genode_uint64_t address_base;
	genode_uint64_t address_end;
	genode_uint64_t address_free;
	genode_uint64_t address_used;

	Genode::Attached_ram_dataspace **ds_list;
	Genode::Attached_ram_dataspace **ds_single;
	int head_idx[DS_SIZE_LEVELS] = { 0 };
	int *used_bitmaps;
	int head_idx_single = 0;
	int *used_bitmaps_single;

	// add a hash table to map addr to ds_list/ds_single
	// 0 -- MEM_LEVEL_SIZE / DS_MIN_SIZE * 2 -1 are for ds_list,
	// MEM_LEVEL_SIZE / DS_MIN_SIZE * 2 -- 
	// MEM_LEVEL_SIZE / DS_MIN_SIZE * 2 + SINGLE_DS_NUM - 1 are for ds_single
	unsigned long *hash_keys;
	int *list_index;

	volatile int activated;

	int memory_ipc_fAPP[MAX_USERAPP] = { 0 };
	int memory_ipc_fSERVICE[MAX_SERVICE] = { 0 };

	Component_state(Genode::Env &env,
		genode_uint64_t base, genode_uint64_t end, 
		genode_uint64_t free, genode_uint64_t used)
	: 
	env(env),
	address_base(base), 
	address_end(end), 
	address_free(free), 
	address_used(used),
	activated(1), // single-source services are activated by default
	memory_ipc_fAPP(),
	memory_ipc_fSERVICE()
	{ 	
		Genode::log("Memory server state initializing");
		ds_list = new (sliced_heap) Genode::Attached_ram_dataspace*[MAX_MEM_CAP];
		// fill the list with different levels of dataspace
		int k = 0;
		for (int i = 0; i < DS_SIZE_LEVELS; i++) {
			int j = (MEM_LEVEL_SIZE / DS_MIN_SIZE) >> i;
			for (int l = 0; l < j; l++) {
				ds_list[k++] = new (sliced_heap) Genode::Attached_ram_dataspace(
						env.ram(), env.rm(), (DS_MIN_SIZE << i));
			}
		}
		used_bitmaps = new (sliced_heap) int[MAX_MEM_CAP];
		for (int i = 0; i < MAX_MEM_CAP; i++) {
			used_bitmaps[i] = 0;
		}
		ds_single = new (sliced_heap) Genode::Attached_ram_dataspace*[SINGLE_DS_NUM];
		for (int i = 0; i < SINGLE_DS_NUM; i++) {
			ds_single[i] = 0;
		}
		used_bitmaps_single = new (sliced_heap) int[SINGLE_DS_NUM];
		for (int i = 0; i < SINGLE_DS_NUM; i++) {
			used_bitmaps_single[i] = 0;
		}
		hash_keys = new (sliced_heap) unsigned long[MEM_HASH_SIZE * MEM_HASH_CAPACITY];
		list_index = new (sliced_heap) int[MEM_HASH_SIZE * MEM_HASH_CAPACITY];
		for (int i = 0; i < MEM_HASH_SIZE * MEM_HASH_CAPACITY; i++) {
			hash_keys[i] = 0;
			list_index[i] = -1;
		}
		Genode::log("Memory server state initialized with ", k, " dataspace");
	}
};


struct MtsysMemory::Session_component : Genode::Rpc_object<Session>
{	
	int client_id;
	Component_state &state;

	int Transform_activation(int flag) override {
		Genode::log("[INFO] Transform activation flag: ", flag);
		int res = 0;
		int original;
		while (!res){
			original = state.activated;
			res = Genode::cmpxchg(&(state.activated), original, flag);
		} 
		return (!res);
	}

	int Memory_hello() override {
		if (state.activated == 0) {
			Genode::log("[INFO] Memory server not activated");
			return -1;
		}
		Genode::log("Hi, Mtsys Memory server for client ", client_id);
		return client_id;
	}

	genode_uint64_t query_free_space () override{
		if (state.activated == 0) {
			Genode::log("[INFO] Memory server not activated");
			return -1;
		}
		// change ipc stats 
		state.memory_ipc_fAPP[client_id]++;
		// a fake implementation now
		state.address_free -= 0x1000;
		return state.address_free;
	}

	Genode::Ram_dataspace_capability Memory_alloc(int size, Genode::addr_t &addr) override {
		if (state.activated == 0) {
			Genode::log("[INFO] Memory server not activated");
			return (state.ds_list[MAX_MEM_CAP - 1])->cap();
		}
		// change ipc stats 
		state.memory_ipc_fAPP[client_id]++;
		// allocate a dataspace and return it with addr
		int level = DS_SIZE2LEVEL(size);
		// Genode::log("Allocating dataspace size: ", size, " level: ", level);
		if (level < 0 || level >= DS_SIZE_LEVELS) {
			// use single dataspace
			int target_id = -1;
			int found = 0;
			for (int i = 0; i < SINGLE_DS_NUM; i++) {
				int idx = (state.head_idx_single + i) % SINGLE_DS_NUM;
				if (state.used_bitmaps_single[idx] == 0) {
					state.used_bitmaps_single[idx] = 1;
					state.ds_single[idx] = new (state.sliced_heap) Genode::Attached_ram_dataspace(
						state.env.ram(), state.env.rm(), size);
					addr = (Genode::addr_t)(state.ds_single[idx]->local_addr<void>());
					target_id = idx;
					found = 1;
					state.head_idx_single = (state.head_idx_single + 1) % SINGLE_DS_NUM;
					break;
				}
			}
			if (!found) {
				Genode::log("[[ERROR]]No more dataspace available for allocation");
				return (state.ds_list[MAX_MEM_CAP - 1])->cap();
			}
			// insert the addr into hash table
			unsigned long h = addr;
			int b = hash_bucket(h, MEM_HASH_SIZE);
			int success = 0;
			for (int i = 0; i < MEM_HASH_CAPACITY; i++) {
				if (state.hash_keys[b * MEM_HASH_CAPACITY + i] == 0) {
					state.hash_keys[b * MEM_HASH_CAPACITY + i] = h;
					state.list_index[b * MEM_HASH_CAPACITY + i] = target_id + 
											MEM_LEVEL_SIZE / DS_MIN_SIZE * 2;
					success = 1;
					break;
				}
			}
			if (!success) {
				Genode::log("[[ERROR]] Mem Hash table full");
				return (state.ds_list[MAX_MEM_CAP - 1])->cap();
			}
			return state.ds_single[target_id]->cap();
			// Genode::log("[[ERROR]]Size too large for dataspace allocation");
			// return (state.ds_list[MAX_MEM_CAP - 1])->cap();
		}
		int idx = start_idxof_level(level);
		int target_id = -1;
		int found = 0;
		for (int i = 0; i < (MEM_LEVEL_SIZE / DS_MIN_SIZE) >> level; i++) {
			if (state.used_bitmaps[idx + i] == 0) {
				state.used_bitmaps[idx + i] = 1;
				addr = (Genode::addr_t)(state.ds_list[idx + i]->local_addr<void>());
				target_id = idx + i;
				found = 1;
				state.head_idx[level] = (state.head_idx[level] + 1) 
								% ((MEM_LEVEL_SIZE / DS_MIN_SIZE) >> level);
				break;
			}
		}
		if (!found) {
			Genode::log("[[ERROR]]No more dataspace available for allocation");
			return (state.ds_list[MAX_MEM_CAP - 1])->cap();
		}
		// insert the addr into hash table
		unsigned long h = addr;
		int b = hash_bucket(h, MEM_HASH_SIZE);
		int success = 0;
		for (int i = 0; i < MEM_HASH_CAPACITY; i++) {
			if (state.hash_keys[b * MEM_HASH_CAPACITY + i] == 0) {
				state.hash_keys[b * MEM_HASH_CAPACITY + i] = h;
				state.list_index[b * MEM_HASH_CAPACITY + i] = target_id;
				success = 1;
				break;
			}
		}
		if (!success) {
			Genode::log("[[ERROR]] Mem Hash table full");
			return (state.ds_list[MAX_MEM_CAP - 1])->cap();
		}
		return state.ds_list[target_id]->cap();
	}

	int Memory_free(Genode::addr_t addr) override {
		if (state.activated == 0) {
			Genode::log("[INFO] Memory server not activated");
			return -1;
		}
		// change ipc stats 
		state.memory_ipc_fAPP[client_id]++;
		// free the dataspace
		unsigned long h = (unsigned long)addr;
		int b = hash_bucket(h, MEM_HASH_SIZE);
		int success = 0;
		for (int i = 0; i < MEM_HASH_CAPACITY; i++) {
			if (state.hash_keys[b * MEM_HASH_CAPACITY + i] == h) {
				int target_id = state.list_index[b * MEM_HASH_CAPACITY + i];
				if (target_id >= MEM_LEVEL_SIZE / DS_MIN_SIZE * 2) {
					// single dataspace
					state.used_bitmaps_single[target_id - MEM_LEVEL_SIZE / DS_MIN_SIZE * 2] = 0;
					state.env.ram().free(state.ds_single[target_id - MEM_LEVEL_SIZE / DS_MIN_SIZE * 2]->cap());
					state.sliced_heap.free(state.ds_single[target_id - MEM_LEVEL_SIZE / DS_MIN_SIZE * 2], 
						sizeof(Genode::Attached_ram_dataspace));
					state.ds_single[target_id - MEM_LEVEL_SIZE / DS_MIN_SIZE * 2] = 0;
				}
				else {
					state.used_bitmaps[target_id] = 0;
				}
				state.hash_keys[b * MEM_HASH_CAPACITY + i] = 0;
				state.list_index[b * MEM_HASH_CAPACITY + i] = -1;
				success = 1;
				break;
			}
		}
		if (!success) {
			Genode::log("[[ERROR]] Address not found in hash table");
			return -1;
		}
		return 0;
	}

	Session_component(int id, Component_state &s) 
	: client_id(id),
	state(s)
	{ }

	~Session_component() {
		Genode::log("Destroying MtsysMemory session for client ", client_id);
		// here we check the address 0x18000 manually, which should be p6
		// TODO: delete this line in performance test
		Genode::log("p6: ", (const char*)0x18000);
	}

};


class MtsysMemory::Root_component
:
	public Genode::Root_component<Session_component>
{	
	private:
		Genode::Env &env;
		Component_state stat;
		int client_used[MAX_USERAPP] = { 0 };
		int next_client_id = 0;
	protected:

		Session_component *_create_session(const char *) override
		{
			Genode::log("Creating MtsysMemory session");
			int new_client_id = -1;

			// find unused client slot
			for (int offset = 0; offset < MAX_USERAPP; offset++) {
				auto new_id = (next_client_id + offset) % MAX_USERAPP;
				if (client_used[new_id] == 0) {
					client_used[new_id] = 1;
					new_client_id = new_id;
					next_client_id = (new_id + 1) % MAX_USERAPP;
					break;
				}
			}

			if (new_client_id == -1) {
				Genode::log("[[ERROR]]No more clients can be created");	
				return nullptr;
			}
			return new (md_alloc()) Session_component(new_client_id, stat);
		}


		virtual void _destroy_session(Session_component* session) override {
			// we should free client id here
			auto& cid = session->client_id;

			if (cid < 0 || cid >= MAX_USERAPP || !client_used[cid]) {
				Genode::log("[Critical] _destroy_session: Bad client id: %d\n", cid);
				goto END;
			}

			client_used[cid] = 0;

END:
			// call super method
			Genode::Root_component<Session_component>::_destroy_session(session);
		}

	public:

		Root_component(Genode::Env &env,
					Genode::Entrypoint &ep,
					Genode::Allocator &alloc)
		:
			Genode::Root_component<Session_component>(ep, alloc),
			env(env),
			stat(env, 0x400000000, 0xfffffffff, 0xc00000000, 0),
			client_used()
		{
			Genode::log("Creating MtsysMemory root component");
		}
};


struct MtsysMemory::Main
{
	Genode::Env &env;

	/*
	 * A sliced heap is used for allocating session objects - thereby we
	 * can release objects separately.
	 */
	Genode::Sliced_heap sliced_heap { env.ram(), env.rm() };

	MtsysMemory::Root_component root {env, env.ep(), sliced_heap };

	Main(Genode::Env &env) : env(env)
	{
		/*
		 * Create a RPC object capability for the root interface and
		 * announce the service to our parent.
		 */
		env.parent().announce(env.ep().manage(root));
		Genode::log("MtsysMemory service is ready");
	}
};


void Component::construct(Genode::Env &env)
{
	static MtsysMemory::Main main(env);
	
}
