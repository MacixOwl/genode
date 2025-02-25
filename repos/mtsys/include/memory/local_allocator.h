#ifndef _INCLUDE__MEMORY_LOCAL_H_
#define _INCLUDE__MEMORY_LOCAL_H_


#include <util/reconstructible.h>
#include <base/ram_allocator.h>
#include <region_map/region_map.h>
#include <base/allocator_avl.h>
#include <base/attached_ram_dataspace.h>
#include <base/mutex.h>
#include <base/env.h>

#include <util/interface.h>
#include <base/stdint.h>
#include <base/exception.h>
#include <base/quota_guard.h>
#include <base/allocator.h>
#include <base/heap.h>

#include <memory/memory_connection.h>
#include <mtsys_options.h>


namespace MtsysMemory {  
    class Local_allocator;
    struct LocalMemobj;
}


const unsigned long REMOTE_MEMADDR = 0x1000000000;

const int DS_MIN_SIZE = 4096 << 2;
const int DS_MAX_SIZE = 4096 << 13;
const int DS_SIZE_LEVELS = 12;
const int SLAB_MIN_SIZE = 4 << 2;
const int SLAB_MAX_SIZE = 4096 << 11;
const int SLAB_SIZE_LEVELS = 20;

const int HASH_SIZE = 4096;
const int HASH_CAPACITY = 20;
const int RING_SIZE = 4096;
const int RING_LEVEL = SLAB_SIZE_LEVELS;
const int PIN_CAPACITY = 32;


inline constexpr static int DS_SIZE2LEVEL(int size) {
#if defined(__GNUC__)

    if (__builtin_popcount(size) != 1)
        return -1;
    
    auto ffs = __builtin_ffs(size);
    return ffs < 15 ? -1 : (ffs - 15);  // todo: what about when size larger than 33554432?

#else
    switch (size)
    {
    case 16384: return 0;
    case 32768: return 1;
    case 65536: return 2;
    case 131072: return 3;
    case 262144: return 4;
    case 524288: return 5;
    case 1048576: return 6;
    case 2097152: return 7;
    case 4194304: return 8;
    case 8388608: return 9;
    case 16777216: return 10;
    case 33554432: return 11;
    default: return -1;
    }
#endif
}


inline constexpr static int SLAB_SIZE2LEVEL(int size) {
#if defined(__GNUC__)

    if (__builtin_popcount(size) != 1)
        return -1;
    
    auto ffs = __builtin_ffs(size);
    return ffs < 5 ? -1 : (ffs - 5);  // todo: what about when size larger than 8388608?

#else


    switch (size)
    {
    case 16: return 0;
    case 32: return 1;
    case 64: return 2;
    case 128: return 3;
    case 256: return 4;
    case 512: return 5;
    case 1024: return 6;
    case 2048: return 7;
    case 4096: return 8;
    case 8192: return 9;
    case 16384: return 10;
    case 32768: return 11;
    case 65536: return 12;
    case 131072: return 13;
    case 262144: return 14;
    case 524288: return 15;
    case 1048576: return 16;
    case 2097152: return 17;
    case 4194304: return 18;
    case 8388608: return 19;
    default: return -1;
    }
#endif
}


static inline constexpr int highest_oneBit(int n) {
    if (n == 0) 
        return 0;

#if defined(__GNUC__)

    return 1 << (32 - __builtin_clz(n) - 1);

#else
    n |= (n >> 1);
    n |= (n >> 2);
    n |= (n >> 4);
    n |= (n >> 8);
    n |= (n >> 16);
    return n & ~(n >> 1);
#endif
}


int best_fit_slab(int size) {
    int b = highest_oneBit(size);
    if (b != size) b = (b << 1);
    if (b < SLAB_MIN_SIZE) return SLAB_MIN_SIZE;
    if (b > SLAB_MAX_SIZE) {
        // Genode::error("Requested size too large for slab");
        return -1;
    }
    return b;
}


int best_fit_ds(int size) {
    if (size > DS_MAX_SIZE) {
        // Genode::error("Requested size too large for dataspace");
        return -1;
    }
    int s = best_fit_slab(size);
    if (s == -1) return size;
    if (s < DS_MIN_SIZE) return DS_MIN_SIZE;
    s = (s << 2);
    return s;
}


int hash_bucket(unsigned long addr, int bucket_size) {
    // first remove all trailing zeros
    if (addr == 0) return 0;
    unsigned long h = addr;
    while ((h & 1) == 0) h >>= 1;
    // then restrict within hash bucket size
    h = h % bucket_size;
    return (int)h;
}


