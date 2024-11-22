#ifndef _INCLUDE__MEMORY_LOCAL_H_
#define _INCLUDE__MEMORY_LOCAL_H_


#include <util/reconstructible.h>
#include <base/ram_allocator.h>
#include <region_map/region_map.h>
#include <base/allocator_avl.h>
#include <base/mutex.h>
#include <base/env.h>

#include <util/interface.h>
#include <base/stdint.h>
#include <base/exception.h>
#include <base/quota_guard.h>
#include <base/allocator.h>
#include <base/heap.h>


namespace MtsysMemory {  
	class Local_allocator;
}


class MtsysMemory::Local_allocator : public Genode::Allocator
{
	using Alloc_result = Genode::Attempt<void *, Alloc_error>;
private:

	Genode::Env &env;
	int hash_allocated[1024] = {0};
	int ring_buffer[1024] = {0};

	Genode::size_t _quota_used {0};

public:
	
	Local_allocator(Genode::Env &env);

	~Local_allocator();

	Alloc_result try_alloc(Genode::size_t) override;

	void free(void *, Genode::size_t) override;

	Genode::size_t consumed() const override { return _quota_used; }
	
	Genode::size_t overhead(Genode::size_t size) const override 
	{ return 0; }
	
	bool need_size_for_free() const override { return false; }

};


typedef Genode::Attempt<void *, Genode::Allocator::Alloc_error> Alloc_result;

namespace MtsysMemory {

    Local_allocator::Local_allocator(Genode::Env &env)
    :
    env(env)
    {}

    Local_allocator::~Local_allocator()
    {}

    Alloc_result Local_allocator::try_alloc(Genode::size_t size)
    {
        Alloc_error err(Alloc_error::DENIED);
        return err;
    }

    void Local_allocator::free(void *addr, Genode::size_t)
    {
        return;
    }

}


#endif
