
#include <base/component.h>
#include <base/log.h>
#include <base/heap.h>
#include <root/component.h>
#include <base/rpc_server.h>
#include <dataspace/capability.h>
#include <dataspace/client.h>
#include <base/attached_ram_dataspace.h>

#include <base/signal.h>

#include <pivot/pivot_session.h>
#include <kv/kv_session.h>
#include <memory/memory_connection.h>
#include <adl/collections/RedBlackTree.hpp>
#include <adl/collections/ArrayList.hpp>
#include <adl/Allocator.h>

// #include <adl/stdint.h>


namespace MtsysKv {
	struct Component_state;
	struct Session_component;
	struct Root_component;
	struct Main;
} 


struct MtsysKv::Component_state
{
	Genode::Ram_dataspace_capability ds_cap;

	genode_uint64_t *pDataVersion = nullptr;
	Genode::Attached_ram_dataspace dataVersion;

	int cid_in4service[MAX_SERVICE] = { 0 };
	int ipc_count[MAX_USERAPP] = { 0 };

	Genode::Env &env;
	MtsysMemory::Connection mem_obj;

	adl::RedBlackTree<KvRpcString, KvRpcString> rbtree;
	
	Component_state(Genode::Env &env, Genode::Allocator& alloc)
	: ds_cap(),
	cid_in4service(),
	ipc_count(),
	env(env),
	mem_obj(env),
	rbtree(),
	dataVersion(env.ram(), env.rm(), sizeof(*pDataVersion), Genode::UNCACHED)
	{	
		// get cids in services for later use, filled manually for now
		int cid_mem = mem_obj.Memory_hello();
		Genode::log("Memory service cid: ", cid_mem);
		cid_in4service[SID_MEMORY_SERVICE] = cid_mem;
		// fake implementation for now
		if (!ds_cap.valid())
        	ds_cap = env.ram().alloc(0x1000);
		pDataVersion = dataVersion.local_addr<genode_uint64_t>();
		*pDataVersion = 65472;
    }
};


struct MtsysKv::Session_component : Genode::Rpc_object<Session>
{	
	int client_id;
	Component_state &state;
	Genode::Env& env;
	Genode::Allocator* allocator;

	// this is a shared dataspace for range scan result
	// which is pinned to the client at first call
	Genode::Attached_ram_dataspace rangeScanRamDataspace;


	adl::ArrayList<KvRpcString> scanData;


	int Kv_hello() override {
		Genode::log("Hi, Mtsys Kv server for client ", client_id); 
        Genode::log("Kv init dataspace cap: ", state.ds_cap);
		return client_id;
	}

	MtsysKv::cid_4service get_cid_4services() override {
		Genode::log("Getting cids for services");
		MtsysKv::cid_4service cids;
		cids.cid_4memory = state.cid_in4service[SID_MEMORY_SERVICE];
		cids.cid_fake = 777;
		return cids;
	}

	void null_function() override {
		// we need update ipc count here
		state.ipc_count[client_id]++;
		return;
	}

	int get_IPC_stats(int client_id) override {
		// note that caller can get IPC stats for any client
		int count = state.ipc_count[client_id];
		state.ipc_count[client_id] = 0;
		// Genode::log("Getting IPC stats for client ", client_id, " count ", count);
		return count;
	}



	virtual int insert(const KvRpcString key, const KvRpcString value) override {
		state.ipc_count[client_id]++;
		state.rbtree.setData(key, value);
		(*state.pDataVersion) ++;
		return 0;
	}


	virtual int del(const KvRpcString key) override {

		state.ipc_count[client_id]++;
		if (state.rbtree.hasKey(key)) {
			state.rbtree.removeKey(key);
			(*state.pDataVersion) ++;
			return 0;
		} else {
			return 1;
		}
	}


	virtual const KvRpcString read(const KvRpcString key) override {
		state.ipc_count[client_id]++;
		return state.rbtree.hasKey(key) ? state.rbtree.getData(key) : KvRpcString {};
	}


	virtual int update(const KvRpcString key, const KvRpcString value) override {
		state.ipc_count[client_id]++;
		return insert(key, value);
	}

