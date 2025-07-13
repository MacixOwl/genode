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
#include <timer_session/connection.h>

const int PING_PONG_NUM = 1 << 16;

const int READ_BUF_SIZE = (1 << 15);

// We keep WRITE_QUEUE_DEPTH x WRITE_BUFFER_SIZE = READ_BUF_SIZE
const int WRITE_BUFFER_SIZE = 256;
const int WRITE_QUEUE_DEPTH = (READ_BUF_SIZE / WRITE_BUFFER_SIZE);

using namespace Genode;

static inline int wq_pos(int64_t id) {
	return (int)(id % WRITE_QUEUE_DEPTH) * WRITE_BUFFER_SIZE;}

struct Main
{	
	Genode::Env &env;
	Terminal::Connection terminal;
	Signal_handler<Main> read_avail;
	char read_buffer[READ_BUF_SIZE];
	char write_buffer[WRITE_QUEUE_DEPTH * WRITE_BUFFER_SIZE];
	int64_t writeID_head = 0;
	int64_t writeID_tail = 0;
	// int pingpong_time = PING_PONG_NUM;
	Timer::Connection _timer;

	// To verify Tx and Rx are one-to-one, we count them
	int64_t info_cnt_sum = 0;

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
				// 
				info_head += (info_len + 1);
				info_cnt += 1;
			}
			// terminal.write(&read_buffer[i], 1);
		}
		// read_buffer[carriage_index] = '\0';
		writeID_head += info_cnt;
		// Genode::log("info_cnt is ", info_cnt);
		info_cnt_sum += info_cnt;

		// if (pingpong_time > 0){
		// 	if (pingpong_time == PING_PONG_NUM) {
		// 		// Timer::Connection _timer(env);
		// 		Genode::Milliseconds time_beg = _timer.curr_time().trunc_to_plain_ms();
		// 		Genode::log("Client test start at ", time_beg);
		// 	}
		// 	terminal.write(read_buffer, carriage_index + 1);
		// 	pingpong_time -= 1;
		// }
		// else{
		// 	// Timer::Connection _timer(env);
		// 	Genode::Milliseconds time_end = _timer.curr_time().trunc_to_plain_ms();
		// 	Genode::log("Client test end at ", time_end);
		// }


		// Genode::log(carriage_index);
		// terminal.write("\n", 2);
		
		// Genode::log((const char*)read_buffer);
		return;
	}

	size_t Send2Server(const char* info, size_t size){
		// this function produce an entry in write buffer

		// Buffering strategy #1
		// while (writeID_tail - writeID_head >= WRITE_QUEUE_DEPTH - 1){
		// 	// Write Queue Pending...
		// 	handle_read_avail();
		// };

		// Buffering strategy #2
		if (writeID_tail - writeID_head >= WRITE_QUEUE_DEPTH - 1){
			while (writeID_tail - writeID_head >= WRITE_QUEUE_DEPTH / 2){
				// Write Queue Pending...
				handle_read_avail();
			};
		}

		Genode::memcpy(write_buffer + wq_pos(writeID_tail), info, size);
		terminal.write(write_buffer + wq_pos(writeID_tail), size);
		// Genode::log("Send2Server(B): ", size);
		writeID_tail += 1;
		return size;
	}

	Main(Env &env)
				: env(env),
				terminal(env),
	            read_avail(env.ep(), *this, &Main::handle_read_avail),
				_timer(env)
	{	
		// Timer::Connection _timer(env);

		terminal.read_avail_sigh(read_avail);
		// terminal.write(intro.string(), intro.length() + 1);
		// terminal.write("123456789\n", 10);

		Genode::log(intro.string());
		Genode::log(" len = ", intro.length());

		// Genode::Milliseconds time_beg = _timer.curr_time().trunc_to_plain_ms();
		// Genode::log("Client test start at ", time_beg);

		// for (int i = 0; i < 20; ++i){
		// 	terminal.read(read_buffer, READ_BUFFER_SIZE);
		// 	terminal.write(str128B.string(), str128B.length());
		// }

		// for (int i = 0; i < PING_PONG_NUM; ++i)
		// 	Send2Server(str128B.string(), str128B.length());

		// Genode::Milliseconds time_end = _timer.curr_time().trunc_to_plain_ms();
		// Genode::log("Client test finish at ", time_end);
	}
};

void Component::construct(Env &env) { 
	String<511> str512B {
"111111111111111111111111111111111111111111111111111111111111111\
1111111111111111111111111111111111111111111111111111111111111111\
1111111111111111111111111111111111111111111111111111111111111111\
1111111111111111111111111111111111111111111111111111111111111111\
1111111111111111111111111111111111111111111111111111111111111111\
1111111111111111111111111111111111111111111111111111111111111111\
1111111111111111111111111111111111111111111111111111111111111111\
111111111111111111111111111111111111111111111111111111111111111" };
	String<255> str256B {
"111111111111111111111111111111111111111111111111111111111111111\
1111111111111111111111111111111111111111111111111111111111111111\
1111111111111111111111111111111111111111111111111111111111111111\
111111111111111111111111111111111111111111111111111111111111111" };
	String<255> str128B {
"111111111111111111111111111111111111111111111111111111111111111\
111111111111111111111111111111111111111111111111111111111111111" };
	String<255> str64B {
"11111111111111111111111111111111111111111111111111111111111111" };
	static Main main(env); 
	// main.Send2Server("11111111", 9);

	Timer::Connection _timer(env);

	Genode::Milliseconds time_beg = _timer.curr_time().trunc_to_plain_ms();
	Genode::log("Client test start at ", time_beg);

	for (int i = 0; i < PING_PONG_NUM; ++i)
		main.Send2Server(str128B.string(), str128B.length());
	
	// Genode::Milliseconds time_end = _timer.curr_time().trunc_to_plain_ms();
	// Genode::log("Client test finish at ", time_end);

	// these lines drain the RX packets, maybe added to member function 
	Genode::log("Tx cnt: ", PING_PONG_NUM, " Rx cnt: ", main.info_cnt_sum);
	while (PING_PONG_NUM > main.info_cnt_sum){
		main.handle_read_avail();
		// Genode::log("Tx cnt: ", PING_PONG_NUM, " Rx cnt: ", main.info_cnt_sum);
	}
	Genode::log("Tx cnt: ", PING_PONG_NUM, " Rx cnt: ", main.info_cnt_sum);

	Genode::Milliseconds time_end = _timer.curr_time().trunc_to_plain_ms();
	Genode::log("Client test finish at ", time_end);

	float thput = (float)(time_end.value - time_beg.value);
	thput = (float)PING_PONG_NUM / thput * (float)1000;
	Genode::log("TCP send/recv rate is ", thput, " op/s.");
}
