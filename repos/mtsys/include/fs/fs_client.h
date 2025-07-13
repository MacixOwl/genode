#ifndef _INCLUDE__FS_CLIENT_H_
#define _INCLUDE__FS_CLIENT_H_

#include <base/rpc_client.h>
#include <base/log.h>
#include <fs/fs_session.h>
#include <base/exception.h>
#include <base/allocator.h>
#include <base/attached_rom_dataspace.h>
#include <base/stdint.h>


namespace MtsysFs { 
    struct Session_client; 
}


struct MtsysFs::Session_client : Genode::Rpc_client<Session>
{
    Session_client(Genode::Capability<Session> cap)
    : Genode::Rpc_client<Session>(cap) { }

    int Fs_hello() override
    {
        // Genode::log("issue RPC for saying hello");
        int r = call<Rpc_Fs_hello>();
        // Genode::log("returned from 'say_hello' RPC call");
        return r;
    }

    int get_IPC_stats(int client_id) override
    {
        return call<Rpc_get_IPC_stats>(client_id);
    }

    Genode::Dataspace_capability get_ds_cap() override
    {
        return call<Rpc_get_ds_cap>();
    }

    int open(const FsPathString path, unsigned flags, unsigned mode) override
    {
        return call<Rpc_open>(path, flags, mode);
    }

    int close(int fd) override
    {
        return call<Rpc_close>(fd);
    }

    int mkdir(const FsPathString path, unsigned mode) override
    {
        return call<Rpc_mkdir>(path, mode);
    }

    int rmdir(const FsPathString path) override
    {
        return call<Rpc_rmdir>(path);
    }

    int unlink(const FsPathString path) override
    {
        return call<Rpc_unlink>(path);
    }

    int rename(const FsPathString from, const FsPathString to) override
    {
        return call<Rpc_rename>(from, to);
    }

    int fstat(const FsPathString path, MtfStat &stat) override
    {
        return call<Rpc_fstat>(path, stat);
    }

    int read(int fd, Genode::size_t buf_off, Genode::size_t count) override
    {
        return call<Rpc_read>(fd, buf_off, count);
    }

    int write(int fd, Genode::size_t buf_off, Genode::size_t count) override
    {
        return call<Rpc_write>(fd, buf_off, count);
    }

    int ftruncate(int fd, Genode::size_t length) override
    {
        return call<Rpc_ftruncate>(fd, length);
    }
};



#endif /* _INCLUDE__FS_CLIENT_H_ */
