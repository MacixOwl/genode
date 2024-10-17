
#ifndef _INCLUDE__KV_SESSION_H_
#define _INCLUDE__KV_SESSION_H_

#include <session/session.h>
#include <base/rpc.h>

namespace MtsysKv { struct Session; }


struct MtsysKv::Session : Genode::Session
{
	static const char *service_name() { return "MtsysKv"; }

	enum { CAP_QUOTA = 4 };

	virtual void Kv_hello() = 0;


	/*******************
	 ** RPC interface **
	 *******************/

	GENODE_RPC(Rpc_Kv_hello, void, Kv_hello);

	GENODE_RPC_INTERFACE(Rpc_Kv_hello);
};

#endif /* _INCLUDE__KV_SESSION_H_ */
