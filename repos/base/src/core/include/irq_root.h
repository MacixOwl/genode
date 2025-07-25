/*
 * \brief  IRQ root interface
 * \author Christian Helmuth
 * \author Alexander Boettcher
 * \date   2007-09-13
 */

/*
 * Copyright (C) 2007-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CORE__INCLUDE__IRQ_ROOT_H_
#define _CORE__INCLUDE__IRQ_ROOT_H_

#include <root/component.h>

/* core includes */
#include <irq_session_component.h>

namespace Core { struct Irq_root; }


struct Core::Irq_root : Root_component<Irq_session_component>
{
	/*
	 * Use a dedicated entrypoint for IRQ session to decouple the interrupt
	 * handling from other core services. If we used the same entrypoint, a
	 * long-running operation (like allocating and clearing a dataspace
	 * from the RAM service) would delay the response to time-critical
	 * calls of the 'Irq_session::ack_irq' function.
	 */
	enum { STACK_SIZE = sizeof(long)*1024 };
	Rpc_entrypoint _session_ep;

	Range_allocator &_irq_alloc;    /* platform irq allocator */

	Create_result _create_session(const char *args) override
	{
		return _alloc_obj(_irq_alloc, args);
	}

	/**
	 * Constructor
	 *
	 * \param irq_alloc    IRQ range that can be assigned to clients
	 * \param md_alloc     meta-data allocator to be used by root component
	 */
	Irq_root(Range_allocator &irq_alloc, Allocator &md_alloc)
	:
		Root_component<Irq_session_component>(&_session_ep, &md_alloc),
		_session_ep(nullptr, STACK_SIZE, "irq", Affinity::Location()),
		_irq_alloc(irq_alloc)
	{ }
};

#endif /* _CORE__INCLUDE__IRQ_ROOT_H_ */
