
#include <base/component.h>
#include <base/log.h>


#include <pivot/pivot_connection.h>


static int runKvTest(MtsysPivot::ServiceHub& hub) {
	hub.Kv_insert("qzl", "nice");
	hub.Kv_insert("zbw", "great");
	hub.Kv_insert("fyt", "smart");
	hub.Kv_insert("qzl", "really nice");
	hub.Kv_insert("gty", "cool");
	hub.Kv_insert("zsa", "wow");

	for (int i = 0; i < 100; i++) {
		auto res = hub.Kv_range_scan("aa", "zz");
		int nElements = res->header.dataSize / sizeof(MtsysKv::KvRpcString);
		auto data = (MtsysKv::KvRpcString*) res->data;
		for (int i = 0; i < nElements; i += 2) {
			Genode::log("range scan received: [", data[i], "] -> [", data[i + 1], "]");
		}

		hub.Kv_recycle_rpc_datapack(res);
	}

	auto res = hub.Kv_range_scan("aa", "bb");
	hub.Kv_recycle_rpc_datapack(res);

	auto r = hub.Kv_read("qzl");
	Genode::log("read result: ", r);
	r = hub.Kv_read("qqq");
	Genode::log("read result: ", r);

	return 0;
}


void Component::construct(Genode::Env &env)
{	
	
	MtsysPivot::ServiceHub hub(env);

	hub.Pivot_hello(); 
	Genode::log("Pivot App ID: ", hub.Pivot_App_getid());

	hub.Memory_hello();
	Genode::log("free space: ", hub.query_free_space());

	hub.Time_usleep(100000);

	hub.Kv_hello();

	runKvTest(hub);


	Genode::log("testapp completed");
}
