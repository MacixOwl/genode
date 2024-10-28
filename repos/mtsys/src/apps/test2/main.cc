
#include <base/component.h>
#include <base/log.h>

#include <memory/memory_connection.h>
#include <kv/kv_connection.h>
#include <pivot/pivot_connection.h>

void Component::construct(Genode::Env &env)
{	
	MtsysPivot::Connection pivot_obj(env);
	MtsysMemory::Connection mem_obj(env);
	MtsysKv::Connection kv_obj(env);

	pivot_obj.Pivot_hello();
	Genode::log("Pivot App ID: ", pivot_obj.Pivot_App_getid());

	mem_obj.Memory_hello();
	Genode::log("free space: ", mem_obj.query_free_space());

	kv_obj.Kv_hello();

	Genode::log("testapp completed");
}
