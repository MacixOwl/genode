#ifndef _INCLUDE__PIVOT__CONNECTION_H_
#define _INCLUDE__PIVOT__CONNECTION_H_

#include <pivot/pivot_client.h>
#include <base/connection.h>

namespace MtsysPivot { struct Connection; }


struct MtsysPivot::Connection : Genode::Connection<Session>, Session_client
{
	Connection(Genode::Env &env)
	:
		/* create session */
		Genode::Connection<MtsysPivot::Session>(env, Label(),
		                                   Ram_quota { 8*1024 }, Args()),
		/* initialize RPC interface */
		Session_client(cap())
	{ }
};

#endif /* _INCLUDE__PIVOT__CONNECTION_H_ */
