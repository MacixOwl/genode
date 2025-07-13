/*
 * \brief  Test client for the Hello RPC interface
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
#include <rpc_session/connection.h>
#include <timer_session/connection.h>

const int LOOP_NUM = 1 << 17;


void Component::construct(Genode::Env &env)
{
	Timer::Connection _timer(env);

	Pipeline::Connection2B call2B(env);
	call2B.say_hello();
	int const sum = call2B.add(2, 5);
	Genode::log("added 2 + 5 = ", sum);

	Genode::log("pipeline test started");

	Genode::log("pipeline test: funcB0");
	Genode::Microseconds time_0 = _timer.curr_time().trunc_to_plain_us();
	Genode::log("funcB0 start at ", time_0);
	for (int i = 0; i < LOOP_NUM; ++i){
		call2B.funcB0();
	}
	Genode::Microseconds time_1 = _timer.curr_time().trunc_to_plain_us();
	Genode::log("funcB0 end at ", time_1);
	long time01 =  time_1.value - time_0.value;
	float thrput01 = (float)LOOP_NUM / (float)time01 * 1000000;
	Genode::log("funcB0 throughput is ", thrput01, "/second");

	Genode::log("pipeline test: funcC0");
	Genode::Microseconds time_2 = _timer.curr_time().trunc_to_plain_us();
	Genode::log("funcC0 start at ", time_2);
	for (int i = 0; i < LOOP_NUM; ++i){
		call2B.funcC0();
	}
	Genode::Microseconds time_3 = _timer.curr_time().trunc_to_plain_us();
	Genode::log("funcC0 end at ", time_3);
	long time23 =  time_3.value - time_2.value;
	float thrput23 = (float)LOOP_NUM / (float)time23 * 1000000;
	Genode::log("funcC0 throughput is ", thrput23, "/second");

	Genode::log("pipeline test: funcC0_usync");
	Genode::Microseconds time_4 = _timer.curr_time().trunc_to_plain_us();
	Genode::log("funcC0_usync start at ", time_4);
	for (int i = 0; i < LOOP_NUM; ++i){
		call2B.funcC0_usync();
	}
	Genode::log(call2B.getcallnum()); // actually draining
	Genode::Microseconds time_5 = _timer.curr_time().trunc_to_plain_us();
	Genode::log("funcC0_usync end at ", time_5);
	long time45 =  time_5.value - time_4.value;
	float thrput45 = (float)LOOP_NUM / (float)time45 * 1000000;
	Genode::log("funcC0_usync throughput is ", thrput45, "/second");

	Genode::log("pipeline test completed");
}
