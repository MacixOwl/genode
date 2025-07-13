#ifndef _INCLUDE__KV__CONNECTION_H_
#define _INCLUDE__KV__CONNECTION_H_

#include <kv/kv_client.h>
#include <base/connection.h>
#include <cpu/atomic.h>
#include <timer_session/connection.h>

#include <pivot/pivot_session.h>

#include <mtsys_options.h>


namespace MtsysKv { 
	struct Connection; 
	struct Work_Thread;
}


struct MtsysKv::Work_Thread : Genode::Thread
{
	volatile int queue_head;
	volatile int queue_tail;

	Timer::Connection timer_obj;
	Timer::Connection timeout_obj;
	int timeout_count = 0;

	MtsysKv::Session_client &client;

	const int cpu_loc = CPUMAP_KV_SERVICE;

	int call_nameid_queue[SERVICE_IPC_QUEUE_SIZE] = {0};
	KvRpcString call_arg1_queue[SERVICE_IPC_QUEUE_SIZE];
	KvRpcString call_arg2_queue[SERVICE_IPC_QUEUE_SIZE];

	Timer::One_shot_timeout<Work_Thread> timeout;

	void timeout_null(Genode::Duration){
		// Genode::log("MtsysKv-Worker: timeout");
	}

	Work_Thread(Genode::Env &env, MtsysKv::Session_client &client)
	:	Genode::Thread(env, "MtsysKv-Worker", 16*1024, 
					Location(cpu_loc,0,SERVICE_CPUWIDTH,1), 
					Weight(5), env.cpu()),
		timer_obj(env),
		timeout_obj(env),
		queue_head(0),
		queue_tail(0),
		client(client),
		timeout(timeout_obj, *this, &Work_Thread::timeout_null)
	{	
		Genode::log("Parent CPU affinity space: ",
			env.cpu().affinity_space().height(), " ", env.cpu().affinity_space().width());
		for (int i = 0; i < SERVICE_IPC_QUEUE_SIZE; i++) {
			call_nameid_queue[i] = 0;
			call_arg1_queue[i] = "";
			call_arg2_queue[i] = "";
		}
		start(); 
	}

	void entry() override
	{
		Genode::log("MtsysKv-Worker started at cpu: ", cpu_loc);
		int head;
		int tail;
		int res;
		int processed = 0;
		while (true){
			res = 0;
			while (!res){
				head = queue_head;
				res = Genode::cmpxchg(&queue_head, head, head);
			}
			res = 0;
			while (!res){
				tail = queue_tail;
				res = Genode::cmpxchg(&queue_tail, tail, tail);
			}
			if (tail - head > 0){
				// Genode::log("MtsysKv-Worker: ", tail - head, " calls in queue");
				for (int i = head; i < tail; i++){
					switch (call_nameid_queue[i % SERVICE_IPC_QUEUE_SIZE])
					{
					case 1:
						client.insert(call_arg1_queue[i % SERVICE_IPC_QUEUE_SIZE], 
									call_arg2_queue[i % SERVICE_IPC_QUEUE_SIZE]);
						// Genode::log("inserted: ", call_arg1_queue[i % SERVICE_IPC_QUEUE_SIZE], 
						// 			" -> ", call_arg2_queue[i % SERVICE_IPC_QUEUE_SIZE]);
						break;
					case 2:
						client.del(call_arg1_queue[i % SERVICE_IPC_QUEUE_SIZE]);
						break;
					case 3:
						call_arg2_queue[i % SERVICE_IPC_QUEUE_SIZE] = 
								client.read(call_arg1_queue[i % SERVICE_IPC_QUEUE_SIZE]);
						// Genode::log("read result put at: ", i % SERVICE_IPC_QUEUE_SIZE, 
						// 			" -> ", call_arg2_queue[i % SERVICE_IPC_QUEUE_SIZE]);
						break;
					case 4:
						client.update(call_arg1_queue[i % SERVICE_IPC_QUEUE_SIZE], 
									call_arg2_queue[i % SERVICE_IPC_QUEUE_SIZE]);
						break;
					default:
						// Genode::log("MtsysKv-Worker: unknown call nameid");
						break;
					}
				}
				res = 0;
				processed = tail - head;
				while (!res){
					head = queue_head + processed;
					// Genode::log("MtsysKv-Worker: ", queue_head, tail, head);
					res = Genode::cmpxchg(&queue_head, head - processed, head);
				}
				// Genode::log("MtsysKv-Worker: ", tail - head, " calls in queue");
			}
#ifdef MTSYS_KV_WAITUS
			else{
				timer_obj.usleep(MTSYS_KV_WAITUS);
				// timeout.schedule(Genode::Microseconds{MTSYS_KV_WAITUS});
				timeout_count++;
				if (timeout_count % 1000 == 0){
					Genode::log("MtsysKv-Worker: timeout count: ", timeout_count);
				}
			}
#endif
		}
	}

};



