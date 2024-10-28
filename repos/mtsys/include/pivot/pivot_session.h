// The pivot is a component collecting IPC stats from other components. 
// Meanwhile, it is responsible for triggering service merge and split.

#ifndef _INCLUDE__PIVOT_SESSION_H_
#define _INCLUDE__PIVOT_SESSION_H_

#include <session/session.h>
#include <base/rpc.h>

namespace MtsysPivot { struct Session; }

static const int MAX_SERVICE = 10;
static const int MAX_COMPSVC = (1 << MAX_SERVICE);
static const int MAX_USERAPP = 64;


struct MtsysPivot::Session : Genode::Session
{
	static const char *service_name() { return "MtsysPivot"; }

	enum { CAP_QUOTA = 4 };

	virtual void Pivot_hello() = 0;

	virtual int Pivot_App_getid() = 0;

    virtual void Pivot_IPC_stats(int appid, int num) = 0;


	/*******************
	 ** RPC interface **
	 *******************/

	GENODE_RPC(Rpc_Pivot_hello, void, Pivot_hello);
	GENODE_RPC(Rpc_Pivot_App_getid, int, Pivot_App_getid);
    GENODE_RPC(Rpc_Pivot_IPC_stats, void, Pivot_IPC_stats, int, int);

	GENODE_RPC_INTERFACE(Rpc_Pivot_hello, Rpc_Pivot_App_getid, Rpc_Pivot_IPC_stats);
};

#endif /* _INCLUDE__PIVOT_SESSION_H_ */
