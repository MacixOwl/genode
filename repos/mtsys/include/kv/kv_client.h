
#ifndef _INCLUDE__KV__CLIENT_H_
#define _INCLUDE__KV__CLIENT_H_

#include <kv/kv_session.h>
#include <base/rpc_client.h>
#include <base/log.h>
#include <base/stdint.h>

namespace MtsysKv { struct Session_client; }


struct MtsysKv::Session_client : Genode::Rpc_client<Session>
{
	Session_client(Genode::Capability<Session> cap)
	: Genode::Rpc_client<Session>(cap) { }

	int Kv_hello() override
	{
		// Genode::log("issue RPC for saying hello");
		int r = call<Rpc_Kv_hello>();
		// Genode::log("returned from 'say_hello' RPC call");
		return r;
	}

	MtsysKv::cid_4service get_cid_4services() override
	{
		// Genode::log("issue RPC for getting cids");
		return call<Rpc_get_cid_4services>();
		// Genode::log("returned from 'get_cids' RPC call");
	}

	void null_function() override
	{
		call<Rpc_null_function>();
	}

	int get_IPC_stats(int client_id) override
	{
		return call<Rpc_get_IPC_stats>(client_id);
	}


	virtual int insert(const KvRpcString key, const KvRpcString value) override {
		return call<Rpc_insert>(key, value);
	}


	virtual int del(const KvRpcString key) override {
		return call<Rpc_del>(key);
	}


	virtual const KvRpcString read(const KvRpcString key) override {
		return call<Rpc_read>(key);
	}


	virtual int update(const KvRpcString key, const KvRpcString value) override {
		return call<Rpc_update>(key, value);
	}
	
	virtual Genode::Ram_dataspace_capability range_scan_prepare() override {
		return call<Rpc_range_scan_prepare>();
	}

	virtual int range_scan(
		const KvRpcString leftBound, 
		const KvRpcString rightBound
	) override {
		return call<Rpc_range_scan>(leftBound, rightBound);
	}

};

#endif /* _INCLUDE__KV__CLIENT_H_ */