struct MtsysKv::Connection : Genode::Connection<Session>, Session_client
{

public:

#ifdef MTSYS_OPTION_KVUNCYNC
	Work_Thread worker;

	Connection(Genode::Env &env)
	:
		/* create session */
		Genode::Connection<MtsysKv::Session>(env, Label(),
		                                   Ram_quota { 8*1024 }, Args()),
		/* initialize RPC interface */
		Session_client(cap(), env),
		worker(env, *this)
	{ }
#else
	Connection(Genode::Env &env)
	:
		/* create session */
		Genode::Connection<MtsysKv::Session>(env, Label(),
		                                   Ram_quota { 8*1024 }, Args()),
		/* initialize RPC interface */
		Session_client(cap(), env)
	{ }
#endif

#ifdef MTSYS_OPTION_KVUNCYNC

	int queued_insert(KvRpcString key, KvRpcString value)
	{	
		// Genode::log("queued insert: ", key, " -> ", value);
		int head;
		int tail;
		int res;
		while (true){
			if (worker.queue_tail - worker.queue_head < SERVICE_IPC_QUEUE_SIZE - 1) {
				res = 0;
				while (!res){
					tail = worker.queue_tail;
					res = Genode::cmpxchg(&worker.queue_tail, tail, tail);
				}
				worker.call_nameid_queue[tail % SERVICE_IPC_QUEUE_SIZE] = 1;
				worker.call_arg1_queue[tail % SERVICE_IPC_QUEUE_SIZE] = key;
				worker.call_arg2_queue[tail % SERVICE_IPC_QUEUE_SIZE] = value;
				res = 0;
				while (!res){
					tail = worker.queue_tail + 1;
					res = Genode::cmpxchg(&worker.queue_tail, tail - 1, tail);
				}
				res = 0;
				while (!res){
					head = worker.queue_head;
					res = Genode::cmpxchg(&worker.queue_head, head, head);
				}
				if (tail - head > SERVICE_IPC_QUEUE_SIZE / 2){
					// wait the head to be updated
					res = 0;
					while (!res){
						head = worker.queue_head;
						res = Genode::cmpxchg(&worker.queue_head, head, head);
#ifdef MTSYS_KV_WAITUS
						worker.timer_obj.usleep(MTSYS_KV_WAITUS);
#endif
					} 
				}
				// Genode::log("queue status: ", worker.queue_tail, worker.queue_head);
				return 0;
			}
		}
	}

	// wait for tail and head to be updated and meet each other
	void wait_queue_empty()
	{
		int head;
		int tail;
		int res;
		while (true){
			res = 0;
			while (!res){
				head = worker.queue_head;
				res = Genode::cmpxchg(&worker.queue_head, head, head);
			}
			res = 0;
			while (!res){
				tail = worker.queue_tail;
				res = Genode::cmpxchg(&worker.queue_tail, tail, tail);
			}
			if (tail - head == 0){
				return;
			}
#ifdef MTSYS_KV_WAITUS
			else{
				worker.timer_obj.usleep(MTSYS_KV_WAITUS);
			}
#endif
		}
	}


