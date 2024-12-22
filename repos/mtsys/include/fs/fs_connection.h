#ifndef _INCLUDE__FS_CONNECTION_H_
#define _INCLUDE__FS_CONNECTION_H_

#include <fs/fs_client.h>
#include <base/connection.h>

namespace MtsysFs { struct Connection; }

struct MtsysFs::Connection : Genode::Connection<Session>, Session_client
{
    Connection(Genode::Env &env)
    :
        /* create session */
        Genode::Connection<MtsysFs::Session>(env, Label(),
                                           Ram_quota { 64*1024 }, Args()),
        /* initialize RPC interface */
        Session_client(cap())
    { }
};

#endif /* _INCLUDE__FS_CONNECTION_H_ */
