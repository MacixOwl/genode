/*
 * \brief  Main program of the Hello server
 * \author zhenlin
 * \date   2024-04-12
 */

/*
 * Copyright (C) 2008-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <base/component.h>
#include <base/log.h>
#include <base/heap.h>
#include <root/component.h>
#include <rpc_session/rpc_session.h>
#include <base/rpc_server.h>

namespace Pipeline {
	struct Session_component;
	struct Root_component;
	struct Main;
}


struct Pipeline::Session_component : Genode::Rpc_object<SessionC>
{
	void say_hello() override {
		Genode::log("I am here... Hello.");
    }

	int add(int a, int b) override {
		return a + b; 
    }

    int funcC0() override {
		return 0; 
    }
};


class Pipeline::Root_component
:
	public Genode::Root_component<Session_component>
{
	protected:

		Session_component *_create_session(const char *) override
		{
			Genode::log("creating session");
			return new (md_alloc()) Session_component();
		}

	public:

		Root_component(Genode::Entrypoint &ep,
		               Genode::Allocator &alloc)
		:
			Genode::Root_component<Session_component>(ep, alloc)
		{
			Genode::log("creating root component");
		}
};


struct Pipeline::Main
{
	Genode::Env &env;

	/*
	 * A sliced heap is used for allocating session objects - thereby we
	 * can release objects separately.
	 */
	Genode::Sliced_heap sliced_heap { env.ram(), env.rm() };

	Pipeline::Root_component root { env.ep(), sliced_heap };

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
	static Pipeline::Main main(env);
}
