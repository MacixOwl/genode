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


const int LOOP_NUM = 1 << 17;
const int CLIENT_ID = 3;


void Component::construct(Genode::Env &env)
{
	RPCplus::Connection rpc(env);
	Timer::Connection _timer(env);

	Genode::Milliseconds time_beg = _timer.curr_time().trunc_to_plain_ms();
	Genode::log("Client test start at ", time_beg);

	rpc.pure_function();
	Genode::Milliseconds time_0 = _timer.curr_time().trunc_to_plain_ms();
	Genode::log("pure_function start at ", time_0);
	for (int i = 0; i < LOOP_NUM; ++i){
		rpc.pure_function();
	}
	Genode::Milliseconds time_1 = _timer.curr_time().trunc_to_plain_ms();
	Genode::log("pure_function end at ", time_1);
	long time01 =  time_1.value - time_0.value;
	float thrput01 = (float)LOOP_NUM / (float)time01 * 1000;
	Genode::log("pure_function throughput is ", thrput01, "/second");

	int a = 114;
	int b = 514;
	int s = rpc.add_int(a, b);
	Genode::log(s);
	Genode::Milliseconds time_2 = _timer.curr_time().trunc_to_plain_ms();
	Genode::log("add_int start at ", time_2);
	for (int i = 0; i < LOOP_NUM; ++i){
		s = rpc.add_int(a, b);
	}
	Genode::Milliseconds time_3 = _timer.curr_time().trunc_to_plain_ms();
	Genode::log("add_int end at ", time_3);
	long time23 =  time_3.value - time_2.value;
	float thrput23 = (float)LOOP_NUM / (float)time23 * 1000;
	Genode::log("add_int throughput is ", thrput23, "/second");

	s = rpc.set_storehead(CLIENT_ID, 0);
	Genode::Milliseconds time_4 = _timer.curr_time().trunc_to_plain_ms();
	Genode::log("set_storehead start at ", time_4);
	for (int i = 0; i < LOOP_NUM; ++i){
		s = rpc.set_storehead(CLIENT_ID, i);
	}
	Genode::Milliseconds time_5 = _timer.curr_time().trunc_to_plain_ms();
	Genode::log("set_storehead end at ", time_5);
	long time45 =  time_5.value - time_4.value;
	float thrput45 = (float)LOOP_NUM / (float)time45 * 1000;
	Genode::log("set_storehead throughput is ", thrput45, "/second");

	Genode::Ram_dataspace_capability bufcap = rpc.get_RPCbuffer(CLIENT_ID);
	env.rm().attach_at(bufcap, 0x60000000);
	int* buf = (int*)0x60000000;
	rpc.get_storehead(CLIENT_ID, 0);
	Genode::log("get rpc buffer head ", buf[0]);
	Genode::Milliseconds time_6 = _timer.curr_time().trunc_to_plain_ms();
	Genode::log("get_storehead start at ", time_6);
	for (int i = 0; i < LOOP_NUM; ++i){
		rpc.get_storehead(CLIENT_ID, i);
		s = buf[0];
	}
	Genode::Milliseconds time_7 = _timer.curr_time().trunc_to_plain_ms();
	Genode::log("get_storehead end at ", time_7);
	long time67 =  time_7.value - time_6.value;
	float thrput67 = (float)LOOP_NUM / (float)time67 * 1000;
	Genode::log("get_storehead throughput is ", thrput67, "/second");

	Genode::Milliseconds time_end = _timer.curr_time().trunc_to_plain_ms();
	Genode::log("Client test end at ", time_end);

	Genode::log("benchmark completed");
}