struct MtsysMemory::LocalMemobj {
    Genode::Ram_dataspace_capability *cap;
    unsigned long addr;
    unsigned long size;
    LocalMemobj() : cap(0), addr(0), size(0) { }
};



class MtsysMemory::Local_allocator : public Genode::Allocator
{
    using Alloc_result = Genode::Attempt<void *, Alloc_error>;
private:

    Genode::Env &env;
    Genode::Sliced_heap sliced_heap { env.ram(), env.rm() };
    MtsysMemory::Connection &mem_obj;

    // hash table record pinned mappings of an address
    // HASH_SIZE * HASH_CAPACITY
    unsigned long *hash_keys;
    int *pinned_mappings;
    // ring buffer store the local cache of slab, with address and base of slab
    // RING_LEVEL * RING_SIZE
    unsigned long *ring_buffer;
    unsigned long *slab_base;
    // pin pointers to ram_dataspace for each allocated memory
    // HASH_SIZE * PIN_CAPACITY
    unsigned long *pin_hash;
    unsigned long *pin_addr;
    // dataspace hook
    // HASH_SIZE * PIN_CAPACITY
    unsigned long *dsaddr_hash;
    MtsysMemory::LocalMemobj** capaddr_list;
    int ring_head[RING_LEVEL] = {0};
    int ring_tail[RING_LEVEL] = {0};

    Genode::size_t _quota_used {0};

    MtsysMemory::LocalMemobj* alloc_ds(int size);
    int free_atds(void* addr);
    int add_new_slab(void* ds_addr, int slab_size, int num_slabs);
    int insert_hash(void* addr, int num_slabs);
    int reduce_hash(unsigned long addr);

public:
    
    Local_allocator(Genode::Env &env, MtsysMemory::Connection &mem_obj);

    ~Local_allocator();

    Alloc_result try_alloc(Genode::size_t) override;

    void free(void *, Genode::size_t) override;

    Genode::size_t consumed() const override { return _quota_used; }
    
    Genode::size_t overhead(Genode::size_t size) const override 
    { return 0; }
    
    bool need_size_for_free() const override { return false; }

    void free(void *addr) { free(addr, 0); }

};


typedef Genode::Attempt<void *, Genode::Allocator::Alloc_error> Alloc_result;

namespace MtsysMemory {

    int Local_allocator::add_new_slab(void* ds_addr, int slab_size, int num_slabs) {
        int slab_level = SLAB_SIZE2LEVEL(slab_size);
        for (int i = 1; i <= num_slabs; i++) {
            if (ring_head[slab_level] == (ring_tail[slab_level] + i) % RING_SIZE) {
                Genode::error("ring buffer full");
                return -1;
            }
        }
        int p = slab_level * RING_SIZE + ring_tail[slab_level];
        unsigned long addr_base = (unsigned long)ds_addr + slab_size;
        for (int i = 0; i < num_slabs; i++) {
            ring_buffer[p] = addr_base + i * slab_size;
            slab_base[p] = (unsigned long)ds_addr;
            p = (p + 1) % RING_SIZE + slab_level * RING_SIZE;
        }
        ring_tail[slab_level] = (ring_tail[slab_level] + num_slabs) % RING_SIZE;
        // Genode::log("ring head: ", ring_head[slab_level], " ring tail: ", ring_tail[slab_level]);
        // for (int i = 0; i < num_slabs; i++) { 
        //     Genode::log("ring buffer: ", ring_buffer[slab_level * RING_SIZE + ring_tail[slab_level] - i - 1]);
        // }
        return 0;
    }

    int Local_allocator::insert_hash(void* addr, int num_slabs) {
        Genode::log("inserting hash: ", addr, " num_slabs: ", num_slabs);
        unsigned long addr_base = (unsigned long)addr;
        int success = 0;
        int b = hash_bucket(addr_base, HASH_SIZE);
        for (int i = 0; i < HASH_CAPACITY; i++) {
            if (hash_keys[b * HASH_CAPACITY + i] == 0) {
                hash_keys[b * HASH_CAPACITY + i] = addr_base;
                pinned_mappings[b * HASH_CAPACITY + i] = num_slabs;
                success = 1;
                return 0;
            }
        }
        if (!success) {
            Genode::error("hash table full");
            return -1;
        }
        return 0;
    }

