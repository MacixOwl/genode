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
#include <rpc_session/connection.h>
#include <cpu/atomic.h>

#include <timer_session/connection.h>

namespace Pipeline {
	struct Session_component;
	struct Root_component;
	struct Main;
}

volatile int queued_callnum = 0;
volatile int callnum_count = 0;
const int qbuffer_cpcty = 1 << 8;

struct Pipeline_Thread : Genode::Thread
{
	int _thread_id;
	Pipeline::Connection2C call2C;

	Pipeline_Thread(Genode::Env &env, int thread_id)
	:	Genode::Thread(env, "Pipeline_Thread", 16*1024),
		_thread_id(thread_id),
		call2C(env)
	{
		start(); 
	}

	void entry() override
	{
		Genode::log("thread ", _thread_id);
		int tmp;
		int res;
		while (true){
			if (queued_callnum > 0) {
				// Genode::log("queued call to funcC0");
				call2C.funcC0();
				res = 0;
				while (!res){
					tmp = queued_callnum - 1;
					res = Genode::cmpxchg(&queued_callnum, tmp + 1, tmp);
				}
				tmp = callnum_count + 1;
				callnum_count = tmp;
			}
		}
	}
};


struct Pipeline::Session_component : Genode::Rpc_object<SessionB>
{	
	Pipeline::Connection2C call2C;
	Pipeline_Thread *call_worker;
	Genode::Heap heap;
	Timer::Connection timer;

	Session_component(Genode::Env &env)
	:	call2C(env),
		call_worker(nullptr),
		heap(env.ram(), env.rm()),
		timer(env)
	{ 
		call_worker = new (heap) Pipeline_Thread(env, 17);
		Genode::log("Session component created");
		timer.msleep(3000);
		Genode::log("Session component created #2");
		timer.msleep(6000);
		Genode::log("Session component created #3");
	}

	void say_hello() override {
		Genode::log("I am here... Hello."); 
		call2C.say_hello(); 
		int const sum = call2C.add(2, 5);
		Genode::log("added 2 + 5 = ", sum);
	}

	int add(int a, int b) override {
		return a + b;
	}

	int funcB0() override {
		return 0;
	}

	int funcC0() override {
		return call2C.funcC0();
	}

	int funcC0_usync() override {
		int res = 0;
		int tmp;
		while (!res){
			tmp = queued_callnum + 1;
			res = Genode::cmpxchg(&queued_callnum, tmp - 1, tmp);
		}
		if (queued_callnum >= qbuffer_cpcty * 2){
			// Genode::log("queue full"); 
			while(queued_callnum > qbuffer_cpcty / 2){}
		}
		return 0;
	}

	Genode::uint64_t getcallnum() override {
		int tmp;
		while(queued_callnum > 0) {
			tmp = queued_callnum;
			if (tmp == 0) break;
		}
		return callnum_count;
	}

};


class Pipeline::Root_component
:
	public Genode::Root_component<Session_component>
{
	private:
		Genode::Env &env;
		Timer::Connection timer;
	
	protected:

		Session_component *_create_session(const char *) override
		{
			Genode::log("creating session");
			return new (md_alloc()) Session_component(env);
		}

	public:

		Root_component(Genode::Env &env,
					Genode::Entrypoint &ep,
		            Genode::Allocator &alloc) 
		:	Genode::Root_component<Session_component>(ep, alloc),
			env(env),
			timer(env)
		{
			Genode::log("creating root component");
			timer.msleep(3000);
			Genode::log("creating root component #2");
			timer.msleep(6000);
			Genode::log("creating root component #3");
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

	Pipeline::Root_component root {env, env.ep(), sliced_heap };

	Main(Genode::Env &env) : env(env) 
	{
		/*
		 * Create a RPC object capability for the root interface and
		 * announce the service to our parent.
		 */
		env.parent().announce(env.ep().manage(root));
		Timer::Connection timer(env);
		Genode::log("Start from pipeline main");
		timer.msleep(3000);
		Genode::log("Start from pipeline main #2");
		timer.msleep(6000);
		Genode::log("Start from pipeline main #3");
	}
};


void Component::construct(Genode::Env &env)
{
	static Pipeline::Main main(env);
	Timer::Connection timer(env);
	Genode::log("Start from pipeline component");
	timer.msleep(3000);
	Genode::log("Start from pipeline component #2");
	timer.msleep(6000);
	Genode::log("Start from pipeline component #3");
}
