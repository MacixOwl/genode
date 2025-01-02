// SPDX-License-Identifier: MulanPSL-2.0

/*
 * Array List Structure
 *   by gongty [AT] tongji [DOT] edu [DOT] cn
 *
 * Warning!! Heavy Data Structure:
 *   This structure heavily depends on template, each usage could make your kernel larger.
 *
 *
 * created on 2023.7.3 at Anting Town, Jiading, Shanghai
 */

/*
 * This data structure is forked from YurongOS, modified for Amkos.
 *
 * https://github.com/FlowerBlackG/YurongOS/blob/master/src/lib/collections/ArrayList.hpp
 */

#pragma once

#include <base/stdint.h>
#include <base/allocator.h>
#include "../string.h"
#include "../config.h"
#include "../stdint.h"

namespace adl {


template<typename DataType>
class ArrayListIterator {
    typedef ArrayListIterator Self;

protected:
    size_t size;
    size_t curr;
    DataType* data;

public:
    ArrayListIterator(DataType* data, size_t size, size_t curr) {
        this->size = size;
        this->data = data;
        this->curr = curr;
    }

    DataType& operator * () const { return data[curr]; };
    Self& operator ++ () {
        curr++;
        return *this;
    }

    Self operator ++ (int) {
        auto tmp = *this;
        curr++;
        return tmp;
    }

    friend bool operator == (const Self& a, const Self& b) {
        return a.curr == b.curr
            && a.size == b.size
            && a.data == b.data;
    }

    friend bool operator != (const Self& a, const Self& b) {
        return a.curr != b.curr
            || a.size != b.size
            || a.data != b.data;
    }

};


template<typename DataType>
class ArrayList {

protected:
    size_t _size = 0;
    size_t _capacity = 0;
    DataType* _data = nullptr;
    Allocator* allocator;

public:
    ArrayList(Allocator& allocator = defaultAllocator) {
        this->allocator = &allocator;
        if (&allocator == nullptr) {
            Genode::error("[CRITICAL] ArrayList's allocator is null!");
            if (&allocator == &defaultAllocator) {
                Genode::error("> Using default allocator, but it is null.");
            }
            return;
        }
    }

    void clear() {
        _size = 0;
    }

    ~ArrayList() {
        if (_capacity)
            allocator->free(_data);
    }

    inline size_t size() const {
        return this->_size;
    }

    inline size_t capacity() const {
        return this->_capacity;
    }

    inline DataType* data() {
        return this->_data;
    }

    inline const DataType* data() const {
        return this->_data;
    }

    void reserve(size_t new_capacity) {
        if (new_capacity <= _capacity) {
            // Do nothing
            return;
        }
        if (!this->allocator) { // Check null ptr
            Genode::error("[CRITICAL] Allocator is null!");
            return;
        }
        // Allocate new memory
        auto newAddr = allocator->alloc<DataType>(new_capacity);
        Genode::log("After allocate");
        if (!newAddr) {
            Genode::error("[CRITICAL] ArrayList reserve failed: insufficient memory.");
            return;
        }
        // Copy the original data to new memory
        adl::memcpy(newAddr, _data, _size * sizeof(DataType));
        // Free old memory
        if (_capacity) {
            allocator->free(_data);
        }
        // Update ptr and capacity
        _data = newAddr;
        _capacity = new_capacity;
    }

    int append(const DataType& data) {

        if (_size == _capacity) {
            // alloc more memory
            size_t newCapacity = _capacity;
            if (newCapacity == 0) {
                newCapacity = 16;
            } else {
                newCapacity += newCapacity / 2;
            }

            auto newAddr = allocator->alloc<DataType>(newCapacity);
            if (!newAddr) {
                return 1; // error
            }

            adl::memcpy(newAddr, _data, _capacity * sizeof(DataType));

            if (_capacity)
                allocator->free(_data);

            _data = newAddr;
            _capacity = newCapacity;
        }

        _data[_size++] = data;

        return 0; // inserted successfully
    }

    inline int push(const DataType& data) {
        return this->append(data);
    }

    DataType pop() {
        if (_size) {
            return _data[--_size]; // todo: should call destructor
        }

        return * ( DataType* ) 0;
    }

    DataType& operator [] (size_t idx) {
        return _data[idx];
    }

    const DataType& operator [] (size_t idx) const {
        return _data[idx];
    }

    // ------ iteration related ------
    ArrayListIterator<DataType> begin() const {
        return ArrayListIterator<DataType>(_data, _size, 0);
    }

    ArrayListIterator<DataType> end() const {
        return ArrayListIterator<DataType>(_data, _size, _size);
    }

};


}  // namespace adl
