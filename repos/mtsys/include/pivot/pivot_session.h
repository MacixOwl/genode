// The pivot is a component collecting IPC stats from other components. 
// Meanwhile, it is responsible for triggering service merge and split.

#ifndef _INCLUDE__PIVOT_SESSION_H_
#define _INCLUDE__PIVOT_SESSION_H_

#include <session/session.h>
#include <base/rpc.h>

namespace MtsysPivot { struct Session; }


struct MtsysPivot::Session : Genode::Session
{
	static const char *service_name() { return "MtsysPivot"; }

	enum { CAP_QUOTA = 4 };

	virtual void Pivot_hello() = 0;

    virtual void Pivot_IPC_stats(int pid, int num) = 0;


	/*******************
	 ** RPC interface **
	 *******************/

	GENODE_RPC(Rpc_Pivot_hello, void, Pivot_hello);
    GENODE_RPC(Rpc_Pivot_IPC_stats, void, Pivot_IPC_stats, int, int);

	GENODE_RPC_INTERFACE(Rpc_Pivot_hello, Rpc_Pivot_IPC_stats);
};

#endif /* _INCLUDE__PIVOT_SESSION_H_ */
