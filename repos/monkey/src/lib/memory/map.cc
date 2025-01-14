/*
    Memory Map

    Created on 2025.1.14

    gongty  [at] tongji [dot] edu [dot] cn
    feng.yt [at]  sjtu  [dot] edu [dot] cn
    Jie Yingqi

*/


#include <base/env.h>
#include <monkey/memory/map.h>

namespace monkey::memory {

Status getMemoryMap(
    Genode::Env& env, 
    adl::ArrayList<MemoryMapEntry>& out,
    adl::uintptr_t from,
    adl::uintptr_t until
) {
    
    // `from` and `until` should align to 4K boundary.
    if ((from & 0xfff) || (until & 0xfff))
        return Status::INVALID_PARAMETERS;


    out.clear();
    if (from >= until)
        return Status::SUCCESS;

    auto availMem = env.pd().avail_ram().value;
    if (availMem < 128 * 1024) {  // n KB
        return Status::OUT_OF_RESOURCE;
    }

    Status status = Status::SUCCESS;

    
    // determine large prober size

    const adl::size_t LARGE_PROBE_SIZE_MAX = 64ull * 1024 * 1024;

    adl::size_t largeProberSize = LARGE_PROBE_SIZE_MAX;
    if (availMem < LARGE_PROBE_SIZE_MAX * 2) {
        auto size = availMem / 2;
        if (size < 4096)
            return Status::OUT_OF_RESOURCE;

#ifdef __GNUC__
        size = 1 << (sizeof(size) * 8 - 1 - unsigned(__builtin_clzl(size)) );
#else
        {
            adl::size_t mask = 0x1;
            while (mask < size) {
                mask <<= 1;
                mask |= 1;
            }

            size &= ( ~ mask ) >> 1;
        }
#endif

        largeProberSize = size;
    }


    if (largeProberSize > until - from)
        largeProberSize = until - from;


    // allocate probers

    auto largeProber = env.ram().alloc(largeProberSize);
    auto smallProber = env.ram().alloc(4096);


    // probe memory

    const auto addRecord = [&] (adl::uintptr_t addr, adl::size_t size, MemoryMapEntry::Type type) {
        if (out.isEmpty() || out.back().type != type || out.back().addr + out.back().size != addr) {
            out.append({
                .type = type,
                .addr = addr,
                .size = size
            });
        }
        else {
            out.back().size += size;
        }


    };

    adl::uintptr_t curr = from;
    bool carefulMode = false;  // whether using 4K prober.
    while (curr < until) {
        if (curr >= until - largeProberSize && !carefulMode) {
            carefulMode = true;
            continue;
        }

        auto& prober = carefulMode ? smallProber : largeProber;
        adl::size_t proberSize = carefulMode ? 4096 : largeProberSize;

        try {
            env.rm().attach_at(prober, curr);
            env.rm().detach(curr);
            // vaddr free (seems)
            addRecord(curr, proberSize, MemoryMapEntry::Type::FREE);
        } catch (...) {
            if (carefulMode) {
                // vaddr occupied.
                addRecord(curr, proberSize, MemoryMapEntry::Type::OCCUPIED);
            } 
            else {  // retry in careful mode.
                carefulMode = true;
                continue;
            }
        }

        curr += proberSize;

        if (carefulMode && (curr & (largeProberSize - 1)) == 0) {
            carefulMode = false;
        }
    }


    // cleanup
    
    env.ram().free(largeProber);
    env.ram().free(smallProber);
    return status;
}


}

