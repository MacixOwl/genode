/*
 * \brief  Interface definition of the RPC service
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

#ifndef _INCLUDE__RPCPLUS_SESSION__RPCPLUS_SESSION_H_
#define _INCLUDE__RPCPLUS_SESSION__RPCPLUS_SESSION_H_

#include <session/session.h>
#include <base/rpc.h>
#include <base/attached_ram_dataspace.h>
#include <base/capability.h>
#include <base/rpc_server.h>
#include <base/attached_ram_dataspace.h>
#include <base/session_object.h>
#include <dataspace/capability.h>
#include <base/rpc_server.h>
#include <dataspace/client.h>

namespace RPCplus { struct Session; struct AllocRet; }

const Genode::addr_t ALLOC_BASE = 0x70000000;


struct RPCplus::Session : Genode::Session
{
	static const char *service_name() { return "RPCplus"; }

	enum { CAP_QUOTA = 4 };

	virtual void say_hello() = 0;
	virtual int add(int a, int b) = 0;
	virtual Genode::Ram_dataspace_capability dataspace() = 0;
	virtual int send2server() = 0;
	virtual Genode::Ram_dataspace_capability memorycap() = 0;
	virtual RPCplus::AllocRet allocate(genode_uint64_t lens) = 0;
	virtual int free(RPCplus::AllocRet ret) = 0;
	virtual Genode::Ram_dataspace_capability capfrom_pagelist(int i) = 0;

	/*******************
	 ** RPC interface **
	 *******************/

	GENODE_RPC(Rpc_say_hello, void, say_hello);
	GENODE_RPC(Rpc_add, int, add, int, int);
	GENODE_RPC(Rpc_dataspace, Genode::Ram_dataspace_capability, dataspace);	
	GENODE_RPC(Rpc_send2server, int, send2server);
	GENODE_RPC(Rpc_memorycap, Genode::Ram_dataspace_capability, memorycap);	
	GENODE_RPC(Rpc_allocate, RPCplus::AllocRet, allocate, genode_uint64_t);
	GENODE_RPC(Rpc_free, int, free, RPCplus::AllocRet);
	GENODE_RPC(Rpc_capfrom_pagelist, Genode::Ram_dataspace_capability, 
				capfrom_pagelist, int);
	GENODE_RPC_INTERFACE(Rpc_say_hello, Rpc_add, Rpc_dataspace, Rpc_send2server, 
						Rpc_memorycap, Rpc_allocate, Rpc_free, Rpc_capfrom_pagelist);
};

struct RPCplus::AllocRet
{
	genode_uint64_t addr;
	genode_uint64_t lent;
	genode_uint64_t id;
};

#endif /* _INCLUDE__RPCPLUS_SESSION__RPCPLUS_SESSION_H_ */