	virtual Genode::Ram_dataspace_capability range_scan_prepare() override {
		return rangeScanRamDataspace.cap();
	}

	virtual int range_scan(
		const KvRpcString leftBound, 
		const KvRpcString rightBound
	) override {

		state.ipc_count[client_id]++;
		const auto& collect = [] (void* untypedData, const KvRpcString& k, const KvRpcString& v) {

			auto& data = *reinterpret_cast< decltype(scanData)* >(untypedData);
			
			data.append(k);
			data.append(v);

			return true;
		};

		int count = state.rbtree.rangeScan(leftBound, rightBound, &scanData, collect);

		Genode::log("Range scan result: ", count, " elements");

		Genode::size_t dataSize = sizeof(MtsysKv::KvRpcString) * scanData.size();
		Genode::size_t dataPackSize = RPCDataPack::HEADER_SIZE + dataSize;
		// rangeScanRamDataspace.realloc(&env.ram(), dataPackSize);

		if (RANGE_SCAN_BUFFER < dataPackSize) {
			Genode::error("Failed to copy range scan result at once!");
			throw -1;  // TODO: should solve this with repeated copy
		}

		auto pDataPack = rangeScanRamDataspace.local_addr<RPCDataPack>();
		pDataPack->header.dataSize = dataSize;

		Genode::memcpy(pDataPack->data, scanData.data(), dataSize);

		scanData.clear();

		return 0;
	}


	virtual Genode::Ram_dataspace_capability get_data_version_addr() override {
		return state.dataVersion.cap();
	}


	Session_component(int id, Component_state &s, Genode::Allocator* allocator, Genode::Env& env) 
	: client_id(id),
		state(s),
		env(env),
		allocator(allocator),
		rangeScanRamDataspace(env.ram(), env.rm(), RANGE_SCAN_BUFFER),
		scanData()
	{
		
	}

};


class MtsysKv::Root_component
:
	public Genode::Root_component<Session_component>
{	
	private:
		Component_state stat;
		int client_used[MAX_USERAPP] = { 0 };
		int next_client_id = 0;
		Genode::Env& env;
		Genode::Heap heap;

	protected:

		Session_component *_create_session(const char *) override
		{
			Genode::log("Creating MtsysKv session");
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
			return new (md_alloc()) Session_component(new_client_id, stat, &heap, env);
		}



		virtual void _destroy_session(Session_component* session) override {
			// we should free client id here
			auto& cid = session->client_id;

			if (cid < 0 || cid >= MAX_USERAPP || !client_used[cid]) {
				Genode::log("[Critical] _destroy_session: Bad client id: %d\n", cid);
				goto END;
			}
			
			client_used[cid] = 0;
			Genode::log("Destroying MtsysKv session for client ", cid);

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
			stat(env, alloc),
			env(env),
			client_used(),
			heap(env.ram(), env.rm())
		{
			Genode::log("Creating MtsysKv root component");
		}
};


struct MtsysKv::Main
{
	Genode::Env &env;

	/*
	 * A sliced heap is used for allocating session objects - thereby we
	 * can release objects separately.
	 */
	Genode::Sliced_heap sliced_heap { env.ram(), env.rm() };

	MtsysKv::Root_component root { env.ep(), sliced_heap, env };

	Main(Genode::Env &env) 
    : 
    env(env)
	{

		adl::defaultAllocator.init({

			.alloc = [] (adl::size_t size, void* data) {
				return reinterpret_cast<Genode::Sliced_heap*>(data)->alloc(size);
			},
			
			.free = [] (void* addr, adl::size_t size, void* data) {
				reinterpret_cast<Genode::Sliced_heap*>(data)->free(addr, size);
			},
			
			.data = &sliced_heap
		});

		/*
		 * Create a RPC object capability for the root interface and
		 * announce the service to our parent.
		 */
		env.parent().announce(env.ep().manage(root));
        Genode::log("MtsysKv service is ready");
	}
};


void Component::construct(Genode::Env &env)
{
	static MtsysKv::Main main(env);

}
