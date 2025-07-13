/*
 * \brief  Test client for our RPC interface
 * \author Björn Döbel
 * \author Norman Feske
 * \author Minyi
 * \author Zhenlin
 * \date   2023-09
 */

/*
 * Copyright (C) 2008-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */
#include <base/allocator_avl.h>
#include <base/attached_rom_dataspace.h>
#include <base/component.h>
#include <base/log.h>
#include <block_session/connection.h>
#include <base/attached_ram_dataspace.h>
#include <timer_session/connection.h>
#include <base/heap.h>
#include <base/allocator_avl.h>

/*
目前探索的Block使用流程：
1. 使用Block::Connection<Job>建立会话
2. 在默认模版基础上定义Job
3. 定义Policy，即read、write、completed三个方法
4. 使用Block::Operation创建Job，再调用update_jobs方法提交Job

*/


namespace Block_demo
{
	struct Job;
	struct update_policy;
	struct Main;
}

//Block操作的作业，每个job对应一个operation，具体可看Block::Connection
struct Block_demo::Job : Block::Connection<Job>::Job
{
	unsigned const id;
	Job(Block::Connection<Job> &connection, Block::Operation operation, unsigned id)
		: Block::Connection<Job>::Job(connection, operation), id(id)
	{
	}
};

//job对应的接口，由用户自己实现，需包含produce_write_content、consume_read_result、completed三个method，具体参数含义可看Block::Connection
struct Block_demo::update_policy
{
	char *write_ptr;
	char *read_ptr;
	Genode::Heap &heap;
	update_policy(char *_write_ptr, char *_read_ptr, Genode::Heap &_heap) : write_ptr(_write_ptr), read_ptr(_read_ptr), heap(_heap){}
	void produce_write_content(Job &job, Block::seek_off_t offset, char *dst, Genode::size_t length)
	{
		for (Genode::size_t i = 0; i < length; i++)
		{
			dst[i] = write_ptr[i];
		}
		Genode::log("job ", job.id, " write ", length, " bytes at ", offset);
	}
	void consume_read_result(Job &job, Block::seek_off_t offset, char const *src, Genode::size_t length)
	{
		for (Genode::size_t i = 0; i < length; i++)
		{
			read_ptr[i] = src[i];
		}
		Genode::log("job ", job.id, " read ", length, " bytes at ", offset);
	}
	void completed(Job &job, bool success)
	{
		if (!success)
		{
			Genode::log("job ", job.id, ": failed");
		}
		else
		{
			Genode::log("job ", job.id, ": completed");
		}
		destroy(heap, &job);
	}
};

struct Block_demo::Main
{
	Genode::Env &_env;

	Genode::Constructible<Block::Connection<Job>> &_block;

	update_policy &_policy;

	Genode::Heap &_heap;


	Block::Session::Info info {};

	void handle_io()
	{
		_block->update_jobs(_policy);
	};

	Genode::Signal_handler<Main> _block_io_sigh{_env.ep(), *this, &Main::handle_io};

	Main(Genode::Env &env, Genode::Constructible<Block::Connection<Job>> &block, update_policy &policy, Genode::Heap &heap) : _env(env), _block(block), _policy(policy), _heap(heap)
	{
		Timer::Connection timer(env);
		Genode::log("current time is ", timer.curr_time().trunc_to_plain_ms());

		//这一行删了也能运行，还不知道怎么用
		block->sigh(_block_io_sigh);

		//这里的测试：往第一个块里写入512个1，往第二个块里写入512个2，然后读取这两个块的内容
		/*
		结果：第二个写入操作并没有写入到第二个块，仍然往第一个块里写入。并且读操作没有执行。（确实有写入，可以查看bin/test.img中的内容
			猜测是没有收到完成的signal，completed()应该被调用却没被调用。
			另外update_jobs()会调用connection.h中_try_process_ack，感觉tx.ack_avail()出了问题，返回了false，参考了os/src/app/block_tester也不知如何解决。
		*/
		char *write_ptr = policy.write_ptr;
		for (int i = 0; i < 512; i++) 
		{
			write_ptr[i] = '1';
		}
		//定义Job
		Block::Operation op_write1{.type = Block::Operation::Type::WRITE, //read or write
								   .block_number = 1, //开始操作的块的号
								   .count = 1}; //操作的块的数量
		//创建Job，构造函数会将job加入待处理队列中，第三个参数是Job id
		new (heap) Job(*block, op_write1, 1);
		handle_io();

		for (int i = 0; i < 512; i++)
		{
			write_ptr[i] = '2';
		}
		Block::Operation op_write2{.type = Block::Operation::Type::WRITE,
								   .block_number = 2,
								   .count = 1};
		new (heap) Job(*block, op_write2, 2);
		handle_io();
		
		Block::Operation op_read1{.type = Block::Operation::Type::READ,
								  .block_number = 1,
								  .count = 1};
		new (heap) Job(*block, op_read1, 3);

		Block::Operation op_read2{.type = Block::Operation::Type::READ,
								  .block_number = 2,
								  .count = 1};
		new (heap) Job(*block, op_read2, 4);

		//调用update_jobs()，会逐个处理队列中的job
		handle_io();
		Genode::log("current time is ", timer.curr_time().trunc_to_plain_ms());

		while (true)
		{
			/* code */
		}

		Genode::log("mission completed");
	}
};



void Component::construct(Genode::Env &env)
{
	Genode::Heap heap{env.ram(), env.rm()};
	Genode::Constructible<Block::Connection<Block_demo::Job>> block{};
	Genode::Allocator_avl _block_alloc{&heap};

	//第三个参数是transmission buffer，默认128*1024
	block.construct(env, &_block_alloc, 36 * 1024 * 1024);


	char *write_ptr = (char *)heap.alloc((4 * 1024 * 1024 + 1) * sizeof(char));
	char *read_ptr = (char *)heap.alloc((4 * 1024 * 1024 + 1) * sizeof(char));

	Block_demo::update_policy policy{write_ptr, read_ptr, heap};

	struct Block_demo::Main main(env, block, policy, heap);
}
