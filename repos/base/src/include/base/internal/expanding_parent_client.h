/*
 * \brief  Parent client that issues resource requests on demand
 * \author Norman Feske
 * \date   2013-09-25
 */

/*
 * Copyright (C) 2006-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__BASE__INTERNAL__EXPANDING_PARENT_CLIENT_H_
#define _INCLUDE__BASE__INTERNAL__EXPANDING_PARENT_CLIENT_H_

/* Genode includes */
#include <base/signal.h>
#include <base/sleep.h>
#include <util/arg_string.h>
#include <util/retry.h>
#include <parent/client.h>

/* base-internal includes */
#include <base/internal/upgradeable_client.h>

namespace Genode { class Expanding_parent_client; }


class Genode::Expanding_parent_client : public Parent_client
{
	private:

		/**
		 * Signal handler state
		 *
		 * UNDEFINED        - No signal handler is effective. If we issue a
		 *                    resource request, use our built-in fallback
		 *                    signal handler.
		 * BLOCKING_DEFAULT - The fallback signal handler is effective.
		 *                    When using this handler, we block for a
		 *                    for a response to a resource request.
		 * CUSTOM           - A custom signal handler was registered. Calls
		 *                    of 'resource_request' won't block.
		 */
		enum State { UNDEFINED, BLOCKING_DEFAULT, CUSTOM };
		State _state = { UNDEFINED };

		/**
		 * Mutex used to serialize resource requests
		 */
		Mutex _mutex { };

		struct Io_signal_context : Signal_context
		{
			Io_signal_context()
			{ Signal_context::_level = Signal_context::Level::Io; }
		};

		/**
		 * Signal context for the fallback signal handler
		 */
		Io_signal_context _fallback_sig_ctx { };

		/**
		 * Signal context capability for the fallback signal handler
		 */
		Signal_context_capability _fallback_sig_cap { };

		/**
		 * Signal receiver for the fallback signal handler
		 */
		Constructible<Signal_receiver> _fallback_sig_rcv { };

		/**
		 * Block for resource response arriving at the fallback signal handler
		 */
		void _wait_for_resource_response() {
			_fallback_sig_rcv->wait_for_signal(); }

	public:

		Expanding_parent_client(Parent_capability cap) : Parent_client(cap) { }


		/**
		 * Downstreamed construction of the fallback signaling, used
		 * when the environment is ready to construct a signal receiver
		 */
		void init_fallback_signal_handling()
		{
			if (!_fallback_sig_cap.valid()) {
				_fallback_sig_rcv.construct();
				_fallback_sig_cap = _fallback_sig_rcv->manage(_fallback_sig_ctx);
			}
		}


		/**********************
		 ** Parent interface **
		 **********************/

		void exit(int exit_value) override
		{
			Parent_client::exit(exit_value);
			sleep_forever();
		}

		Session_result session(Client::Id          id,
		                       Service_name const &name,
		                       Session_args const &args,
		                       Affinity     const &affinity) override
		{
			return Parent_client::session(id, name, args, affinity);
		}

		Upgrade_result upgrade(Client::Id id, Upgrade_args const &args) override
		{
			/*
			 * Upgrades from our PD to our own PD session are futile. The only
			 * thing we can do when our PD is drained is requesting further
			 * resources from our parent.
			 */
			if (id == Env::pd()) {
				resource_request(Resource_args(args.string()));
				return Upgrade_result::OK;
			}

			/*
			 * If the upgrade fails, attempt to issue a resource request
			 */
			Session::Resources const amount = session_resources_from_args(args.string());
			using Arg = String<64>;

			for (;;) {
				Upgrade_result const result = Parent_client::upgrade(id, args);
				if (result == Upgrade_result::OUT_OF_RAM) {
					Arg ram_arg("ram_quota=", amount.ram_quota);
					resource_request(Resource_args(ram_arg.string()));
				}
				else if (result == Upgrade_result::OUT_OF_CAPS) {
					Arg cap_arg("cap_quota=", amount.cap_quota);
					resource_request(Resource_args(cap_arg.string()));
				}
				else
					return result;
			}
		}

		void resource_avail_sigh(Signal_context_capability sigh) override
		{
			Mutex::Guard guard(_mutex);

			/*
			 * If signal hander gets de-installed, let the next call of
			 * 'resource_request' install the fallback signal handler.
			 */
			if (_state == CUSTOM && !sigh.valid())
				_state = UNDEFINED;

			/*
			 * Forward information about a custom signal handler and remember
			 * state to avoid blocking in 'resource_request'.
			 */
			if (sigh.valid()) {
				_state = CUSTOM;
				Parent_client::resource_avail_sigh(sigh);
			}
		}

		void resource_request(Resource_args const &args) override
		{
			Mutex::Guard guard(_mutex);

			/*
			 * Issue request but don't block if a custom signal handler is
			 * installed.
			 */
			if (_state == CUSTOM) {
				Parent_client::resource_request(args);
				return;
			}

			/*
			 * Install fallback signal handler not yet installed.
			 */
			if (_state == UNDEFINED) {
				Parent_client::resource_avail_sigh(_fallback_sig_cap);
				_state = BLOCKING_DEFAULT;
			}

			/*
			 * Issue resource request
			 */
			Parent_client::resource_request(args);

			/*
			 * Block until we get a response for the outstanding resource
			 * request.
			 */
			if (_state == BLOCKING_DEFAULT)
				_wait_for_resource_response();
		}
};

#endif /* _INCLUDE__BASE__INTERNAL__EXPANDING_PARENT_CLIENT_H_ */
