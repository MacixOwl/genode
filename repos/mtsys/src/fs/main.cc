
#include <base/component.h>
#include <base/log.h>
#include <base/heap.h>
#include <root/component.h>
#include <base/rpc_server.h>
#include <cpu/atomic.h>

#include <pivot/pivot_session.h>
#include <fs/fs_session.h>
#include <memory/memory_connection.h>
#include <kv/kv_connection.h>
#include <fs/filesys_ram.h>


namespace MtsysFs {
    struct Component_state;
    struct Session_component;
    struct Root_component;
    struct Main;
}


const int MAX_FDNUM = 2048;

inline int fd_user2server(int fd) {
    return fd - 7;
}

inline int fd_server2user(int fd) {
    return fd + 7;
}


typedef Vfs::Vfs_handle MfsHandle;

struct MtsysFs::Component_state
{
    Genode::Env &env;
    Genode::Sliced_heap sliced_heap { env.ram(), env.rm() };
    MfsHandle* fd_array[MAX_FDNUM];
    int fd_tail = 0;
    int fd_head = 0;

    int ipc_count[MAX_USERAPP] = { 0 };

    Mfs::Ram_file_system ramfs;

    Component_state(Genode::Env &env)
    : env(env),
    fd_array(),
    ramfs(env, sliced_heap)
    {
        Genode::log("File system server state initializing");
        for (int i = 0; i < MAX_FDNUM; i++) {
            fd_array[i] = nullptr;
        }

    }
};



struct MtsysFs::Session_component : Genode::Rpc_object<Session>
{
    int client_id;
    Component_state &state;


    int Fs_hello() override {
        Genode::log("Hi, MtsysFs server for client ", client_id); 
        return client_id;
    }

    int get_IPC_stats(int client_id) override {
        int count = state.ipc_count[client_id];
        // Genode::log("IPC count for client ", client_id, " is ", count);
        state.ipc_count[client_id] = 0;
        return count;
    }


    int open(const FsPathString path, unsigned flags, unsigned mode) override {
        state.ipc_count[client_id]++;
        // Genode::log("Opening file ", path, " for client ", client_id);
        int fd_target = state.fd_tail;
        state.fd_tail = (state.fd_tail + 1) % MAX_FDNUM;
        if (state.fd_tail == state.fd_head) {
            Genode::log("[[ERROR]]No more file descriptors available");
            return -1;
        }
        int a = -1;
        a = state.ramfs.open(path.string(), flags, &(state.fd_array[fd_target]), 
            state.sliced_heap);
        if (a == MtfOpenResult::OPEN_OK) {
            // Genode::log("Opened file ", path, " for client ", client_id, 
            //         " with fd ", fd_target);
            return fd_server2user(fd_target);
        }
        else {
            Genode::log("Failed to open file ", path, " for client ", client_id);
            return -1;
        }
    }

    int close(int fd) override {
        state.ipc_count[client_id]++;
        Genode::log("Closing file ", fd, " for client ", client_id);
        fd = fd_user2server(fd);
        if (fd < 0 || fd >= MAX_FDNUM || state.fd_array[fd] == nullptr) {
            Genode::log("[[ERROR]]Bad file descriptor: ", fd);
            return -1;
        }
        state.ramfs.close(state.fd_array[fd]);
        state.fd_array[fd] = nullptr;
        state.fd_head = (state.fd_head + 1) % MAX_FDNUM;
        return 0;
    }
    

    Session_component(int id, Component_state &s)
    : client_id(id),
    state(s)
    { }

};




struct MtsysFs::Root_component
    : public Genode::Root_component<Session_component>
{
    private:
        Genode::Env &env;
        Component_state stat;
        int client_used[MAX_USERAPP] = { 0 };
        int next_client_id = 0;
    
    protected:

        Session_component *_create_session(const char *) override
        {
            Genode::log("Creating MtsysFs session");
            int new_client_id = -1;

            // find unused client slot
            for (int offset = 0; offset < MAX_USERAPP; offset++) {
                auto new_id = (next_client_id + offset) % MAX_USERAPP;
                if (client_used[new_id] == 0) {
                    client_used[new_id] = 1;
                    new_client_id = new_id;
                    next_client_id = (new_id + 1) % MAX_USERAPP;
                    break;
                }
            }
            
            if (new_client_id == -1) {
                Genode::log("[[ERROR]]No more clients can be created");	
                return nullptr;
            }
            return new (md_alloc()) Session_component(new_client_id, stat);
        }

        void _destroy_session(Session_component* session) override
        {
            // we should free client id here
            auto& cid = session->client_id;

            if (cid < 0 || cid >= MAX_USERAPP || !client_used[cid]) {
                Genode::log("[Critical] _destroy_session: Bad client id: %d\n", cid);
                goto END;
            }
            
            client_used[cid] = 0;
            Genode::log("Destroying MtsysFs session for client ", cid);

        END:
            // call super method
            Genode::Root_component<Session_component>::_destroy_session(session);
        }
    
    public:
    
        Root_component(Genode::Env &env, Genode::Entrypoint &ep,
                    Genode::Allocator &alloc)
        :
            Genode::Root_component<Session_component>(ep, alloc),
            env(env),
            stat(env)
        {
            Genode::log("Creating MtsysFs root component");
        }

};



struct MtsysFs::Main
{
    Genode::Env &env;

    Genode::Sliced_heap sliced_heap { env.ram(), env.rm() };

    MtsysFs::Root_component root {env, env.ep(), sliced_heap };


    Main(Genode::Env &env) : env(env)
    {
        /*
		 * Create a RPC object capability for the root interface and
		 * announce the service to our parent.
		 */
        env.parent().announce(env.ep().manage(root));
        Genode::log("MtsysFs service is ready");
    }
};


void Component::construct(Genode::Env &env)
{
    static MtsysFs::Main main(env);
    
}
