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


struct Allocator {
public:
    typedef void* (* AllocFunc) (size_t, void*);
    typedef void (* FreeFunc) (void*, size_t, void*);

protected:
    void* data = nullptr;
    AllocFunc _alloc = nullptr;
    FreeFunc _free = nullptr;

public:
    inline void* alloc(size_t size) { return this->_alloc(size, data); }
    inline void free(void* addr, size_t size) { this->_free(addr, size, data); }

    inline bool ready() { return !!_alloc && !!_free; }
    inline bool notReady() { return !ready(); }

    inline void bindAlloc(AllocFunc func) { this->_alloc = func; };
    inline void bindFree(FreeFunc func) { this->_free = func; };
    inline void bindData(void* data) { this->data = data; };


    void init(AllocFunc alloc, FreeFunc free, void* data = nullptr) {
        bindAlloc(alloc);
        bindFree(free);
        bindData(data);
    }

    struct InitParams {
        AllocFunc alloc;
        FreeFunc free;
        void* data = nullptr;
    };

    inline void init(InitParams x) { init(x.alloc, x.free, x.data); }

    inline void operator = (InitParams x) { init(x); }

};




}  // namespace adl


inline void* operator new (adl::size_t size, adl::Allocator* allocator) {
    return allocator->alloc(size);
}

inline void* operator new [] (adl::size_t size, adl::Allocator* allocator) {
    return allocator->alloc(size);
}

inline void* operator new (adl::size_t size, adl::Allocator& allocator) {
    return allocator.alloc(size);
}

inline void* operator new [] (adl::size_t size, adl::Allocator& allocator) {
    return allocator.alloc(size);
}

