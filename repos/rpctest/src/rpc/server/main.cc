/*
 * \brief  Main program of the RPC server
 * \author Björn Döbel
 * \author Minyi
 * \author Zhenlin
 * \date   2023-09
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
#include <rpcplus_session/rpcplus_session.h>
#include <base/rpc_server.h>
#include <base/attached_ram_dataspace.h>
#include <base/session_object.h>
#include <dataspace/capability.h>
#include <dataspace/client.h>
#include <timer_session/connection.h>


const int PAGE_SIZE = 4096;
const int RPC_BUFFER_LEN = PAGE_SIZE * 16; // 64KB
const int MAINMEM_STORE_LEN = PAGE_SIZE * 1024 * 25; // 100 MB

const int MAINMEM_ALLOC_SLOT = MAINMEM_STORE_LEN / PAGE_SIZE;

const int PRE_ALLOC_PAGENUM = 100;


namespace RPCplus {
	struct Session_component;
	struct Root_component;
	struct Main;
}


struct RPCplus::Session_component : Genode::Rpc_object<Session>
{
	//服务端创建一块ram dataspace，用于RPC消息
	Genode::Attached_ram_dataspace rpc_buffer;
	Genode::Attached_ram_dataspace &storage;
	Genode::Attached_ram_dataspace **pagelist;

	genode_uint8_t alloc_slot[MAINMEM_ALLOC_SLOT];
	genode_uint64_t alloc_id;

	Session_component(Genode::Env &env, 
					Genode::Attached_ram_dataspace &main_store,
					Genode::Attached_ram_dataspace **pagelist)
	:	rpc_buffer(env.ram(), env.rm(), RPC_BUFFER_LEN),
		storage(main_store),
		pagelist(pagelist),
		alloc_id(0)
	{	
		void* storage_ptr = storage.local_addr<int>();
		Genode::log("Server local storage addr: ", storage_ptr);
		int* p = rpc_buffer.local_addr<int>();
		p[0] = 114514;
		Genode::log("RPC buffer init.");

		for (int i = 0; i < MAINMEM_ALLOC_SLOT; ++i)
		{
			alloc_slot[i] = 0;
		}
		Genode::log("alloc slot array init as ", alloc_slot[MAINMEM_ALLOC_SLOT-1]);

		// for (int i = 0; i < PRE_ALLOC_PAGENUM; ++i) {
		// 	Genode::log("pagelist [", i, "] at addr: ", pagelist[i]);
		// 	Genode::log(pagelist[i]->cap());
		// }
	}

	void say_hello() override {
		Genode::log("I am here... Hello."); 
	}

	int add(int a, int b) override {
		return a + b; 
	}

	//把这块dataspace的cap返回给客户端
	Genode::Ram_dataspace_capability dataspace() override {
		Genode::log("cap in server is ", rpc_buffer.cap());
		return rpc_buffer.cap();
	}

	int send2server() override {
		int* p = rpc_buffer.local_addr<int>();
		int* storage_int = storage.local_addr<int>();
		Genode::log("message init for mem_storage: ", storage_int[0]);
		storage_int[0] = p[p[0]];
		Genode::log("message stored to mem_storage: ", storage_int[0]);
		return 0;
	}

	Genode::Ram_dataspace_capability memorycap() override {
		Genode::log("memory cap in server is ", storage.cap());
		return storage.cap();
	}

	RPCplus::AllocRet allocate(genode_uint64_t lens) override {
		genode_uint64_t p = 0;
		int page_ptr;
		for (page_ptr = 0; page_ptr < MAINMEM_ALLOC_SLOT; ++page_ptr){
			if (alloc_slot[page_ptr] == 0) {
				Genode::log("server memory allocate find slot ", page_ptr);
				alloc_slot[page_ptr] = 1;
				break;
			}
		}
		p = (genode_uint64_t)page_ptr * PAGE_SIZE;
		alloc_id += 1;
		RPCplus::AllocRet ret{p, lens, alloc_id};
		return ret;
	}

	int free(RPCplus::AllocRet ret) override{
		int page_ptr = (int)(ret.addr / PAGE_SIZE);
		int page_cnt = (int)(ret.lent / PAGE_SIZE);
		int page_tail = (int)(ret.lent % PAGE_SIZE);
		if (page_tail > 0) page_cnt += 1;
		Genode::log("memory free at ", page_ptr, ", count ",  page_cnt);
		for (int i = 0; i < page_cnt; ++i){
			alloc_slot[page_ptr + i] = 0;
		}
		return 0;
	}

	Genode::Ram_dataspace_capability capfrom_pagelist(int i) override {
		Genode::log("page_cap in server is ", pagelist[i]->cap());
		return pagelist[i]->cap();
	}

};


class RPCplus::Root_component
:
	public Genode::Root_component<Session_component>
	
{
	private:
		//加个env
		Genode::Env &_env;

		Genode::Heap heap{_env.ram(), _env.rm()};
		Genode::Attached_ram_dataspace* pagelist[PRE_ALLOC_PAGENUM];
	
	protected:

		Session_component *_create_session(const char *) override
		{
			Genode::log("creating rpcplus session");
			//把env作为参数传给Session_component，Session_component才能创建Attached_ram_dataspace
			//we also add the main memory store space to the session
			return new (md_alloc()) Session_component(_env, _main_store, pagelist);
		}
	
	public:

		//handle the main memory dataspace
		Genode::Attached_ram_dataspace &_main_store;

		Root_component(Genode::Env &env, Genode::Attached_ram_dataspace &main_store,
					Genode::Entrypoint &ep, Genode::Allocator &alloc)
		:	Genode::Root_component<Session_component>(ep, alloc), 
			_env(env), //这里把env传进去
			_main_store(main_store)
		{
			Genode::log("creating root component");

			for (int i = 0; i < PRE_ALLOC_PAGENUM; ++i){
				pagelist[i] = new(heap)Genode::Attached_ram_dataspace( _env.ram(), _env.rm(), 4096);
			}
			for (int i = 0; i < PRE_ALLOC_PAGENUM; ++i) {
				Genode::log("pagelist [", i, "] at addr: ", pagelist[i]);
				Genode::log(pagelist[i]->cap());
			}
		}
};


struct RPCplus::Main
{
	Genode::Env &env;
	Timer::Connection &timer;

	Genode::Attached_ram_dataspace mem_store{ env.ram(), env.rm(), MAINMEM_STORE_LEN };

	/*
	 * A sliced heap is used for allocating session objects - thereby we
	 * can release objects separately.
	 */
	Genode::Sliced_heap sliced_heap { env.ram(), env.rm() };
	RPCplus::Root_component root { env, mem_store, env.ep(), sliced_heap };

	Main(Genode::Env &env, Timer::Connection &timer) 
	:	env(env), timer(timer)
	{
		Genode::log("Time mem allocation ", timer.curr_time().trunc_to_plain_ms());
		/*
		 * Create a RPC object capability for the root interface and
		 * announce the service to our parent.
		 */
		env.parent().announce(env.ep().manage(root));
	}
};


void Component::construct(Genode::Env &env)
{	
	Timer::Connection _timer(env);
	static RPCplus::Main main(env, _timer);

}
