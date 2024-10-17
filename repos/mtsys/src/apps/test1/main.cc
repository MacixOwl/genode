
#include <base/component.h>
#include <base/log.h>
#include <timer_session/connection.h>

#include <memory/memory_connection.h>
#include <kv/kv_connection.h>

void Component::construct(Genode::Env &env)
{
	Timer::Connection timer(env);
	MtsysMemory::Connection mem_obj(env);
	MtsysKv::Connection kv_obj(env);

	mem_obj.Memory_hello();
	Genode::log("free space: ", mem_obj.query_free_space());

	timer.usleep(100000);

	kv_obj.Kv_hello();

	Genode::log("testapp completed");
}
