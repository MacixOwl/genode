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

namespace RPCplus { struct Session; }


struct RPCplus::Session : Genode::Session
{
	static const char *service_name() { return "RPCplus"; }

	enum { CAP_QUOTA = 4 };

	virtual void pure_function() = 0;
	virtual int add_int(int a, int b) = 0;
	virtual int set_storehead(int clientID, int a) = 0;
	virtual Genode::Ram_dataspace_capability get_RPCbuffer(int clientID) = 0;
	virtual int get_storehead(int clientID, int a) = 0;

	/*******************
	 ** RPC interface **
	 *******************/

	GENODE_RPC(Rpc_pure_function, void, pure_function);
	GENODE_RPC(Rpc_add_int, int, add_int, int, int);
	GENODE_RPC(Rpc_set_storehead, int, set_storehead, int, int);
	GENODE_RPC(Rpc_get_RPCbuffer, Genode::Ram_dataspace_capability, get_RPCbuffer, int);
	GENODE_RPC(Rpc_get_storehead, int, get_storehead, int, int);
	GENODE_RPC_INTERFACE(Rpc_pure_function, Rpc_add_int, 
					Rpc_set_storehead, Rpc_get_RPCbuffer, Rpc_get_storehead);
};

#endif /* _INCLUDE__RPCPLUS_SESSION__RPCPLUS_SESSION_H_ */
