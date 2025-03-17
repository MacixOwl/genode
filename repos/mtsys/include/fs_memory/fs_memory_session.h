// This is the combination of fs session and memory session

#ifndef _INCLUDE__FS_MEMORY_SESSION_H_
#define _INCLUDE__FS_MEMORY_SESSION_H_

#include <session/session.h>
#include <base/rpc.h>
#include <base/stdint.h>
#include <base/attached_ram_dataspace.h>
#include <base/attached_rom_dataspace.h>
#include <fs/filesys_ram.h>

namespace MtsysFsMemory { 
    struct Session;
    using FsPathString = Genode::String<128>; 
}

typedef Vfs::Directory_service::Stat MtfStat;
typedef Vfs::Directory_service::Open_mode MtfOpenMode;
typedef Vfs::Directory_service::Open_result MtfOpenResult;
typedef Vfs::Directory_service::Opendir_result MtfDirResult;
typedef Vfs::Directory_service::Unlink_result MtfRmResult;
typedef Vfs::Directory_service::Rename_result MtfMvResult;
typedef Vfs::Directory_service::Stat_result MtfStatResult;
typedef Vfs::File_io_service::Read_result MtfReadResult;
typedef Vfs::File_io_service::Write_result MtfWriteResult;
typedef Vfs::File_io_service::Ftruncate_result MtfTrunResult;


// const Genode::size_t FILEIO_DSSIZE = 2 * 1024 * 1024;

struct MtsysFsMemory::Session : Genode::Session
{
    static const char *service_name() { return "MtsysFsMemory"; }

    enum { CAP_QUOTA = 4 };

    virtual int Fs_hello() = 0;

    virtual int get_IPC_stats(int client_id) = 0;

    virtual Genode::Dataspace_capability get_ds_cap() = 0;

    virtual int open(const FsPathString path, unsigned flags, unsigned mode) = 0;

    virtual int close(int fd) = 0;

    virtual int mkdir(const FsPathString path, unsigned mode) = 0;

    virtual int rmdir(const FsPathString path) = 0;

    virtual int unlink(const FsPathString path) = 0;

    virtual int rename(const FsPathString from, const FsPathString to) = 0;

    virtual int fstat(const FsPathString path, MtfStat &stat) = 0;

    virtual int read(int fd, Genode::size_t buf_off, Genode::size_t count) = 0;

    virtual int write(int fd, Genode::size_t buf_off, Genode::size_t count) = 0;

    virtual int ftruncate(int fd, Genode::size_t length) = 0;

    virtual int Transform_activation(int flag) = 0;

    virtual int Memory_hello() = 0;

    virtual genode_uint64_t query_free_space() = 0;

    virtual Genode::Ram_dataspace_capability Memory_alloc(int size, Genode::addr_t &addr) = 0;

    virtual int Memory_free(Genode::addr_t addr) = 0;

    /*******************
     ** RPC interface **
     *******************/

    GENODE_RPC(Rpc_Fs_hello, int, Fs_hello);
    GENODE_RPC(Rpc_get_IPC_stats, int, get_IPC_stats, int);
    GENODE_RPC(Rpc_get_ds_cap, Genode::Dataspace_capability, get_ds_cap);
    GENODE_RPC(Rpc_open, int, open, const FsPathString, unsigned, unsigned);
    GENODE_RPC(Rpc_close, int, close, int);
    GENODE_RPC(Rpc_mkdir, int, mkdir, const FsPathString, unsigned);
    GENODE_RPC(Rpc_rmdir, int, rmdir, const FsPathString);
    GENODE_RPC(Rpc_unlink, int, unlink, const FsPathString);
    GENODE_RPC(Rpc_rename, int, rename, const FsPathString, const FsPathString);
    GENODE_RPC(Rpc_fstat, int, fstat, const FsPathString, MtfStat&);
    GENODE_RPC(Rpc_read, int, read, int, Genode::size_t, Genode::size_t);
    GENODE_RPC(Rpc_write, int, write, int, Genode::size_t, Genode::size_t);
    GENODE_RPC(Rpc_ftruncate, int, ftruncate, int, Genode::size_t);

    GENODE_RPC(Rpc_Transform_activation, int, Transform_activation, int);
    GENODE_RPC(Rpc_Memory_hello, int, Memory_hello);
    GENODE_RPC(Rpc_query_free_space, genode_uint64_t, query_free_space);
    GENODE_RPC(Rpc_Memory_alloc, Genode::Ram_dataspace_capability, Memory_alloc, int, Genode::addr_t&);
    GENODE_RPC(Rpc_Memory_free, int, Memory_free, Genode::addr_t);

    GENODE_RPC_INTERFACE(
        Rpc_Fs_hello,
        Rpc_get_IPC_stats,
        Rpc_get_ds_cap,
        Rpc_open,
        Rpc_close,
        Rpc_mkdir,
        Rpc_rmdir,
        Rpc_unlink,
        Rpc_rename,
        Rpc_fstat,
        Rpc_read,
        Rpc_write,
        Rpc_ftruncate,
        Rpc_Transform_activation,
        Rpc_Memory_hello,
        Rpc_query_free_space,
        Rpc_Memory_alloc,
        Rpc_Memory_free
    );
};
    



#endif /* _INCLUDE__FS_MEMORY_SESSION_H_ */
