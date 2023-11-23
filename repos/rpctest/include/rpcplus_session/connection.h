/*
 * \brief  Connection to RPCplus service
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

#ifndef _INCLUDE__RPCPLUS_SESSION__CONNECTION_H_
#define _INCLUDE__RPCPLUS_SESSION__CONNECTION_H_

#include <rpcplus_session/client.h>
#include <base/connection.h>

namespace RPCplus { struct Connection; }


struct RPCplus::Connection : Genode::Connection<Session>, Session_client
{
	Connection(Genode::Env &env)
	:
		/* create session */
		Genode::Connection<RPCplus::Session>(env, Label(),
		                                   Ram_quota { 32*1024 }, Args()),
		/* initialize RPC interface */
		Session_client(cap())
	{ }
};

#endif /* _INCLUDE__RPCPLUS_SESSION__CONNECTION_H_ */
