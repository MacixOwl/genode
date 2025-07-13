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
	Session_client(Genode::Capability<Session> cap)
	: Genode::Rpc_client<Session>(cap) { }

	void pure_function() override
	{
		call<Rpc_pure_function>();
	}

	int add_int(int a, int b) override
	{
		return call<Rpc_add_int>(a, b);
	}

	int set_storehead(int clintID, int a) override
	{
		return call<Rpc_set_storehead>(clintID, a);
	}

	Genode::Ram_dataspace_capability get_RPCbuffer(int clientID) override
	{
		return call<Rpc_get_RPCbuffer>(clientID);
	}

	int get_storehead(int clientID, int a) override
	{
		return call<Rpc_get_storehead>(clientID, a);
	}

};

#endif /* _INCLUDE__RPCPLUS_SESSION_H__CLIENT_H_ */
