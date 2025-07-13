/*
 * \brief  Test client for our RPC interface
 * \author Björn Döbel
 * \author Norman Feske
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
#include <rpcplus_session/connection.h>
#include <base/attached_ram_dataspace.h>
#include <timer_session/connection.h>


void Component::construct(Genode::Env &env)
{
	RPCplus::Connection rpc(env);
	Timer::Connection _timer(env);

	Genode::Milliseconds time_beg = _timer.curr_time().trunc_to_plain_ms();
	Genode::log("Client test start at ", time_beg);

	rpc.say_hello();

	int const sum = rpc.add(2, 5);
	Genode::log("added 2 + 5 = ", sum);

	//获取服务端dataspace
	Genode::Ram_dataspace_capability dscap = rpc.dataspace();
	Genode::log("cap in client is ", dscap);
	//指定dataspace的虚拟内存地址q
	//Now a fixed high addr, free of conflict
	Genode::addr_t q = 0x60000000;
	//绑定
	env.rm().attach_at(dscap, q);
	//addr_t没法直接用，类型转换一下
	int* qq = (int*) q;
	//qq对应的物理地址就是服务端dataspace的地址
	Genode::log("get rpc buffer head ", qq[0]);
	//Write some reply to the server
	qq[0] = 19;
	qq[19] = 1919810;
	rpc.send2server();


	char* contents;

	RPCplus::AllocRet mem_0 = rpc.amkos_alloc(env, 4096);
	Genode::log("memory allocated at: ", mem_0.addr, " +", mem_0.lent);
	contents = (char*)mem_0.addr;
	Genode::log("contents[0] is ", contents[0]);
	contents[0] = 'q';
	Genode::log("contents[0] is ", contents[0]);

	RPCplus::AllocRet mem_1 = rpc.amkos_alloc(env, 4096);
	Genode::log("memory allocated at: ", mem_1.addr, " +", mem_1.lent);
	contents = (char*)mem_1.addr;
	Genode::log("contents[0] is ", contents[0]);
	contents[0] = 'q';
	Genode::log("contents[0] is ", contents[0]);

	rpc.amkos_free(mem_0);

	RPCplus::AllocRet mem_2 = rpc.amkos_alloc(env, 4096);
	Genode::log("memory allocated at: ", mem_2.addr, " +", mem_2.lent);
	contents = (char*)mem_2.addr;
	Genode::log("contents[0] is ", contents[0]);
	contents[0] = 'q';
	Genode::log("contents[0] is ", contents[0]);

	Genode::Ram_dataspace_capability page_0 = rpc.capfrom_pagelist(0);
	Genode::log("page_cap in client is ", page_0);
	Genode::Ram_dataspace_capability page_1 = rpc.capfrom_pagelist(1);
	Genode::addr_t pages = 0x66000000;
	env.rm().attach_at(page_0, pages);
	env.rm().attach_at(page_1, pages + 4096);
	int* pageint = (int*) pages;
	pageint[1] = 7;
	pageint[2000] = 7;
	Genode::log("page contents: ", pageint[0], " ", pageint[1], " ", pageint[2000]);
	pageint[2047] = 7;


	Genode::Milliseconds time_end = _timer.curr_time().trunc_to_plain_ms();
	Genode::log("Client test end at ", time_end);

	Genode::log("rpc test completed");
}