    int Local_allocator::reduce_hash(unsigned long addr) {
        unsigned long addr_base = (unsigned long)addr;
        int b = hash_bucket(addr_base, HASH_SIZE);
        int success = 0;
        for (int i = 0; i < HASH_CAPACITY; i++) {
            if (hash_keys[b * HASH_CAPACITY + i] == addr_base) {
                pinned_mappings[b * HASH_CAPACITY + i] -= 1;
                if (pinned_mappings[b * HASH_CAPACITY + i] == 0) {
                    hash_keys[b * HASH_CAPACITY + i] = 0;
                    free_atds((void *)addr_base);
                }
                success = 1;
            }
        }
        if (!success) {
            Genode::error("address not in hash table but trying to reduce");
            return -1;
        }
        return 0;
    }

    Local_allocator::Local_allocator(Genode::Env &env, MtsysMemory::Connection &mem_obj)
    :
    env(env),
    mem_obj(mem_obj)
    {
        // Genode::log("hash of 640: ", hash_bucket(640));
        hash_keys = new(sliced_heap) unsigned long[HASH_SIZE * HASH_CAPACITY];
        pinned_mappings = new(sliced_heap) int[HASH_SIZE * HASH_CAPACITY];
        ring_buffer = new(sliced_heap) unsigned long[RING_LEVEL * RING_SIZE];
        slab_base = new(sliced_heap) unsigned long[RING_LEVEL * RING_SIZE];
        pin_hash = new(sliced_heap) unsigned long[HASH_SIZE * PIN_CAPACITY];
        pin_addr = new(sliced_heap) unsigned long[HASH_SIZE * PIN_CAPACITY]; 
        dsaddr_hash = new(sliced_heap) unsigned long[HASH_SIZE * PIN_CAPACITY];
        capaddr_list = new(sliced_heap) MtsysMemory::LocalMemobj*[HASH_SIZE * PIN_CAPACITY];

        for (int i = 0; i < HASH_SIZE * HASH_CAPACITY; i++) {
            hash_keys[i] = 0;
            pinned_mappings[i] = 0;
        }
        for (int i = 0; i < RING_LEVEL * RING_SIZE; i++) {
            ring_buffer[i] = 0;
            slab_base[i] = 0;
        }
        for (int i = 0; i < HASH_SIZE * PIN_CAPACITY; i++) {
            pin_hash[i] = 0;
            pin_addr[i] = 0;
            dsaddr_hash[i] = 0;
            capaddr_list[i] = 0;
        }
    }

    Local_allocator::~Local_allocator()
    { 
        // note: genode did not implement delete[], so we have to free manually
        sliced_heap.free(hash_keys, HASH_SIZE * HASH_CAPACITY * sizeof(unsigned long));
        sliced_heap.free(pinned_mappings, HASH_SIZE * HASH_CAPACITY * sizeof(int));
        sliced_heap.free(ring_buffer, RING_LEVEL * RING_SIZE * sizeof(unsigned long));
        sliced_heap.free(slab_base, RING_LEVEL * RING_SIZE * sizeof(unsigned long));
        sliced_heap.free(pin_hash, HASH_SIZE * PIN_CAPACITY * sizeof(unsigned long));
        sliced_heap.free(pin_addr, HASH_SIZE * PIN_CAPACITY * sizeof(unsigned long));
        sliced_heap.free(dsaddr_hash, HASH_SIZE * PIN_CAPACITY * sizeof(unsigned long));
        sliced_heap.free(capaddr_list, HASH_SIZE * PIN_CAPACITY * sizeof(MtsysMemory::LocalMemobj*));
    }


    MtsysMemory::LocalMemobj* Local_allocator::alloc_ds(int size) {
        int ds_size = best_fit_ds(size);
        if (ds_size == 0) return 0;
        MtsysMemory::LocalMemobj *capaddr = new(sliced_heap) MtsysMemory::LocalMemobj();
        if (!MTSYS_OPTION_LCMEMORY){
            // use local dataspace
            Genode::Attached_ram_dataspace* ds;
            if (ds_size > 0){
                Genode::log("allocating dataspace ds_size: ", ds_size);
                ds = new(sliced_heap) Genode::Attached_ram_dataspace(env.ram(), env.rm(), ds_size);
            }
            else{
                Genode::log("allocating dataspace size: ", size);
                ds = new(sliced_heap) Genode::Attached_ram_dataspace(env.ram(), env.rm(), size);
                // Genode::log("allocated dataspace size: ", ds->size(), " addr: ", ds->local_addr<void>());
            }
            // Genode::log("allocated dataspace size: ", ds->size(), " addr: ", ds->local_addr<void>());
            Genode::Ram_dataspace_capability cap = ds->cap();
            capaddr->cap = &cap;
            capaddr->addr = (unsigned long)ds->local_addr<void>();
            capaddr->size = ds->size();
        }
        else{
            // use remote dataspace
            if (ds_size > 0){
                Genode::Ram_dataspace_capability cap = mem_obj.Memory_alloc(ds_size, capaddr->addr);
                capaddr->cap = &cap;
                capaddr->size = ds_size;
                // attach the dataspace
                env.rm().attach_at(*(capaddr->cap), capaddr->addr + REMOTE_MEMADDR, ds_size);
            }
            else{
                Genode::log("allocating single dataspace size: ", size);
                Genode::Ram_dataspace_capability cap = mem_obj.Memory_alloc(size, capaddr->addr);
                capaddr->cap = &cap;
                capaddr->size = size;
                // attach the dataspace
                env.rm().attach_at(*(capaddr->cap), capaddr->addr + REMOTE_MEMADDR, size);
            }
            capaddr->addr += REMOTE_MEMADDR;
        }
        return capaddr;
        // return mem_obj.alloc_ds(ds_size);
    }


