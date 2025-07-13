// define the fs session header

#ifndef _INCLUDE__FS_SESSION_H_
#define _INCLUDE__FS_SESSION_H_

#include <session/session.h>
#include <base/rpc.h>
#include <base/stdint.h>
#include <base/attached_rom_dataspace.h>

#include <fs/filesys_ram.h>

namespace MtsysFs { 
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


const Genode::size_t FILEIO_DSSIZE = 2 * 1024 * 1024;


struct MtsysFs::Session : Genode::Session
{
    static const char *service_name() { return "MtsysFs"; }

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
        Rpc_ftruncate
    );

};

#endif // _INCLUDE__FS_SESSION_H_
