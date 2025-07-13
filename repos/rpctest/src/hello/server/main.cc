/*
 * \brief  Main program of the Hello server
 * \author Björn Döbel
 * \author Norman Feske
 * \date   2008-03-20
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
#include <hello_session/hello_session.h>
#include <base/rpc_server.h>
#include <base/attached_ram_dataspace.h>
#include <base/session_object.h>
#include <dataspace/capability.h>
#include <base/rpc_server.h>
#include <dataspace/client.h>

namespace Hello {
	struct Session_component;
	struct Root_component;
	struct Main;
}


struct Hello::Session_component : Genode::Rpc_object<Session>
{
	//服务端创建一块ram dataspace
	Genode::Attached_ram_dataspace _ds;
	Session_component(Genode::Env &env):_ds(env.ram(), env.rm(), sizeof(int)*100){
		int* p = _ds.local_addr<int>();
		p[0] = 114514;
	}
	void say_hello() override {
		Genode::log("I am here... Hello."); }

	int add(int a, int b) override {
		return a + b; }
	//把这块dataspace的cap返回给客户端
	Genode::Ram_dataspace_capability dataspace() override {
		Genode::log("cap in server is ", _ds.cap());
		return _ds.cap();
	}
};


class Hello::Root_component
:
	public Genode::Root_component<Session_component>
	
{
	private:
		//加个env
		Genode::Env &_env;
	protected:

		Session_component *_create_session(const char *) override
		{
			Genode::log("creating hello session");
			//把env作为参数传给Session_component，Session_component才能创建Attached_ram_dataspace
			return new (md_alloc()) Session_component(_env);
		}
	public:
	

		Root_component(Genode::Env &env, Genode::Entrypoint &ep,
		               Genode::Allocator &alloc)
		:	 Genode::Root_component<Session_component>(ep, alloc), _env(env) //这里把env传进去
		{
			Genode::log("creating root component");
		}
};


struct Hello::Main
{
	Genode::Env &env;

	/*
	 * A sliced heap is used for allocating session objects - thereby we
	 * can release objects separately.
	 */
	Genode::Sliced_heap sliced_heap { env.ram(), env.rm() };
	Hello::Root_component root { env, env.ep(), sliced_heap };

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
	static Hello::Main main(env);
}
