/*
 * \brief  Access to the component's LOG session
 * \author Norman Feske
 * \date   2016-05-03
 */

/*
 * Copyright (C) 2016-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <util/construct_at.h>
#include <base/log.h>
#include <base/buffered_output.h>
#include <base/sleep.h>
#include <log_session/client.h>

/* base-internal includes */
#include <base/internal/globals.h>

using namespace Genode;


namespace {

	/**
	 * Singleton back end for writing messages to the component's log session
	 */
	struct Back_end : Noncopyable
	{
		Log_session_client _client;

		static Session_capability _cap(Parent &parent)
		{
			return parent.session_cap(Parent::Env::log()).convert<Capability<Session>>(
				[&] (Capability<Session> cap)   { return cap; },
				[&] (Parent::Session_cap_error) { return Capability<Session>(); });
		}

		Back_end(Parent &parent)
		: _client(reinterpret_cap_cast<Log_session>(_cap(parent))) { }

		void write(char const *string) { _client.write(string); }
	};
}

static Back_end *back_end_ptr;


/**
 * Singleton instance of the 'Log' interface
 */
static Log *log_ptr;

Log &Log::log()
{
	if (log_ptr)
		return *log_ptr;

	raw("Error: Missing call of init_log log");
	sleep_forever();
}


static Trace_output *trace_ptr;

Trace_output &Trace_output::trace_output()
{
	if (trace_ptr)
		return *trace_ptr;

	raw("Error: Missing call of init_log trace");
	sleep_forever();
}


/**
 * Hook for support the 'fork' implementation of the noux libc backend
 */
extern "C" void stdout_reconnect(Parent &parent)
{
	/*
	 * We cannot use a 'Reconstructible' because we have to skip
	 * the object destruction inside a freshly forked process.
	 * Otherwise, the attempt to destruct the capability contained
	 * in the 'Log' object would result in an inconsistent ref counter
	 * of the respective capability-space element.
	 */
	construct_at<Back_end>(back_end_ptr, parent);
}


void Genode::init_log(Parent &parent)
{
	/* ignore subsequent calls */
	if (log_ptr)
		return;

	static Back_end back_end { parent };
	back_end_ptr = &back_end;

	struct Write_fn {
		void operator () (char const *s) {
			if (Thread::trace_captured(s)) return;
			back_end_ptr->write(s);
		}
	};

	using Buffered_log_output = Buffered_output<Log_session::MAX_STRING_LEN, Write_fn>;

	static Buffered_log_output buffered_log_output { Write_fn() };
	static Log log { buffered_log_output };
	static Trace_output trace { };

	log_ptr   = &log;
	trace_ptr = &trace;
}
