/*
 * \brief  Client-side interface of the RPC service
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

#ifndef _INCLUDE__RPCPLUS_SESSION_H__CLIENT_H_
#define _INCLUDE__RPCPLUS_SESSION_H__CLIENT_H_

#include <rpcplus_session/rpcplus_session.h>
#include <base/rpc_client.h>
#include <base/log.h>
#include <base/session_object.h>
#include <base/attached_ram_dataspace.h>
#include <dataspace/capability.h>
#include <base/rpc_server.h>
#include <dataspace/client.h>


namespace RPCplus { struct Session_client; }


struct RPCplus::Session_client : Genode::Rpc_client<Session>
{	
	int memcap_init;
	Genode::Ram_dataspace_capability memcap;

	Session_client(Genode::Capability<Session> cap)
	: 
	Genode::Rpc_client<Session>(cap),
	memcap_init(0),
	memcap()
	{ }

	void say_hello() override
	{
		Genode::log("issue RPC for saying hello");
		call<Rpc_say_hello>();
		Genode::log("returned from 'say_hello' RPC call");
	}

	int add(int a, int b) override
	{
		return call<Rpc_add>(a, b);
	}

	Genode::Ram_dataspace_capability dataspace() override
	{
		return call<Rpc_dataspace>();
	}

	int send2server() override
	{
		return call<Rpc_send2server>();
	}

	Genode::Ram_dataspace_capability memorycap() override
	{
		return call<Rpc_memorycap>();
	}

	RPCplus::AllocRet allocate(genode_uint64_t lens) override
	{
		return call<Rpc_allocate>(lens);
	}

	RPCplus::AllocRet amkos_alloc(Genode::Env &env, genode_uint64_t lens){
		if (!memcap_init){
			memcap = call<Rpc_memorycap>();
			memcap_init = 1;
			Genode::log("main mem cap in client ", memcap);
			env.rm().attach_at(memcap, ALLOC_BASE);
			Genode::log("amkos_alloc init success");
		}
		RPCplus::AllocRet ret = call<Rpc_allocate>(lens);
		ret.addr += ALLOC_BASE;
		return ret;
	}

	// don't call this explicitly
	int free(RPCplus::AllocRet ret) override
	{
		int id = (int)ret.id;
		return id;
	}

	int amkos_free(RPCplus::AllocRet ret){
		ret.addr -= ALLOC_BASE;
		return call<Rpc_free>(ret);
	}

	Genode::Ram_dataspace_capability capfrom_pagelist(int i) override 
	{
		return call<Rpc_capfrom_pagelist>(i);
	}

};

#endif /* _INCLUDE__RPCPLUS_SESSION_H__CLIENT_H_ */
