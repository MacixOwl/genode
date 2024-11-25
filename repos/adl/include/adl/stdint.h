// SPDX-License-Identifier: MulanPSL-2.0

/*
 * 整数类型头文件。
 * 创建于 2022年7月2日。
 */

#pragma once

// forked from yros stdlib.
// github.com/FlowerBlackG/YurongOS

// modified for Amkos

namespace adl {

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

// 拥有最大宽度的整数类型。
typedef int64_t intmax_t;
typedef uint64_t uintmax_t;

#ifndef intptr_t
typedef int64_t intptr_t;
#endif

#ifndef uintptr_t
typedef uint64_t uintptr_t;
#endif

}


#ifndef uint32_t
using uint32_t = adl::uint32_t;
#endif


#ifndef uint8_t
using uint8_t = adl::uint8_t;
#endif

#ifndef int8_t
using int8_t = adl::int8_t;
#endif

#ifndef int16_t
using int16_t = adl::int16_t;
#endif


#ifndef uint16_t
using uint16_t = adl::uint16_t;
#endif


#ifndef uint64_t
using uint64_t = adl::uint64_t;
#endif

