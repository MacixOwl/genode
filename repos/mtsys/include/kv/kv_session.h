
#ifndef _INCLUDE__KV_SESSION_H_
#define _INCLUDE__KV_SESSION_H_

#include <session/session.h>
#include <base/rpc.h>

namespace MtsysKv { 
	struct Session; 
	struct cid_4service;
}

struct MtsysKv::cid_4service
{
	int cid_4memory;
	int cid_fake;
};


struct MtsysKv::Session : Genode::Session
{
	static const char *service_name() { return "MtsysKv"; }

	enum { CAP_QUOTA = 4 };

	virtual int Kv_hello() = 0;

	virtual cid_4service get_cid_4services() = 0;

	virtual void null_function() = 0;

	virtual int get_IPC_stats(int client_id) = 0;


	/*******************
	 ** RPC interface **
	 *******************/

	GENODE_RPC(Rpc_Kv_hello, int, Kv_hello);
	GENODE_RPC(Rpc_get_cid_4services, cid_4service, get_cid_4services);
	GENODE_RPC(Rpc_null_function, void, null_function);
	GENODE_RPC(Rpc_get_IPC_stats, int, get_IPC_stats, int);

	GENODE_RPC_INTERFACE(Rpc_Kv_hello, Rpc_get_cid_4services,
						Rpc_null_function, Rpc_get_IPC_stats);
};

#endif /* _INCLUDE__KV_SESSION_H_ */
