
#ifndef _INCLUDE__MEMORY_SESSION_H_
#define _INCLUDE__MEMORY_SESSION_H_

#include <session/session.h>
#include <base/rpc.h>
#include <base/attached_ram_dataspace.h>

namespace MtsysMemory { struct Session; }


struct MtsysMemory::Session : Genode::Session
{
	static const char *service_name() { return "MtsysMemory"; }

	enum { CAP_QUOTA = 4 };

	virtual int Transform_activation(int flag) = 0;

	virtual int Memory_hello() = 0;

	virtual genode_uint64_t query_free_space() = 0;

	virtual Genode::Ram_dataspace_capability Memory_alloc(int size, Genode::addr_t &addr) = 0;

	virtual int Memory_free(Genode::addr_t addr) = 0;

	/*******************
	 ** RPC interface **
	 *******************/

	GENODE_RPC(Rpc_Transform_activation, int, Transform_activation, int);
	GENODE_RPC(Rpc_Memory_hello, int, Memory_hello);
	GENODE_RPC(Rpc_query_free_space, genode_uint64_t, query_free_space);
	GENODE_RPC(Rpc_Memory_alloc, Genode::Ram_dataspace_capability, Memory_alloc, int, Genode::addr_t&);
	GENODE_RPC(Rpc_Memory_free, int, Memory_free, Genode::addr_t);

	GENODE_RPC_INTERFACE(Rpc_Transform_activation,
						Rpc_Memory_hello, 
						Rpc_query_free_space,
						Rpc_Memory_alloc,
						Rpc_Memory_free);
};

#endif /* _INCLUDE__MEMORY_SESSION_H_ */
