
#include <base/component.h>
#include <base/log.h>


#include <pivot/pivot_connection.h>


static int runKvTest(MtsysPivot::ServiceHub& hub) {
	// test kv here
	hub.Kv_insert("qzl", "nice");
	hub.Kv_insert("zbw", "great");
	hub.Kv_insert("fyt", "smart");
	hub.Kv_insert("qzl", "really nice");
	hub.Kv_insert("gty", "cool");
	hub.Kv_insert("zsa", "wow");

	for (int i = 0; i < 2; i++) {
		auto res = hub.Kv_range_scan("aa", "zz");
		int nElements = res->header.dataSize / sizeof(MtsysKv::KvRpcString);
		auto data = (MtsysKv::KvRpcString*) res->data;
		for (int i = 0; i < nElements; i += 2) {
			Genode::log("range scan received: [", data[i], "] -> [", data[i + 1], "]");
		}

		hub.Kv_recycle_rpc_datapack(res);
	}

	auto res1 = hub.Kv_range_scan("p", "r");
	hub.Kv_recycle_rpc_datapack(res1);

	auto res2 = hub.Kv_range_scan("aa", "bb");
	hub.Kv_recycle_rpc_datapack(res2);

	auto r = hub.Kv_read("qzl");
	Genode::log("read result: ", r);
	r = hub.Kv_read("qzl");
	Genode::log("read result: ", r);
	hub.Kv_insert("qqq1", "zzzl");
	r = hub.Kv_read("qqq");
	Genode::log("read result: ", r);
	r = hub.Kv_read("qzl");
	Genode::log("read result: ", r);

	return 0;
}


static int runKvBench(MtsysPivot::ServiceHub& hub, int n) {
	// record start time
	Genode::log("\n\n =================== \n\n");
	Genode::log("KV bench: ", n, " ops");
	Genode::log("\n\n =================== \n\n");
	auto start = hub.Time_now_us().value;

	for (int i = 0; i < n; i++) {
		hub.Kv_insert("qqz", "zzl");
		hub.Kv_del("qqz");
	}
	hub.Kv_insert("qqz", "zzl");
	auto r = hub.Kv_read("qqz");
	Genode::log("read result: ", r);

	// record end time
	auto end = hub.Time_now_us().value;
	Genode::log("\n\n =================== \n\n");
	Genode::log("KV bench: ", n, " ops, time: ", end - start, " us");
	Genode::log("KV bench throughput: ", (float)n * 1000000 / (end - start), " ops/s");
	Genode::log("\n\n =================== \n\n");
	return 0;
}


void Component::construct(Genode::Env &env)
{	
	
	MtsysPivot::ServiceHub hub(env);

	hub.Pivot_hello(); 
	Genode::log("Pivot App ID: ", hub.Pivot_App_getid());

	hub.Memory_hello();
	Genode::log("free space: ", hub.query_free_space());

	hub.Time_usleep(12345);

	hub.Kv_hello();

	runKvTest(hub);

	runKvBench(hub, 10000);


	Genode::log("testapp completed");
}
