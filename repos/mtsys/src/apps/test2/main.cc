
#include <base/component.h>
#include <base/log.h>


#include <pivot/pivot_connection.h>

void Component::construct(Genode::Env &env)
{	

	MtsysPivot::ServiceHub hub(env);

	hub.Pivot_hello();
	Genode::log("Pivot App ID: ", hub.Pivot_App_getid());

	hub.Kv_hello();

	hub.Memory_hello();
	Genode::log("free space: ", hub.query_free_space());

	// test allocation here
	void* p1 = hub.Memory_alloc(4096);
	Genode::log("Allocated memory at: ", p1);
	void* p2 = hub.Memory_alloc(4096);
	Genode::log("Allocated memory at: ", p2);
	void* p3 = hub.Memory_alloc(4096);
	Genode::log("Allocated memory at: ", p3);
	void* p4 = hub.Memory_alloc(40969);
	Genode::log("Allocated memory at: ", p4);
	// note here we need to enlarge cap in run file, as it is local obj
	void* p5 = hub.Memory_alloc(40960000);
	Genode::log("Allocated memory at: ", p5);
	void* p6 = hub.Memory_alloc(8);
	Genode::log("Allocated memory at: ", p6);

	((char*)p6)[0] = 'a';
	((char*)p6)[1] = 'b';
	((char*)p6)[2] = 'c';
	((char*)p6)[3] = 0;
	Genode::log("p6: ", (const char*)p6);

	// test free here
	hub.Memory_free(p1);
	hub.Memory_free(p2);
	hub.Memory_free(p3);
	hub.Memory_free(p4);
	hub.Memory_free(p5);
	hub.Memory_free(p6);

	// test IPC hotness 
	for (int i = 0; i < 100000; i++) {
		hub.null_function();
	}

	Genode::log("testapp completed");
}