	int queued_del(KvRpcString key)
	{
		int head;
		int tail;
		int res;
		while (true){
			if (worker.queue_tail - worker.queue_head < SERVICE_IPC_QUEUE_SIZE - 1) {
				res = 0;
				while (!res){
					tail = worker.queue_tail;
					res = Genode::cmpxchg(&worker.queue_tail, tail, tail);
				}
				worker.call_nameid_queue[tail % SERVICE_IPC_QUEUE_SIZE] = 2;
				worker.call_arg1_queue[tail % SERVICE_IPC_QUEUE_SIZE] = key;
				worker.call_arg2_queue[tail % SERVICE_IPC_QUEUE_SIZE] = "";
				res = 0;
				while (!res){
					tail = worker.queue_tail + 1;
					res = Genode::cmpxchg(&worker.queue_tail, tail - 1, tail);
				}
				res = 0;
				while (!res){
					head = worker.queue_head;
					res = Genode::cmpxchg(&worker.queue_head, head, head);
				}
				if (tail - head > SERVICE_IPC_QUEUE_SIZE / 2){
					// wait the head to be updated
					res = 0;
					while (!res){
						head = worker.queue_head;
						res = Genode::cmpxchg(&worker.queue_head, head, head);
#ifdef MTSYS_KV_WAITUS
						worker.timer_obj.usleep(MTSYS_KV_WAITUS);
#endif
					} 
				}
				return 0;
			}
		}
	}

	KvRpcString queued_read(KvRpcString key)
	{
		int head;
		int tail;
		int res;
		int ret;
		while (true){
			if (worker.queue_tail - worker.queue_head < SERVICE_IPC_QUEUE_SIZE - 1) {
#ifndef MTSYS_OPTION_CACHE
				wait_queue_empty();
#endif
				res = 0;
				while (!res){
					tail = worker.queue_tail;
					res = Genode::cmpxchg(&worker.queue_tail, tail, tail);
				}
				worker.call_nameid_queue[tail % SERVICE_IPC_QUEUE_SIZE] = 3;
				worker.call_arg1_queue[tail % SERVICE_IPC_QUEUE_SIZE] = key;
				worker.call_arg2_queue[tail % SERVICE_IPC_QUEUE_SIZE] = "";
				ret = tail;
				res = 0;
				while (!res){
					tail = worker.queue_tail + 1;
					res = Genode::cmpxchg(&worker.queue_tail, tail - 1, tail);
				}
#ifndef MTSYS_OPTION_CACHE
				wait_queue_empty();
#endif

				// Genode::log("returning read result at: ", ret % SERVICE_IPC_QUEUE_SIZE, 
				// 			" -> ", worker.call_arg2_queue[ret % SERVICE_IPC_QUEUE_SIZE]);
				return worker.call_arg2_queue[ret % SERVICE_IPC_QUEUE_SIZE];
			}
		}
	}

	int queued_update(KvRpcString key, KvRpcString value)
	{
#ifdef MTSYS_OPTION_CACHE
		// update cacheDB first
		cacheDB.setData(key, value);
#endif
		int head;
		int tail;
		int res;
		while (true){
			if (worker.queue_tail - worker.queue_head < SERVICE_IPC_QUEUE_SIZE - 1) {
				res = 0;
				while (!res){
					tail = worker.queue_tail;
					res = Genode::cmpxchg(&worker.queue_tail, tail, tail);
				}
				worker.call_nameid_queue[tail % SERVICE_IPC_QUEUE_SIZE] = 4;
				worker.call_arg1_queue[tail % SERVICE_IPC_QUEUE_SIZE] = key;
				worker.call_arg2_queue[tail % SERVICE_IPC_QUEUE_SIZE] = value;
				res = 0;
				while (!res){
					tail = worker.queue_tail + 1;
					res = Genode::cmpxchg(&worker.queue_tail, tail - 1, tail);
				}
				res = 0;
				while (!res){
					head = worker.queue_head;
					res = Genode::cmpxchg(&worker.queue_head, head, head);
				}
				if (tail - head > SERVICE_IPC_QUEUE_SIZE / 2){
					// wait the head to be updated
					res = 0;
					while (!res){
						head = worker.queue_head;
						res = Genode::cmpxchg(&worker.queue_head, head, head);
#ifdef MTSYS_KV_WAITUS
						worker.timer_obj.usleep(MTSYS_KV_WAITUS);
#endif
					} 
				}
				return 0;
			}
		}
	}
#endif

};

#endif /* _INCLUDE__KV__CONNECTION_H_ */