    int Local_allocator::free_atds(void* addr) {
        Genode::log("freeing dataspace at addr: ", addr);
        if (!MTSYS_OPTION_LCMEMORY){            
            // now free the local dataspace
            env.rm().detach(addr);
            // find the cap in the hash table
            unsigned long addr_base = (unsigned long)addr;
            int b = hash_bucket(addr_base, HASH_SIZE);
            MtsysMemory::LocalMemobj* capaddr = 0;
            int success = 0;
            for (int i = 0; i < PIN_CAPACITY; i++) {
                if (dsaddr_hash[b * PIN_CAPACITY + i] == addr_base) {
                    capaddr = capaddr_list[b * PIN_CAPACITY + i];
                    dsaddr_hash[b * PIN_CAPACITY + i] = 0;
                    capaddr_list[b * PIN_CAPACITY + i] = 0;
                    success = 1;
                    break;
                }
            }
            if (!success) {
                Genode::error("address not in dsaddr hash but trying to free");
                return -1;
            }
            env.ram().free(*(capaddr->cap));
        }
        else{
            // free the remote dataspace
            mem_obj.Memory_free((Genode::addr_t)addr - REMOTE_MEMADDR);
            // env.rm().detach(addr + REMOTE_MEMADDR);
        }
        return 0;
    }


