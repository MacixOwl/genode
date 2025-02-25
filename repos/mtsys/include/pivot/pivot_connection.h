
#pragma once

#include <base/connection.h>
#include <timer_session/connection.h>

#include <pivot/pivot_client.h>
#include <kv/kv_connection.h>
#include <memory/memory_connection.h>
#include <memory/local_allocator.h>
#include <fs/fs_connection.h>
#include <cpu/atomic.h>


#include <base/heap.h>


namespace MtsysPivot { 
	struct Connection; 
	struct ServiceHub;
	struct Pivot_Thread;
}


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



struct MtsysPivot::ServiceHub {
	Genode::Env &env;
	Timer::Connection timer_obj;
	MtsysPivot::Connection pivot_obj;

	Genode::Sliced_heap sliced_heap { env.ram(), env.rm() };

	int service_main_id_cache[MAX_SERVICE] = { 0 };
	int main_id_cache_dirty = 1;

	MtsysMemory::Connection mem_obj;
	MtsysMemory::Local_allocator alloc_obj;

	MtsysKv::Connection kv_obj;
	int kvrpc_dataspace_prepared = 0;
	static const Genode::addr_t Kvrpc_Addr = 0xa0000000; // TODO: really this address?

	MtsysFs::Connection fs_obj;
	int fsIO_dataspace_prepared = 0;
	Genode::addr_t fsIO_Addr = 0xa4000000;

	ServiceHub(Genode::Env &env)
	: env(env), 
	timer_obj(env), 
	pivot_obj(env),
	service_main_id_cache(),
	mem_obj(env), 
	alloc_obj(env, mem_obj),
	kv_obj(env),
	fs_obj(env)
	{ 
		// get and attach dataspace for fsIO
		auto fsIO_ds = fs_obj.get_ds_cap();
		env.rm().attach_at(fsIO_ds, fsIO_Addr);
		fsIO_dataspace_prepared = 1;
	}

	~ServiceHub() {
		if (kvrpc_dataspace_prepared) {
			env.rm().detach(Kvrpc_Addr);
		}
		if (fsIO_dataspace_prepared) {
			env.rm().detach(fsIO_Addr);
		}
	}

	void update_service_main_id_cache(){
		if (main_id_cache_dirty){
			MtsysPivot::Service_Main_Id ids = pivot_obj.Pivot_service_mainIDs();
			for (int i = 0; i < MAX_SERVICE; i++) {
				service_main_id_cache[i] = ids.id_array[i];
			}
			main_id_cache_dirty = 0;
		}
	}

	// APIs from Timer
	void Time_usleep(int usec) { timer_obj.usleep(usec); }
	Genode::Milliseconds Time_now_ms() { 
		return timer_obj.curr_time().trunc_to_plain_ms(); }
	Genode::Microseconds Time_now_us() { 
		return timer_obj.curr_time().trunc_to_plain_us(); }

	// APIs from pivot
	void Pivot_hello() {
		pivot_obj.Pivot_hello(); 
	}
	int Pivot_App_getid() { 
		return pivot_obj.Pivot_App_getid(); 
	}
	void Pivot_IPC_stats(int appid, int num) { 
		pivot_obj.Pivot_IPC_stats(appid, num); 
	}

	// APIs from memory
	int Memory_hello() { 
		while (1){
			int res = -1;
			switch (service_main_id_cache[SID_MEMORY_SERVICE])
			{
			case 1: // only memory service
				res = mem_obj.Memory_hello(); 
				break;
			default:
				break;
			}
			if (res == -1){
				Genode::log("[INFO] Memory service not available or Main ID cache not updated");
				main_id_cache_dirty = 1;
				update_service_main_id_cache();
			}
			else{
				return res;
			}
		}
	}

	genode_uint64_t query_free_space() { return mem_obj.query_free_space(); }

	void* Memory_alloc(int size) { 
		return alloc_obj.alloc(size); 
	}

	void Memory_free(void* addr) { 
		alloc_obj.free(addr); 
	}


	// APIs from kv
	int Kv_hello() { 

		while (true) {
			// the service here are composed services, and service_main_id is a composed ID, 
			// where i-th bit represents its availability (inclusion) of i-th service
			if (service_main_id_cache[SID_KV_SERVICE] == 2) {  // question: why 2 is for kv service?
				return kv_obj.Kv_hello(); 
			} else {
				Genode::log("[INFO] Kv service not available or Main ID cache not updated");
				main_id_cache_dirty = 1;
				update_service_main_id_cache();
			}
		}
	}


	int Kv_insert(const MtsysKv::KvRpcString key, const MtsysKv::KvRpcString value) {
#ifdef MTSYS_OPTION_KVUNCYNC
			return kv_obj.queued_insert(key, value);
#else
			return kv_obj.insert(key, value);
#endif
	}

	int Kv_del(const MtsysKv::KvRpcString key) {
#ifdef MTSYS_OPTION_KVUNCYNC
			return kv_obj.queued_del(key);
#else
			return kv_obj.del(key);
#endif
	}

