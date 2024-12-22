
#include <base/component.h>
#include <base/log.h>
#include <fs/filesys_ram.h>


#include <pivot/pivot_connection.h>

void Component::construct(Genode::Env &env)
{	

	MtsysPivot::ServiceHub hub(env);

	hub.Pivot_hello();
	Genode::log("Pivot App ID: ", hub.Pivot_App_getid());

	hub.Kv_hello();

	hub.Memory_hello();
	Genode::log("free space: ", hub.query_free_space());

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

	// test IPC hotness 
	for (int i = 0; i < 100000; i++) {
		hub.null_function();
	}

	// test ramfs here
	hub.Fs_hello();
	int fd1 = hub.Fs_open("testfile", 
		MtfOpenMode::OPEN_MODE_CREATE | MtfOpenMode::OPEN_MODE_RDWR, 0);
	Genode::log("Opened file: ", fd1);
	hub.Fs_close(fd1);

	// Genode::Heap heap(env.ram(), env.rm());
	// int a = -1;

	// Genode::warning("Creating ramfs");
	// Mfs::Ram_file_system ramfs(env, heap);
	// Genode::log("Created ramfs");
	// Mfs::Vfs_handle* handle0 = nullptr;
	// a = ramfs.opendir("/testdir", 1, &handle0, heap);
	// Genode::log("Opened dir: ", a);
	// // if not closing at end, will cause dangling memory
	// ramfs.close(handle0);
	// Mfs::Vfs_handle* handle1 = nullptr;
	// a = ramfs.open("/testdir/testfile1", 
	// 	Vfs::Directory_service::Open_mode::OPEN_MODE_CREATE | Vfs::Directory_service::Open_mode::OPEN_MODE_RDWR,
	// 	&handle1, heap);
	// Genode::log("Opened file: ", a);
	// Genode::size_t size = 0;
	// a = ramfs.write(handle1, Genode::Const_byte_range_ptr((char*)"hello world", 11), size);
	// Genode::log("Wrote file: ", a);
	// ramfs.close(handle1);
	// Mfs::Vfs_handle* handle2 = nullptr;
	// a = ramfs.open("/testdir/testfile2", 
	// 	Vfs::Directory_service::Open_mode::OPEN_MODE_CREATE | Vfs::Directory_service::Open_mode::OPEN_MODE_RDWR,
	// 	&handle2, heap);
	// Genode::log("Opened file: ", a);
	// a = ramfs.write(handle2, Genode::Const_byte_range_ptr((char*)"aloha world", 11), size);
	// Genode::log("Wrote file: ", a);
	// ramfs.close(handle2);
	// Mfs::Vfs_handle* handle3 = nullptr;
	// a = ramfs.open("/testdir/testfile2", 
	// 	Vfs::Directory_service::Open_mode::OPEN_MODE_RDONLY,
	// 	&handle3, heap);
	// Genode::log("Opened file: ", a);
	// char* buf = (char*)heap.alloc(64);
	// Genode::size_t read_size = 0;
	// a = ramfs.complete_read(handle3, Genode::Byte_range_ptr(buf, 64), read_size);
	// Genode::log("Read file: ", a);
	// ramfs.close(handle3);
	// Genode::log("Read result: ", (const char*)buf);
	// heap.free(buf, 64);

	Genode::log("testapp completed");
}
