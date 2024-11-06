
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

	hub.Kv_insert("qzl", "nice");
	hub.Kv_insert("qzl", "great");

	Genode::log(hub.Kv_read("qzl").string());

	Genode::log("testapp completed");
}
