// SPDX-License-Identifier: MulanPSL-2.0

/*
 * 分配器。
 *
 * 创建于 2023年7月2日 上海市嘉定区安亭镇
 */

// forked from yros stdlib.
// github.com/FlowerBlackG/YurongOS

// modified for Amkos

#pragma once

#include "./sys/types.h"
#include <base/allocator.h>

namespace adl {

#if 1

typedef Genode::Allocator Allocator;

#else

struct Allocator {
    void* (* alloc) (size_t) = nullptr;
    void (* free) (void* addr) = nullptr;
};

#endif

}
