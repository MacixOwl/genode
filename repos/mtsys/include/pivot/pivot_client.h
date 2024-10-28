
#ifndef _INCLUDE__PIVOT__CLIENT_H_
#define _INCLUDE__PIVOT__CLIENT_H_

#include <pivot/pivot_session.h>
#include <base/rpc_client.h>
#include <base/log.h>

namespace MtsysPivot { struct Session_client; }


struct MtsysPivot::Session_client : Genode::Rpc_client<Session>
{
	Session_client(Genode::Capability<Session> cap)
	: Genode::Rpc_client<Session>(cap) { }

	void Pivot_hello() override
	{
		// Genode::log("issue RPC for saying hello");
		call<Rpc_Pivot_hello>();
		// Genode::log("returned from 'say_hello' RPC call");
	}

    int Pivot_App_getid() override
    {
        return call<Rpc_Pivot_App_getid>();
    }

    void Pivot_IPC_stats(int appid, int num) override
    {
        call<Rpc_Pivot_IPC_stats>(appid, num);
    }

};

#endif /* _INCLUDE__PIVOT__CLIENT_H_ */
