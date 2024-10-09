
#ifndef _INCLUDE__MEMORY__CLIENT_H_
#define _INCLUDE__MEMORY__CLIENT_H_

#include <memory/memory_session.h>
#include <base/rpc_client.h>
#include <base/log.h>

namespace MtsysMemory { struct Session_client; }


struct MtsysMemory::Session_client : Genode::Rpc_client<Session>
{
	Session_client(Genode::Capability<Session> cap)
	: Genode::Rpc_client<Session>(cap) { }

	void Memory_hello() override
	{
		// Genode::log("issue RPC for saying hello");
		call<Rpc_Memory_hello>();
		// Genode::log("returned from 'say_hello' RPC call");
	}

	genode_uint64_t query_free_space() override
	{
		return call<Rpc_query_free_space>();
	}

};

#endif /* _INCLUDE__MEMORY__CLIENT_H_ */
