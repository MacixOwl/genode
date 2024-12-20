#ifndef _INCLUDE__KV__CONNECTION_H_
#define _INCLUDE__KV__CONNECTION_H_

#include <kv/kv_client.h>
#include <base/connection.h>

namespace MtsysKv { struct Connection; }


struct MtsysKv::Connection : Genode::Connection<Session>, Session_client
{

public:

	Connection(Genode::Env &env)
	:
		/* create session */
		Genode::Connection<MtsysKv::Session>(env, Label(),
		                                   Ram_quota { 8*1024 }, Args()),
		/* initialize RPC interface */
		Session_client(cap(), env)
	{ }
};

#endif /* _INCLUDE__KV__CONNECTION_H_ */
