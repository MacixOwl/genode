
#ifndef _INCLUDE__KV__CLIENT_H_
#define _INCLUDE__KV__CLIENT_H_

#include <kv/kv_session.h>
#include <base/rpc_client.h>
#include <base/log.h>

namespace MtsysKv { struct Session_client; }


struct MtsysKv::Session_client : Genode::Rpc_client<Session>
{
	Session_client(Genode::Capability<Session> cap)
	: Genode::Rpc_client<Session>(cap) { }

	void Kv_hello() override
	{
		// Genode::log("issue RPC for saying hello");
		call<Rpc_Kv_hello>();
		// Genode::log("returned from 'say_hello' RPC call");
	}

};

#endif /* _INCLUDE__KV__CLIENT_H_ */
