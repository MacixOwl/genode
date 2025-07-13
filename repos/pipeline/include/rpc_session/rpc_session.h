/*
 * \brief  Interface definition of the Hello service
 * \author zhenlin
 * \date   2024-04-12
 */

/*
 * Copyright (C) 2008-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__PIPELINE__RPC_SESSION_H_
#define _INCLUDE__PIPELINE__RPC_SESSION_H_

#include <session/session.h>
#include <base/rpc.h>

namespace Pipeline { struct SessionB;
					struct SessionC; }


struct Pipeline::SessionB : Genode::Session
{
	static const char *service_name() { return "PipelineB"; }

	enum { CAP_QUOTA = 4 };

	virtual void say_hello() = 0;
	virtual int add(int a, int b) = 0;
	virtual int funcB0() = 0;
	virtual int funcC0() = 0;
	virtual int funcC0_usync() = 0;
	virtual Genode::uint64_t getcallnum() = 0;


	/*******************
	 ** RPC interface **
	 *******************/

	GENODE_RPC(Rpc_say_hello, void, say_hello);
	GENODE_RPC(Rpc_add, int, add, int, int);
	GENODE_RPC(Rpc_funcB0, int, funcB0);
	GENODE_RPC(Rpc_funcC0, int, funcC0);
	GENODE_RPC(Rpc_funcC0_usync, int, funcC0_usync);
	GENODE_RPC(Rpc_getcallnum, Genode::uint64_t, getcallnum);

	GENODE_RPC_INTERFACE(Rpc_say_hello, Rpc_add, Rpc_funcB0, Rpc_funcC0,
					Rpc_funcC0_usync, Rpc_getcallnum);
};

struct Pipeline::SessionC : Genode::Session
{
	static const char *service_name() { return "PipelineC"; }

	enum { CAP_QUOTA = 4 };

	virtual void say_hello() = 0;
	virtual int add(int a, int b) = 0;
	virtual int funcC0() = 0;


	/*******************
	 ** RPC interface **
	 *******************/

	GENODE_RPC(Rpc_say_hello, void, say_hello);
	GENODE_RPC(Rpc_add, int, add, int, int);
	GENODE_RPC(Rpc_funcC0, int, funcC0);

	GENODE_RPC_INTERFACE(Rpc_say_hello, Rpc_add, Rpc_funcC0);
};

#endif /* _INCLUDE__PIPELINE__RPC_SESSION_H_ */
