/*
 * \brief  Test client for the Hello RPC interface
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
#include <hello_session/connection.h>
#include <base/attached_ram_dataspace.h>


void Component::construct(Genode::Env &env)
{
	Hello::Connection hello(env);

	hello.say_hello();

	int const sum = hello.add(2, 5);
	Genode::log("added 2 + 5 = ", sum);
	//获取服务端dataspace
	Genode::Ram_dataspace_capability dscap = hello.dataspace();
	Genode::log("cap in client is ", dscap);
	//指定dataspace的虚拟内存地址q
	Genode::addr_t q = 0x60000000;
	//绑定
	env.rm().attach_at(dscap, q);
	//addr_t没法直接用，类型转换一下
	int* qq = (int*) q;
	//qq对应的物理地址就是服务端dataspace的地址
	Genode::log(qq[0]);

	Genode::log("hello test completed");
}
