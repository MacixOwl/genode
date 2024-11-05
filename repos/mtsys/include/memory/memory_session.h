
#ifndef _INCLUDE__MEMORY_SESSION_H_
#define _INCLUDE__MEMORY_SESSION_H_

#include <session/session.h>
#include <base/rpc.h>

namespace MtsysMemory { struct Session; }


struct MtsysMemory::Session : Genode::Session
{
	static const char *service_name() { return "MtsysMemory"; }

	enum { CAP_QUOTA = 4 };

	virtual int Transform_activation(int flag) = 0;

	virtual int Memory_hello() = 0;

	virtual genode_uint64_t query_free_space() = 0;


	/*******************
	 ** RPC interface **
	 *******************/

	GENODE_RPC(Rpc_Transform_activation, int, Transform_activation, int);
	GENODE_RPC(Rpc_Memory_hello, int, Memory_hello);
	GENODE_RPC(Rpc_query_free_space, genode_uint64_t, query_free_space);

	GENODE_RPC_INTERFACE(Rpc_Transform_activation,
						Rpc_Memory_hello, 
						Rpc_query_free_space);
};

#endif /* _INCLUDE__MEMORY_SESSION_H_ */
