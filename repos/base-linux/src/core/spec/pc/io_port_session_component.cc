/*
 * \brief  Linux-specific IO_PORT service
 * \author Johannes Kliemann
 * \date   2019-11-25
 */

/*
 * Copyright (C) 2006-2019 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <util/arg_string.h>
#include <io_port_session_component.h>

using namespace Core;


Io_port_session_component::Io_port_session_component(Range_allocator &io_port_alloc,
                                                     const char *args)
:
	_io_port_range(io_port_alloc.alloc_addr(
		uint16_t(Arg_string::find_arg(args, "io_port_size").ulong_value(0)),
		uint16_t(Arg_string::find_arg(args, "io_port_base").ulong_value(0))))
{ }
