
#pragma once

#include <session/session.h>
#include <base/rpc.h>
#include <util/array.h>

namespace MtsysKv { 
	struct Session; 
	struct cid_4service;
	struct RPCDataPack;

	using KvRpcString = Genode::String<32>;
}


// now we define a fixed size for the range scan buffer
static int RANGE_SCAN_BUFFER = (1 << 11);


struct MtsysKv::RPCDataPack {

	struct {
		Genode::size_t dataSize;
	} header;

	static const Genode::size_t HEADER_SIZE = sizeof(header);

	Genode::int8_t data[0];
	
} __attribute__((__packed__));


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


	virtual int insert(const KvRpcString key, const KvRpcString value) = 0;
	virtual int del(const KvRpcString key) = 0;
	virtual const KvRpcString read(const KvRpcString key) = 0;
	virtual int update(const KvRpcString key, const KvRpcString value) = 0;

	virtual Genode::Ram_dataspace_capability range_scan(
		const KvRpcString leftBound, 
		const KvRpcString rightBound
	) = 0;



	/*******************
	 ** RPC interface **
	 *******************/

	GENODE_RPC(Rpc_Kv_hello, int, Kv_hello);
	GENODE_RPC(Rpc_get_cid_4services, cid_4service, get_cid_4services);
	GENODE_RPC(Rpc_null_function, void, null_function);
	GENODE_RPC(Rpc_get_IPC_stats, int, get_IPC_stats, int);

	GENODE_RPC(Rpc_insert, int, insert,const KvRpcString, const KvRpcString);
	GENODE_RPC(Rpc_del, int, del, const KvRpcString);
	GENODE_RPC(Rpc_read, const KvRpcString, read, const KvRpcString);
	GENODE_RPC(Rpc_update, int, update, const KvRpcString, const KvRpcString);
	GENODE_RPC(Rpc_range_scan, Genode::Ram_dataspace_capability, range_scan, const KvRpcString, const KvRpcString);


	GENODE_RPC_INTERFACE(
		Rpc_Kv_hello, 
		Rpc_get_cid_4services,				
		Rpc_null_function, 
		Rpc_get_IPC_stats,

		Rpc_insert,
		Rpc_del,
		Rpc_read,
		Rpc_update,
		Rpc_range_scan
	);
};


