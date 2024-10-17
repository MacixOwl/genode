
#include <base/component.h>
#include <base/log.h>
#include <base/heap.h>
#include <root/component.h>
#include <base/rpc_server.h>
#include <dataspace/capability.h>
#include <dataspace/client.h>
#include <base/attached_ram_dataspace.h>

#include <kv/kv_session.h>
#include <memory/memory_connection.h>

namespace MtsysKv {
	struct Component_state;
	struct Session_component;
	struct Root_component;
	struct Main;
}


struct MtsysKv::Component_state
{
	Genode::Ram_dataspace_capability ds_cap;

	Component_state(Genode::Env &env)
	: ds_cap()
	{
        ds_cap = env.ram().alloc(0x1000);
    }
};


struct MtsysKv::Session_component : Genode::Rpc_object<Session>
{	
	int client_id;
	Component_state &state;

	void Kv_hello() override {
		Genode::log("Hi, Mtsys Kv server for client ", client_id); 
        Genode::log("Kv init dataspace cap: ", state.ds_cap);
	}

	Session_component(int id, Component_state &s) 
	: client_id(id),
	state(s)
	{ }

};


class MtsysKv::Root_component
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
			Genode::log("Creating MtsysKv session");
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
		               Genode::Allocator &alloc, 
                       Genode::Env &env)
		:
			Genode::Root_component<Session_component>(ep, alloc),
			stat(env),
			client_used()
		{
			Genode::log("Creating MtsysKv root component");
			for (int i = 0; i < max_client; i++) {
				client_used[i] = 0;
			}
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

    MtsysMemory::Connection mem_obj(env);
    mem_obj.Memory_hello();
}
