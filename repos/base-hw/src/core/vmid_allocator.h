/*
 * \brief  VM ID allocator
 * \author Stefan Kalkowski
 * \author Benjamin Lamowski
 * \date   2024-11-21
 */

/*
 * Copyright (C) 2015-2024 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CORE__VMID_ALLOCATOR_H_
#define _CORE__VMID_ALLOCATOR_H_

#include <hw/assert.h>
#include <util/bit_allocator.h>

namespace Core { struct Vmid_allocator; }

struct Core::Vmid_allocator
:	Genode::Bit_allocator<256>
{
	Vmid_allocator()
	{
		/* reserve VM ID 0 for the hypervisor */
		alloc().with_result([] (addr_t id)             { assert(id == 0); },
		                    [] (Vmid_allocator::Error) { assert(false); });
	}
};

#endif /* _CORE__VMID_ALLOCATOR_H_ */
