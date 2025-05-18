
#include <base/component.h>
#include <base/log.h>
#include <fs/filesys_ram.h>


#include <pivot/pivot_connection.h>


static int runMemoryTest(MtsysPivot::ServiceHub& hub) {
	// test allocation here
	void* p1 = hub.Memory_alloc(4096);
	Genode::log("Allocated memory at: ", p1);
	void* p2 = hub.Memory_alloc(4096);
	Genode::log("Allocated memory at: ", p2);
	void* p3 = hub.Memory_alloc(4096);
	Genode::log("Allocated memory at: ", p3);
	void* p4 = hub.Memory_alloc(40969);
	Genode::log("Allocated memory at: ", p4);
	// note here we need to enlarge cap in run file, as it is local obj
	void* p5 = hub.Memory_alloc(40960000);
	Genode::log("Allocated memory at: ", p5);
	void* p6 = hub.Memory_alloc(8);
	Genode::log("Allocated memory at: ", p6);

	((char*)p6)[0] = 'a';
	((char*)p6)[1] = 'b';
	((char*)p6)[2] = 'c';
	((char*)p6)[3] = 0;
	Genode::log("p6: ", (const char*)p6);

	// test free here
	hub.Memory_free(p1);
	hub.Memory_free(p2);
	hub.Memory_free(p3);
	hub.Memory_free(p4);
	hub.Memory_free(p5);
	hub.Memory_free(p6);

	return 0;
}


static int runFsTest(MtsysPivot::ServiceHub& hub) {
	// test ramfs here
	hub.Fs_hello();
	int fd1 = hub.Fs_open("/testfile", 
		MtfOpenMode::OPEN_MODE_CREATE | MtfOpenMode::OPEN_MODE_RDWR, 666);
	Genode::log("Opened file: ", fd1);
	hub.Fs_close(fd1);
	// test dir
	hub.Fs_mkdir("/testdir0", 666);
	hub.Fs_mkdir("/testdir1", 666);
	hub.Fs_rmdir("/testdir1");
	hub.Fs_unlink("/testfile");
	fd1 = hub.Fs_open("/testdir0/testfile0", 
		MtfOpenMode::OPEN_MODE_CREATE | MtfOpenMode::OPEN_MODE_RDWR, 666);
	// test write and read
	MtfStat stat;
	hub.Fs_fstat("/testdir0/testfile0", stat);
	Genode::log("File size: ", stat.size);
	char* content = "Hello, world!";
	hub.Fs_write(fd1, (const char*)content, 14);
	char* buf = (char*)(hub.Memory_alloc(16));
	hub.Fs_read(fd1, buf, 14);
	Genode::log("Read content: ", (const char*)buf);
	hub.Fs_close(fd1);
	hub.Fs_rename("/testdir0/testfile0", "/testdir0/testfile1");
	hub.Fs_fstat("/testdir0/testfile1", stat);
	Genode::log("File size: ", stat.size);
	// test truncate
	fd1 = hub.Fs_open("/testdir0/testfile1", MtfOpenMode::OPEN_MODE_RDWR, 666);
	hub.Fs_ftruncate(fd1, 6);
	hub.Fs_read(fd1, buf, 6);
	Genode::log("Read content: ", (const char*)buf);
	hub.Fs_fstat("/testdir0/testfile1", stat);
	Genode::log("File size: ", stat.size);
	hub.Fs_close(fd1);
	// test multiple read
	fd1 = hub.Fs_open("/testdir0/testfile1", MtfOpenMode::OPEN_MODE_RDWR, 666);
	hub.Fs_read(fd1, buf, 2);
	Genode::log("Read content: ", (const char*)buf);
	hub.Fs_read(fd1, buf, 3);
	Genode::log("Read content: ", (const char*)buf);
	hub.Fs_close(fd1);

	return 0;
}


static int runFsBench(MtsysPivot::ServiceHub& hub, int n) {
	// record start time
	Genode::log("\n\n =================== \n\n");
	Genode::log("FS bench: ", n, " ops");
	Genode::log("\n\n =================== \n\n");
	auto start = hub.Time_now_us().value;

	int fd1 = hub.Fs_open("/testfile", 
			MtfOpenMode::OPEN_MODE_CREATE | MtfOpenMode::OPEN_MODE_RDWR, 666);
	hub.Fs_close(fd1);
	for (int i = 0; i < n; i++) {
		fd1 = hub.Fs_open("/testfile", MtfOpenMode::OPEN_MODE_RDWR, 666);
		hub.Fs_write(fd1, "Hello, world!", 14);
		hub.Fs_ftruncate(fd1, 8);
		hub.Fs_close(fd1);
	}
	fd1 = hub.Fs_open("/testfile", MtfOpenMode::OPEN_MODE_RDWR, 666);
	hub.Fs_write(fd1, "Hello, world!", 14);
	char* buf = (char*)(hub.Memory_alloc(16));
	hub.Fs_read(fd1, buf, 14);
	Genode::log("Read content: ", (const char*)buf);
	hub.Fs_close(fd1);
	hub.Fs_unlink("/testfile");

	// record end time
	auto end = hub.Time_now_us().value;
	Genode::log("\n\n =================== \n\n");
	Genode::log("FS bench: ", n, " ops, time: ", end - start, " us");
	Genode::log("FS bench throughput: ", (float)n * 1000000 * 4 / (end - start), " ops/s");
	Genode::log("\n\n =================== \n\n");
	return 0;
}


static int runNullBench(MtsysPivot::ServiceHub& hub, int n) {
	// record start time
	Genode::log("\n\n =================== \n\n");
	Genode::log("Null bench: ", n, " ops");
	Genode::log("\n\n =================== \n\n");
	auto start = hub.Time_now_us().value;

	for (int i = 0; i < n; i++) {
		hub.null_function();
	}
	// record end time
	auto end = hub.Time_now_us().value;
	Genode::log("\n\n =================== \n\n");
	Genode::log("Null bench: ", n, " ops, time: ", end - start, " us");
	Genode::log("Null bench throughput: ", (float)n * 1000000 / (end - start), " ops/s");
	Genode::log("\n\n =================== \n\n");
	return 0;
}



void Component::construct(Genode::Env &env)
{	

	MtsysPivot::ServiceHub hub(env);

	hub.Pivot_hello();
	Genode::log("Pivot App ID: ", hub.Pivot_App_getid());

	hub.Kv_hello();

	hub.Memory_hello();
	Genode::log("free space: ", hub.query_free_space());

	runMemoryTest(hub);

	// runNullBench(hub, 100000);

	runFsTest(hub);

	runFsBench(hub, 10000);


	Genode::log("testapp completed");
}
