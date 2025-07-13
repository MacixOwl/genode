/*
 * \brief  Terminal echo program
 * \author Norman Feske
 * \author Martin Stein
 * \date   2009-10-16
 */

/*
 * Copyright (C) 2009-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/component.h>
#include <terminal_session/connection.h>

using namespace Genode;

const int READ_BUF_SIZE = (1 << 15);

struct Main
{
	Terminal::Connection terminal;
	Signal_handler<Main> read_avail;
	char read_buffer[READ_BUF_SIZE];

	String<128> intro {
"--- Terminal echo test started, now you can type characters to be echoed. ---\n" };

	void handle_read_avail()
	{	
		size_t num_bytes = terminal.read(read_buffer, READ_BUF_SIZE);
		// log("got ", num_bytes, " byte(s)");
		size_t info_cnt = 0;
		size_t info_head = 0;
		size_t info_tail = 0;
		size_t info_len = 0;
		for (; info_tail < num_bytes; info_tail++) {
			if (read_buffer[info_tail] == '\0') {
				info_len = info_tail - info_head;
				// terminal.write("\n", 1); 
				// Genode::log("info_len is ", info_len);
				// WE CAN PROCESS THE BACK INFO HERE
				terminal.write(read_buffer + info_head, info_len + 1);
				info_head = info_tail + 1;
				info_cnt += 1;
			}
			// terminal.write(&read_buffer[i], 1);
		}
		// read_buffer[carriage_index] = '\0';

		// Genode::log("info_cnt is ", info_cnt);

		// terminal.write(read_buffer, carriage_index);
		// Genode::log(carriage_index);
		// terminal.write("\n", 2);
	}

	Main(Env &env) : terminal(env),
	                 read_avail(env.ep(), *this, &Main::handle_read_avail)
	{
		terminal.read_avail_sigh(read_avail);
		// terminal.write(intro.string(), intro.length() + 1);
		// terminal.write("123456789\n", 10);
	}
};

void Component::construct(Env &env) { static Main main(env); }
