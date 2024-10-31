
#include <base/component.h>
#include <base/log.h>
#include <base/heap.h>
#include <root/component.h>
#include <base/rpc_server.h>
#include <dataspace/capability.h>
#include <dataspace/client.h>
#include <base/attached_ram_dataspace.h>
#include <timer_session/connection.h>

#include <pivot/pivot_session.h>

namespace MtsysPivot {
	struct Component_state;
	struct Session_component;
	struct Root_component;
	struct Main;
}


struct MtsysPivot::Component_state
{
    int pivot_appid_used[MAX_USERAPP] = { 0 };
    int pivot_ipc_app2comp[MAX_USERAPP][MAX_COMPSVC] = { 0 };
    int pivot_ipc_service2service[MAX_SERVICE][MAX_SERVICE] = { 0 };
    int lockstate;

	Genode::Env &env;
	Timer::Connection timer;

	void update_ipc_stats(Genode::Duration) {
		Genode::log("Updating IPC stats");
	}

	Timer::Periodic_timeout<MtsysPivot::Component_state> timeout;

	Component_state(Genode::Env &env)
	: lockstate(0),
	env(env),
	timer(env),
	timeout(timer, *this, &Component_state::update_ipc_stats, Genode::Microseconds{500 * 1000})
    {
        
    }
};


struct MtsysPivot::Session_component : Genode::Rpc_object<Session>
{	
	int client_id;
	Component_state &state;

	void Pivot_hello() override {
		Genode::log("Hi, Mtsys Pivot for client ", client_id); 
        Genode::log("Pivot init lockstate: ", state.lockstate);
	}

    int Pivot_App_getid() override {
        return client_id;
    }

    void Pivot_IPC_stats(int appid, int num) override {
        Genode::log("Pivot IPC stats: appid ", appid, " num ", num);
        state.pivot_ipc_app2comp[appid][num]++; // increment the IPC count
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
            stat.pivot_appid_used[new_client_id] = 1;
			if (new_client_id == -1) {
				Genode::log("[[ERROR]]No more apps can be created");	
			}
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
