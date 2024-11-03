
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

	hub.timer_obj.usleep(100000);

	hub.Kv_hello();

	Genode::log("testapp completed");
}
