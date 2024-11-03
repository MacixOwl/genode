
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


	for (int i = 0; i < 100000; i++) {
		hub.null_function();
	}
	Genode::log("testapp completed");
}