    Alloc_result Local_allocator::try_alloc(Genode::size_t size)
    {   
        if (size == 0) return Alloc_error(Alloc_error::DENIED);

        int slab_size = best_fit_slab(size);
        
        if (MTSYS_OPTION_LCMEMORY == 2){
            // only use remote memory
            MtsysMemory::LocalMemobj *capaddr = new(sliced_heap) MtsysMemory::LocalMemobj();
            Genode::log("allocating remote dataspace size: ", slab_size);
            Genode::Ram_dataspace_capability cap;
            if (slab_size > 0)
                cap = mem_obj.Memory_alloc(slab_size, capaddr->addr);
            else
                cap = mem_obj.Memory_alloc(size, capaddr->addr);
            capaddr->cap = &cap;
            capaddr->size = slab_size;
            Genode::log("attaching remote dataspace at addr: ", (void*)(capaddr->addr + REMOTE_MEMADDR));
            if (slab_size > 0)
                env.rm().attach_at(*(capaddr->cap), capaddr->addr + REMOTE_MEMADDR, slab_size);
            else
                env.rm().attach_at(*(capaddr->cap), capaddr->addr + REMOTE_MEMADDR, size);
            return (void *)(capaddr->addr + REMOTE_MEMADDR);
        }

        // Genode::log("allocating size: ", size);
        
        int slab_level = SLAB_SIZE2LEVEL(slab_size);
        // Genode::log("slab size: ", slab_size, " slab level: ", slab_level);
        if (slab_size < 0 || slab_level < 0 || slab_level >= SLAB_SIZE_LEVELS) {
            MtsysMemory::LocalMemobj* ds = alloc_ds(size);
            Genode::log("allocated dataspace size: ", ds->size, " addr: ", ds->addr, " cap ", *(ds->cap));
            if (ds->addr) {
                _quota_used += size;
                void* ret = (void*)(ds->addr);
                // insert into dsaddr hash
                int success = 0;
                int b = hash_bucket((unsigned long)ret, HASH_SIZE);
                for (int i = 0; i < PIN_CAPACITY; i++) {
                    if (dsaddr_hash[b * PIN_CAPACITY + i] == 0) {
                        dsaddr_hash[b * PIN_CAPACITY + i] = (unsigned long)ret;
                        capaddr_list[b * PIN_CAPACITY + i] = ds;
                        success = 1;
                        break;
                    }
                }
                if (!success) {
                    Genode::error("dsaddr hash full");
                    return Alloc_error(Alloc_error::DENIED);
                }
                return ret;
            }
            else {
                Genode::error("allocating dataspace failed");
                Alloc_error err(Alloc_error::DENIED);
                return err;
            }
        }
        if (ring_head[slab_level] != ring_tail[slab_level]) {
            // Genode::log("using ring buffer: ", ring_head[slab_level]);
            int target_id = slab_level * RING_SIZE + ring_head[slab_level];
            unsigned long addr = ring_buffer[target_id];
            ring_head[slab_level] = (ring_head[slab_level] + 1) % RING_SIZE;
            _quota_used += size;
            // record the pin address
            int b = hash_bucket(addr, HASH_SIZE);
            for (int i = 0; i < PIN_CAPACITY; i++) {
                if (pin_hash[b * PIN_CAPACITY + i] == 0) {
                    pin_hash[b * PIN_CAPACITY + i] = addr;
                    pin_addr[b * PIN_CAPACITY + i] = slab_base[target_id];
                    break;
                }
            }
            return (void *)addr;
        }
        else {
            // Genode::log("allocating new ds");
            MtsysMemory::LocalMemobj* ds = alloc_ds(size);
            Genode::log("allocated dataspace size: ", ds->size, " addr: ", ds->addr, " cap ", *(ds->cap));
            if (ds->addr) {
                _quota_used += size;
                // Genode::log("allocated dataspace size: ", ds->size(), " addr: ", ds->local_addr<void>());
                void* ret = (void*)(ds->addr);
                // insert into dsaddr hash
                int success = 0;
                int b = hash_bucket((unsigned long)ret, HASH_SIZE);
                for (int i = 0; i < PIN_CAPACITY; i++) {
                    if (dsaddr_hash[b * PIN_CAPACITY + i] == 0) {
                        dsaddr_hash[b * PIN_CAPACITY + i] = (unsigned long)ret;
                        capaddr_list[b * PIN_CAPACITY + i] = ds;
                        success = 1;
                        break;
                    }
                }
                if (!success) {
                    Genode::error("dsaddr hash full");
                    return Alloc_error(Alloc_error::DENIED);
                }
                // record the pin address
                for (int i = 0; i < PIN_CAPACITY; i++) {
                    if (pin_hash[b * PIN_CAPACITY + i] == 0) {
                        pin_hash[b * PIN_CAPACITY + i] = (unsigned long)ret;
                        pin_addr[b * PIN_CAPACITY + i] = (unsigned long)ret;
                        break;
                    }
                }
                Genode::size_t ds_size = ds->size;
                int num_slabs = (ds_size / slab_size) - 1;
                if (add_new_slab(ret, slab_size, num_slabs) == 0){
                    insert_hash(ret, num_slabs);
                }
                return ret;
            }
            else {
                Genode::error("allocating dataspace failed");
                Alloc_error err(Alloc_error::DENIED);
                return err;
            }
        }
        Genode::error("Unexpected allocating failed");
        Alloc_error err(Alloc_error::DENIED);
        return err;
    }

    void Local_allocator::free(void *addr, Genode::size_t)
    {
        if (addr == 0) return;

        if (MTSYS_OPTION_LCMEMORY == 2){
            // only use remote memory
            mem_obj.Memory_free((Genode::addr_t)addr - REMOTE_MEMADDR);
            return;
        }

        // find in the pin hash
        unsigned long addr_base = (unsigned long)addr;
        int b = hash_bucket(addr_base, HASH_SIZE);
        int success = 0;
        for (int i = 0; i < PIN_CAPACITY; i++) {
            if (pin_hash[b * PIN_CAPACITY + i] == addr_base) {
                addr_base = pin_addr[b * PIN_CAPACITY + i];
                success = 1;
                // clear the pin hash
                pin_hash[b * PIN_CAPACITY + i] = 0;
                pin_addr[b * PIN_CAPACITY + i] = 0;
                break;
            }
        }
        // reduce pinned mappings if found, otherwise deallocate the dataspace
        if (success) {
            Genode::log("freeing addr: ", addr, " base: ", addr_base);
            reduce_hash(addr_base);
            return;
        }
        else {
            free_atds(addr);
        }
        return;
    }

}


#endif
