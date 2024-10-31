
#include <base/component.h>
#include <base/log.h>
#include <base/heap.h>
#include <root/component.h>
#include <base/rpc_server.h>

#include <pivot/pivot_session.h>
#include <memory/memory_session.h>
#include <kv/kv_connection.h>

namespace MtsysMemory {
	struct Component_state;
	struct Session_component;
	struct Root_component;
	struct Main;
}


struct MtsysMemory::Component_state
{
	genode_uint64_t address_base;
	genode_uint64_t address_end;
	genode_uint64_t address_free;
	genode_uint64_t address_used;

	int memory_ipc_fAPP[MAX_USERAPP] = { 0 };
	int memory_ipc_fSERVICE[MAX_SERVICE] = { 0 };

	Component_state(genode_uint64_t base, genode_uint64_t end, 
		genode_uint64_t free, genode_uint64_t used)
	: address_base(base), 
	address_end(end), 
	address_free(free), 
	address_used(used),
	memory_ipc_fAPP(),
	memory_ipc_fSERVICE()
	{ }
};


struct MtsysMemory::Session_component : Genode::Rpc_object<Session>
{	
	int client_id;
	Component_state &state;

	int Memory_hello() override {
		Genode::log("Hi, Mtsys Memory server for client ", client_id);
		return client_id;
	}

	genode_uint64_t query_free_space () override{
		// change ipc stats 
		state.memory_ipc_fAPP[client_id]++;
		// a fake implementation now
		state.address_free -= 0x1000;
		return state.address_free;
	}

	Session_component(int id, Component_state &s) 
	: client_id(id),
	state(s)
	{ }

};


class MtsysMemory::Root_component
:
	public Genode::Root_component<Session_component>
{	
	private:
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

			if (cid < 0 || cid >= MAX_CLIENT || !client_used[cid]) {
				Genode::log("[Critical] _destroy_session: Bad client id: %d\n", cid);
				goto END;
			}

			client_used[cid] = 0;

END:
			// call super method
			Genode::Root_component<Session_component>::_destroy_session(session);
		}

	public:

		Root_component(Genode::Entrypoint &ep,
		               Genode::Allocator &alloc)
		:
			Genode::Root_component<Session_component>(ep, alloc),
			stat(0x400000000, 0xfffffffff, 0xc00000000, 0),
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

	MtsysMemory::Root_component root { env.ep(), sliced_heap };

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
