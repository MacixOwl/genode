
#include <base/component.h>
#include <base/log.h>


#include <pivot/pivot_connection.h>

void Component::construct(Genode::Env &env)
{	
	
	MtsysPivot::ServiceHub hub(env);

	hub.Pivot_hello(); 
	Genode::log("Pivot App ID: ", hub.Pivot_App_getid());

	hub.Memory_hello();
	Genode::log("free space: ", hub.query_free_space());

	hub.Time_usleep(100000);

	hub.Kv_hello();

	hub.Kv_insert("111", "aaa");
	hub.Kv_insert("222", "ccc");
	hub.Kv_insert("111", "bbb");

	Genode::log(hub.Kv_read("111").string());

	Genode::log("testapp completed");
}
