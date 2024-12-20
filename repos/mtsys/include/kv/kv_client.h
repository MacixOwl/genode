
#pragma once

#include <kv/kv_session.h>
#include <base/rpc_client.h>
#include <base/log.h>
#include <base/stdint.h>
#include <base/heap.h>

#include <adl/collections/RedBlackTree.hpp>

namespace MtsysKv { struct Session_client; }


#define MTSYS_KV_CLIENT_ENSURE_DATA_VERSION() \
	do { \
		if (!pRemoteDataVersion) {\
			this->get_data_version_addr(); \
		} \
		Genode::log("remote version: ", *pRemoteDataVersion, ". local version: ", localDataVersion); \
		if (*pRemoteDataVersion != localDataVersion) { \
			localDataVersion = *pRemoteDataVersion; \
			Genode::log("Data updated. Local cache cleared."); \
			cacheDB.clear(); \
		} \
	} while (0)


struct MtsysKv::Session_client : Genode::Rpc_client<Session>
{
	
protected:
	
	Genode::Env& env;
	Genode::Sliced_heap sliced_heap { env.ram(), env.rm() };
	Genode::uint64_t localDataVersion = 0;
	Genode::uint64_t* pRemoteDataVersion = nullptr;

	adl::RedBlackTree<KvRpcString, KvRpcString> cacheDB;

	static const Genode::addr_t remoteDataVersionLocalAddr = 0xf1000000; // TODO: really this address?

public:

	Session_client(Genode::Capability<Session> cap, Genode::Env& env)
	: Genode::Rpc_client<Session>(cap), env(env) 
	{
		if (adl::defaultAllocator.notReady()) {
			adl::defaultAllocator.init({

				.alloc = [] (adl::size_t size, void* data) {
					return reinterpret_cast<Genode::Sliced_heap*>(data)->alloc(size);
				},
				
				.free = [] (void* addr, adl::size_t size, void* data) {
					reinterpret_cast<Genode::Sliced_heap*>(data)->free(addr, size);
				},
				
				.data = &sliced_heap  // todo: really use sliced_heap ?
			});

		}

	}

	~Session_client() {
		if (pRemoteDataVersion) {
			env.rm().detach(remoteDataVersionLocalAddr);
		}
	}

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
		MTSYS_KV_CLIENT_ENSURE_DATA_VERSION();

		if (cacheDB.hasKey(key)) {
			Genode::log("Data found in cache.");
			return cacheDB.getData(key);
		}
		
		Genode::log("Firing RPC read request.");
		
		auto result = call<Rpc_read>(key);
		cacheDB.setData(key, result);
		return result;
	}


	virtual int update(const KvRpcString key, const KvRpcString value) override {
		return call<Rpc_update>(key, value);
	}
	
	virtual Genode::Ram_dataspace_capability range_scan_prepare() override {
		return call<Rpc_range_scan_prepare>();
	}

	virtual Genode::Ram_dataspace_capability get_data_version_addr() override {
		auto cap = call<Rpc_get_data_version_addr>();

		if (!pRemoteDataVersion) {
			env.rm().attach_at(cap, remoteDataVersionLocalAddr);
			pRemoteDataVersion = (Genode::uint64_t*) remoteDataVersionLocalAddr;
		}

		return cap;
	}

	virtual int range_scan(
		const KvRpcString leftBound, 
		const KvRpcString rightBound
	) override {
		return call<Rpc_range_scan>(leftBound, rightBound);
	}

};


#undef MTSYS_KV_CLIENT_ENSURE_DATA_VERSION


