// Combine fs connection and memory connection

#ifndef _INCLUDE__FS_MEMORY_CONNECTION_H_
#define _INCLUDE__FS_MEMORY_CONNECTION_H_

#include <fs_memory/fs_memory_client.h>
#include <base/connection.h>


namespace MtsysFsMemory { struct Connection; }

struct MtsysFsMemory::Connection : Genode::Connection<Session>, Session_client
{
    Connection(Genode::Env &env)
    :
        /* create session */
        Genode::Connection<MtsysFsMemory::Session>(env, Label(),
                                           Ram_quota { 64*1024 }, Args()),
        /* initialize RPC interface */
        Session_client(cap())
    { }
};



#endif /* _INCLUDE__FS_MEMORY_CONNECTION_H_ */
