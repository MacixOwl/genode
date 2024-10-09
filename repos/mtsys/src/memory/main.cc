
#include <base/component.h>
#include <base/log.h>
#include <base/heap.h>
#include <root/component.h>
#include <base/rpc_server.h>

#include <memory/memory_session.h>

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

	Component_state(genode_uint64_t base, genode_uint64_t end, 
		genode_uint64_t free, genode_uint64_t used)
	: address_base(base), 
	address_end(end), 
	address_free(free), 
	address_used(used)
	{ }
};


struct MtsysMemory::Session_component : Genode::Rpc_object<Session>
{	
	int client_id;
	Component_state &state;

	void Memory_hello() override {
		Genode::log("Hi, Mtsys Memory server for client ", client_id); 
	}

	genode_uint64_t query_free_space () override{
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
		const static int max_client = 64;
		Component_state stat;
		int client_used[max_client];
	protected:

		Session_component *_create_session(const char *) override
		{
			Genode::log("Creating MtsysMemory session");
			int new_client_id = -1;
			for (int i = 0; i < max_client; i++) {
				if (client_used[i] == 0) {
					client_used[i] = 1;
					new_client_id = i;
					break;
				}
			}
			if (new_client_id == -1) {
				Genode::log("[[ERROR]]No more clients can be created");	
			}
			return new (md_alloc()) Session_component(new_client_id, stat);
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
			for (int i = 0; i < max_client; i++) {
				client_used[i] = 0;
			}
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
	}
};


void Component::construct(Genode::Env &env)
{
	static MtsysMemory::Main main(env);
}