	const MtsysKv::KvRpcString Kv_read(const MtsysKv::KvRpcString key) {
#ifdef MTSYS_OPTION_KVUNCYNC
			return kv_obj.queued_read(key);
#else
			return kv_obj.read(key);
#endif
	}

	int Kv_update(const MtsysKv::KvRpcString key, const MtsysKv::KvRpcString value) {
#ifdef MTSYS_OPTION_KVUNCYNC
			return kv_obj.queued_update(key, value);
#else
		 	return kv_obj.update(key, value);
#endif
	}


	/**
	 * The caller is responsible for freeing the memory 
	 * received from this structure by simply calling
	 * `Kv_recycle_rpc_datapack`.
	 */
	MtsysKv::RPCDataPack* Kv_range_scan(
		const MtsysKv::KvRpcString leftBound, 
		const MtsysKv::KvRpcString rightBound
	) {
		if (!kvrpc_dataspace_prepared) {
			auto ds = kv_obj.range_scan_prepare();
			env.rm().attach_at(ds, Kvrpc_Addr);
			kvrpc_dataspace_prepared = 1;
		}
#ifdef MTSYS_OPTION_KVUNCYNC
		kv_obj.wait_queue_empty();
#endif
		kv_obj.range_scan(leftBound, rightBound);

		auto pTmpData = (MtsysKv::RPCDataPack*)Kvrpc_Addr;

		Genode::size_t dataPackSize = pTmpData->header.dataSize + pTmpData->HEADER_SIZE;

		auto data = (MtsysKv::RPCDataPack*) sliced_heap.alloc(dataPackSize);
		Genode::memcpy(data, pTmpData, dataPackSize);
		return data;
	}

	
	void Kv_recycle_rpc_datapack(MtsysKv::RPCDataPack* dataPack) {
		sliced_heap.free(dataPack, dataPack->HEADER_SIZE + dataPack->header.dataSize);
	}


	// MtsysKv::cid_4service get_cid_4services() { return kv_obj.get_cid_4services(); }
	void null_function() { kv_obj.null_function(); }
	int get_IPC_stats(int client_id) { return kv_obj.get_IPC_stats(client_id); }

	// APIs from fs
	int Fs_hello() { 
		while (1){
			int res = -1;
			switch (service_main_id_cache[SID_FS_SERVICE])
			{
			case 8: // only fs service
				res = fs_obj.Fs_hello(); 
				break;
			default:
				break;
			}
			if (res == -1){
				Genode::log("[INFO] Fs service not available or Main ID cache not updated");
				main_id_cache_dirty = 1;
				update_service_main_id_cache();
			}
			else{
				return res;
			}
		}
	}

	int Fs_open(const MtsysFs::FsPathString path, unsigned flags, unsigned mode) {
		return fs_obj.open(path, flags, mode);
	}

	int Fs_close(int fd) {
		return fs_obj.close(fd);
	}

	int Fs_mkdir(const MtsysFs::FsPathString path, unsigned mode) {
		return fs_obj.mkdir(path, mode);
	}

	int Fs_rmdir(const MtsysFs::FsPathString path) {
		return fs_obj.rmdir(path);
	}

	int Fs_unlink(const MtsysFs::FsPathString path) {
		return fs_obj.unlink(path);
	}

	int Fs_rename(const MtsysFs::FsPathString from, const MtsysFs::FsPathString to) {
		return fs_obj.rename(from, to);
	}

	int Fs_fstat(const MtsysFs::FsPathString path, MtfStat &stat) {
		return fs_obj.fstat(path, stat);
	}

	int Fs_read(int fd, char *buf, Genode::size_t count) {
		// read one or multiple times to get all data, each time at most FILEIO_DSSIZE
		Genode::size_t read_count = 0;
		while (read_count < count) {
			Genode::size_t read_size = count - read_count;
			if (read_size > FILEIO_DSSIZE) {
				read_size = FILEIO_DSSIZE;
			}
			int res = fs_obj.read(fd, 0, read_size);
			if (res == -1) {
				return -1;
			}
			// copy data from fsIO dataspace
			Genode::memcpy(buf + read_count, (void*)fsIO_Addr, read_size);
			read_count += read_size;
		}
		buf[read_count] = '\0';
		return (int)read_count;
	}

	int Fs_write(int fd, const char *buf, Genode::size_t count) {
		// write one or multiple times to write all data, each time at most FILEIO_DSSIZE
		Genode::size_t write_count = 0;
		while (write_count < count) {
			Genode::size_t write_size = count - write_count;
			if (write_size > FILEIO_DSSIZE) {
				write_size = FILEIO_DSSIZE;
			}
			// copy data to fsIO dataspace
			Genode::memcpy((void*)fsIO_Addr, buf + write_count, write_size);
			int res = fs_obj.write(fd, 0, write_size);
			if (res == -1) {
				return -1;
			}
			write_count += write_size;
		}
		return (int)write_count;
	}

	int Fs_ftruncate(int fd, Genode::size_t length) {
		return fs_obj.ftruncate(fd, length);
	}

};


