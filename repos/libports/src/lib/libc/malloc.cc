/*
 * \brief  Simplistic malloc and free implementation
 * \author Norman Feske
 * \author Sebastian Sumpf
 * \date   2006-07-21
 */

/*
 * Copyright (C) 2006-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/env.h>
#include <base/log.h>
#include <base/slab.h>
#include <util/reconstructible.h>
#include <util/string.h>
#include <util/misc_math.h>

/* libc includes */
extern "C" {
#include <string.h>
#include <stdlib.h>
}

/* libc-internal includes */
#include <internal/init.h>
#include <internal/clone_session.h>
#include <internal/errno.h>


namespace Libc {
	class Slab_alloc;
	class Malloc;
}


class Libc::Slab_alloc : public Slab
{
	private:

		size_t const _object_size;

		size_t _calculate_block_size(size_t object_size)
		{
			size_t block_size = 16*object_size;
			return align_addr(block_size, 12);
		}

	public:

		Slab_alloc(size_t object_size, Allocator &backing_store)
		:
			Slab(object_size, _calculate_block_size(object_size), 0, &backing_store),
			_object_size(object_size)
		{ }

		void *alloc()
		{
			return Slab::try_alloc(_object_size).convert<void *>(
				[&] (Allocation &a) { a.deallocate = false; return a.ptr; },
				[&] (Alloc_error)   { return nullptr; });
		}

		void dealloc(void *ptr) { Slab::free(ptr, _object_size); }
};


/**
 * Allocator that uses slabs for small objects sizes
 */
class Libc::Malloc
{
	private:

		using size_t = Genode::size_t;
		using addr_t = Genode::addr_t;

		enum {
			SLAB_START    = 5,  /* 32 bytes (log2) */
			SLAB_STOP     = 11, /* 2048 bytes (log2) */
			NUM_SLABS     = (SLAB_STOP - SLAB_START) + 1,
			DEFAULT_ALIGN = 16
		};

		struct Metadata
		{
			size_t size;
			size_t offset;

			/**
			 * Allocation metadata
			 *
			 * \param size    allocation size
			 * \param offset  offset of pointer from allocation
			 */
			Metadata(size_t size, size_t offset)
			: size(size), offset(offset) { }
		};

		/**
		 * Allocation overhead due to alignment and metadata storage
		 *
		 * We store the metadata of the allocation right before the pointer
		 * returned to the caller and can then retrieve the information when
		 * freeing the block. Therefore, we add room for metadata and
		 * alignment.
		 *
		 * Note, the worst case is an allocation that starts at
		 * 'align' byte - sizeof(Metadata) + 1 because it misses one byte of space
		 * for the metadata and therefore increases the worst-case allocation
		 * by ('align' - 1) bytes additionally to the metadata space.
		 */
		static constexpr size_t _room(size_t align)
		{
			return sizeof(Metadata) + (align - 1);
		}

		Allocator &_backing_store; /* back-end allocator */

		Constructible<Slab_alloc> _slabs[NUM_SLABS]; /* slab allocators */

		Mutex _mutex;

		unsigned _slab_log2(size_t size) const
		{
			unsigned msb = Genode::log2(size);

			/* size is greater than msb */
			if (size > (1U << msb))
				msb++;

			/* use smallest slab */
			if (size < (1U << SLAB_START))
				msb = SLAB_START;

			return msb;
		}

	public:

		Malloc(Allocator &backing_store) : _backing_store(backing_store)
		{
			for (unsigned i = SLAB_START; i <= SLAB_STOP; i++)
				_slabs[i - SLAB_START].construct(1U << i, backing_store);
		}

		~Malloc() { warning(__func__, " unexpectedly called"); }

		/**
		 * Allocator interface
		 */

		void * alloc(size_t size, size_t align = DEFAULT_ALIGN)
		{
			Mutex::Guard guard(_mutex);

			size_t   const real_size = size + _room(align);
			unsigned const msb       = _slab_log2(real_size);

			void *alloc_addr = nullptr;

			/* use backing store if requested memory is larger than largest slab */
			if (msb > SLAB_STOP)
				_backing_store.try_alloc(real_size).with_result(
					[&] (Range_allocator::Allocation &a) {
						a.deallocate = false; alloc_addr = a.ptr; },
					[&] (Alloc_error) { });
			else
				alloc_addr = _slabs[msb - SLAB_START]->alloc();

			if (!alloc_addr) return nullptr;

			/* correctly align the allocation address */
			Metadata * const aligned_addr =
				(Metadata *)(((addr_t)alloc_addr + _room(align)) & ~(align - 1));

			size_t const offset = (addr_t)aligned_addr - (addr_t)alloc_addr;

			*(aligned_addr - 1) = Metadata(real_size, offset);

			return aligned_addr;
		}

		void *realloc(void *ptr, size_t size)
		{
			size_t const real_size     = size + _room(DEFAULT_ALIGN);
			size_t const old_real_size = ((Metadata *)ptr - 1)->size;

			/* do not reallocate if new size is less than the current size */
			if (real_size <= old_real_size)
				return ptr;

			/* allocate new block */
			void *new_addr = alloc(size);

			if (new_addr) {
				/* copy content from old block into new block */
				::memcpy(new_addr, ptr, old_real_size - _room(DEFAULT_ALIGN));

				/* free old block */
				free(ptr);
			}

			return new_addr;
		}

		void free(void *ptr)
		{
			Mutex::Guard lock_guard(_mutex);

			Metadata *md = (Metadata *)ptr - 1;

			size_t   const  real_size  = md->size;
			unsigned const  msb        = _slab_log2(real_size);

			void *alloc_addr = (void *)((addr_t)ptr - md->offset);

			if (md->offset == 0)
				error("libc free: meta-data offset is 0 for address: ", ptr,
				      " - corrupted allocation");

			if (msb > SLAB_STOP) {
				_backing_store.free(alloc_addr, real_size);
			} else {
				_slabs[msb - SLAB_START]->dealloc(alloc_addr);
			}
		}
};


using namespace Libc;


static Malloc *mallocator;


extern "C" void *malloc(size_t size)
{
	return mallocator->alloc(size);
}


extern "C" void *calloc(size_t nmemb, size_t size)
{
	void *addr = malloc(nmemb*size);
	if (addr)
		Genode::memset(addr, 0, nmemb*size);
	return addr;
}


extern "C" void free(void *ptr)
{
	if (ptr) mallocator->free(ptr);
}


extern "C" void *realloc(void *ptr, size_t size)
{
	if (!ptr) return malloc(size);

	if (!size) {
		free(ptr);
		return nullptr;
	}

	return mallocator->realloc(ptr, size);
}


int posix_memalign(void **memptr, size_t alignment, size_t size)
{
	*memptr = mallocator->alloc(size, alignment);

	if (!*memptr)
		return Errno(ENOMEM);

	return 0;
}


/* space for singleton object w/o destructor */
static long _malloc_obj[(sizeof(Malloc) + sizeof(long))/sizeof(long)];


void Libc::init_malloc(Genode::Allocator &heap)
{
	mallocator = construct_at<Malloc>(_malloc_obj, heap);
}


void Libc::init_malloc_cloned(Clone_connection &clone_connection)
{
	clone_connection.object_content(_malloc_obj);
	mallocator = (Malloc *)_malloc_obj;
}


void Libc::reinit_malloc(Genode::Allocator &heap)
{
	construct_at<Libc::Malloc>(_malloc_obj, heap);
}
