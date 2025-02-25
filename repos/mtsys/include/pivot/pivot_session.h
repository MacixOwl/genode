// The pivot is a component collecting IPC stats from other components. 
// Meanwhile, it is responsible for triggering service merge and split.

#ifndef _INCLUDE__PIVOT_SESSION_H_
#define _INCLUDE__PIVOT_SESSION_H_

#include <session/session.h>
#include <base/rpc.h>
#include <mtsys_options.h>

namespace MtsysPivot { 
	struct Service_Main_Id;
	struct Session; 
}

static const int MAX_SERVICE = 10;
static const int MAX_COMPSVC = (1 << MAX_SERVICE);
static const int MAX_USERAPP = 64;

enum SERVICE_VIRTUAL_ID {
	SID_MEMORY_SERVICE = 0,
	SID_KV_SERVICE = 1,
	SID_BLOCK_SERVICE = 2,
	SID_FS_SERVICE = 3
};

// Remember to update the service name array when adding new service
// And also update the run script to match the cpu mapping
enum SERVICE_CPUMAP {
	CPUMAP_MEMORY_SERVICE = 2,
	CPUMAP_KV_SERVICE = 4,
	CPUMAP_BLOCK_SERVICE = 6,
	CPUMAP_FS_SERVICE = 6
};

const int SERVICE_CPUWIDTH = 2;


struct MtsysPivot::Service_Main_Id
{
	int id_array[MAX_SERVICE] = { 0 };
};



inline const char *id2_service_name(int service)
{
	switch (service) {
		case SID_MEMORY_SERVICE: return "Memory";
		case SID_KV_SERVICE: return "KV";
		case SID_BLOCK_SERVICE: return "Block";
		case SID_FS_SERVICE: return "FS";
		default: return "Unknown";
	}
}

inline int compsvc_id(int id1, int id2)
{
	return (1 << id1) | (1 << id2);
}

struct MtsysPivot::Session : Genode::Session
{
	static const char *service_name() { return "MtsysPivot"; }

	enum { CAP_QUOTA = 4 };

	virtual void Pivot_hello() = 0;

	virtual int Pivot_App_getid() = 0;

	virtual Service_Main_Id Pivot_service_mainIDs() = 0;

    virtual void Pivot_IPC_stats(int appid, int num) = 0;


	/*******************
	 ** RPC interface **
	 *******************/

	GENODE_RPC(Rpc_Pivot_hello, void, Pivot_hello);
	GENODE_RPC(Rpc_Pivot_App_getid, int, Pivot_App_getid);
	GENODE_RPC(Rpc_Pivot_service_mainIDs, Service_Main_Id, Pivot_service_mainIDs);
    GENODE_RPC(Rpc_Pivot_IPC_stats, void, Pivot_IPC_stats, int, int);

	GENODE_RPC_INTERFACE(Rpc_Pivot_hello, 
						Rpc_Pivot_service_mainIDs,
						Rpc_Pivot_App_getid, 
						Rpc_Pivot_IPC_stats);
};

#endif /* _INCLUDE__PIVOT_SESSION_H_ */
