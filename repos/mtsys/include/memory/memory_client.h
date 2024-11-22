
#ifndef _INCLUDE__MEMORY__CLIENT_H_
#define _INCLUDE__MEMORY__CLIENT_H_

#include <memory/memory_session.h>
#include <base/rpc_client.h>
#include <base/log.h>

#include <util/list.h>
#include <util/reconstructible.h>
#include <base/ram_allocator.h>
#include <region_map/region_map.h>
#include <base/allocator_avl.h>
#include <base/mutex.h>

#include <util/interface.h>
#include <base/stdint.h>
#include <base/exception.h>
#include <base/quota_guard.h>
#include <base/allocator.h>

namespace MtsysMemory { 
	struct Session_client; 
}


struct MtsysMemory::Session_client : Genode::Rpc_client<Session>
{
	Session_client(Genode::Capability<Session> cap)
	: Genode::Rpc_client<Session>(cap) { }

	int Transform_activation(int flag) override
	{
		return call<Rpc_Transform_activation>(flag);
	}

	int Memory_hello() override
	{
		// Genode::log("issue RPC for saying hello");
		int r = call<Rpc_Memory_hello>();
		// Genode::log("returned from 'say_hello' RPC call");
		return r;
	}

	genode_uint64_t query_free_space() override
	{
		return call<Rpc_query_free_space>();
	}

};

#endif /* _INCLUDE__MEMORY__CLIENT_H_ */ 
