/*
 * \brief  Fixed-width integer types for 32-bit architectures
 * \author Christian Helmuth
 * \author Norman Feske
 * \date   2006-05-10
 *
 * In contrast to most Genode header files, which are only usable for C++,
 * this file is a valid C header file because a lot of existing C-based
 * software relies on fixed-size integer types. These types, however, are
 * platform specific but cannot be derived from the compiler's built-in
 * types. Normally, the platform-dependent part of a C library provides
 * these type definitions. This header file provides a single header for
 * C and C++ programs that are not using a C library but need fixed-width
 * integer types.
 *
 * All type definition are prefixed with 'genode_'. If included as a C++
 * header, the types are also defined within the 'Genode' namespace.
 */

/*
 * Copyright (C) 2006-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__SPEC__32BIT__BASE__FIXED_STDINT_H_
#define _INCLUDE__SPEC__32BIT__BASE__FIXED_STDINT_H_


/**********************************************************
 ** Fixed-size types usable from both C and C++ programs **
 **********************************************************/

typedef __INT8_TYPE__   genode_int8_t;
typedef __UINT8_TYPE__  genode_uint8_t;
typedef __INT16_TYPE__  genode_int16_t;
typedef __UINT16_TYPE__ genode_uint16_t;
typedef __INT32_TYPE__  genode_int32_t;
typedef __UINT32_TYPE__ genode_uint32_t;
typedef __INT64_TYPE__  genode_int64_t;
typedef __UINT64_TYPE__ genode_uint64_t;


/**************************************************
 ** Types residing within Genode's C++ namespace **
 **************************************************/

#ifdef __cplusplus
namespace Genode {
	typedef genode_int8_t     int8_t;
	typedef genode_uint8_t   uint8_t;
	typedef genode_int16_t   int16_t;
	typedef genode_uint16_t uint16_t;
	typedef genode_int32_t   int32_t;
	typedef genode_uint32_t uint32_t;
	typedef genode_int64_t   int64_t;
	typedef genode_uint64_t uint64_t;
}
#endif

#endif /* _INCLUDE__SPEC__32BIT__BASE__FIXED_STDINT_H_ */
