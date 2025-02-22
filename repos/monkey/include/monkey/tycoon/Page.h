/*
 * Tycoon Page
 *
 * Created on 2025.2.21 at Wujing, Minhang
 *
 * gongty
 */


#pragma once

#ifndef MONKEY_TYCOON_INCLUDE_INTERNALS
    #error "Do not include this file directly. Use #include <monkey/tycoon/Tycoon.h> instead."
#endif

#include <adl/sys/types.h>

namespace monkey::tycoon {
    struct Page {
        adl::uintptr_t addr = 0;  // aligned to 4KB.
        // size must be 4KB;

        bool present = false;
        bool dirty = false;
        bool readable = false;
        
        adl::int64_t mnemosyneId = 0;
        adl::int64_t blockId = 0;
    };
}
