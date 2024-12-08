#ifndef _INCLUDE__MEMORY__CONNECTION_H_
#define _INCLUDE__MEMORY__CONNECTION_H_

#include <memory/memory_client.h>
#include <base/connection.h>

namespace MtsysMemory { struct Connection; }


struct MtsysMemory::Connection : Genode::Connection<Session>, Session_client
{
	Connection(Genode::Env &env)
	:
		/* create session */
		Genode::Connection<MtsysMemory::Session>(env, Label(),
		                                   Ram_quota { 8*1024*1024 }, Args()),
		/* initialize RPC interface */
		Session_client(cap())
	{ }
};

#endif /* _INCLUDE__MEMORY__CONNECTION_H_ */
