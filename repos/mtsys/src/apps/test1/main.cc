
#include <base/component.h>
#include <base/log.h>

#include <memory/memory_connection.h>

void Component::construct(Genode::Env &env)
{
	MtsysMemory::Connection mem_obj(env);

	mem_obj.Memory_hello();

	Genode::log("free space: ", mem_obj.query_free_space());

	Genode::log("testapp completed");
}
