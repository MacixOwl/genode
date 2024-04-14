/*
 * \brief  Connection to rpc service
 * \author zhenlin
 * \date   2024-04-12
 */

/*
 * Copyright (C) 2008-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__PIPELINE__CONNECTION_H_
#define _INCLUDE__PIPELINE__CONNECTION_H_

#include <rpc_session/client.h>
#include <base/connection.h>

namespace Pipeline { struct Connection2B;
					struct Connection2C; }


struct Pipeline::Connection2B : Genode::Connection<SessionB>, Session_clientB
{
	Connection2B(Genode::Env &env)
	:
		/* create session */
		Genode::Connection<Pipeline::SessionB>(env, Label(),
		                                   Ram_quota { 8*1024 }, Args()),
		/* initialize RPC interface */
		Session_clientB(cap())
	{ }
};

struct Pipeline::Connection2C : Genode::Connection<SessionC>, Session_clientC
{
	Connection2C(Genode::Env &env)
	:
		/* create session */
		Genode::Connection<Pipeline::SessionC>(env, Label(),
		                                   Ram_quota { 8*1024 }, Args()),
		/* initialize RPC interface */
		Session_clientC(cap())
	{ }
};

#endif /* _INCLUDE__PIPELINE__CONNECTION_H_ */
