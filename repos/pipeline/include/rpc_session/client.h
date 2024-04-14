/*
 * \brief  Client-side interface of the rpc service
 * \author zhenlin
 * \date   2024-04-12
 */

/*
 * Copyright (C) 2008-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__PIPELINE__CLIENT_H_
#define _INCLUDE__PIPELINE__CLIENT_H_

#include <rpc_session/rpc_session.h>
#include <base/rpc_client.h>
#include <base/log.h>

namespace Pipeline { struct Session_clientB;
					struct Session_clientC; }


struct Pipeline::Session_clientB : Genode::Rpc_client<SessionB>
{
	Session_clientB(Genode::Capability<SessionB> cap)
	: Genode::Rpc_client<SessionB>(cap) { }

	void say_hello() override
	{
		Genode::log("issue RPC for saying hello to B");
		call<Rpc_say_hello>();
		Genode::log("returned from 'say_hello' to B");
	}

	int add(int a, int b) override
	{
		return call<Rpc_add>(a, b);
	}

	int funcB0() override
	{
		return call<Rpc_funcB0>();
	}

	int funcC0() override
	{
		return call<Rpc_funcC0>();
	}

	int funcC0_usync() override
	{
		return call<Rpc_funcC0_usync>();
	}

	Genode::uint64_t getcallnum() override
	{
		return call<Rpc_getcallnum>();
	}

};

struct Pipeline::Session_clientC : Genode::Rpc_client<SessionC>
{
	Session_clientC(Genode::Capability<SessionC> cap)
	: Genode::Rpc_client<SessionC>(cap) { }

	void say_hello() override
	{
		Genode::log("issue RPC for saying hello to C");
		call<Rpc_say_hello>();
		Genode::log("returned from 'say_hello' to C");
	}

	int add(int a, int b) override
	{
		return call<Rpc_add>(a, b);
	}

	int funcC0() override
	{
		return call<Rpc_funcC0>();
	}

};

#endif /* _INCLUDE__PIPELINE__CLIENT_H_ */
