
#include <base/component.h>
#include <base/log.h>
#include <base/heap.h>
#include <root/component.h>
#include <base/rpc_server.h>
#include <cpu/atomic.h>

#include <pivot/pivot_session.h>
#include <fs/fs_session.h>
#include <memory/memory_connection.h>
#include <memory/local_allocator.h>
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
    MtsysMemory::Connection mem_obj { env };
    MtsysMemory::Local_allocator alloc_obj { env, mem_obj };
    MfsHandle* fd_array[MAX_FDNUM];
    int fd_tail = 0;
    int fd_head = 0;

    int ipc_count[MAX_USERAPP] = { 0 };

    Mfs::Ram_file_system ramfs;

    Component_state(Genode::Env &env)
    : env(env),
    fd_array(),
    ramfs(env, alloc_obj)
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
    Genode::Attached_ram_dataspace *ds;


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


    Genode::Dataspace_capability get_ds_cap() {
        return ds->cap();
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
        // Genode::log("Closing file ", fd, " for client ", client_id);
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

    int mkdir(const FsPathString path, unsigned mode) override {
        state.ipc_count[client_id]++;
        // Genode::log("Creating directory ", path, " for client ", client_id);
        int a = -1;
        int fd_target = state.fd_tail;
        state.fd_tail = (state.fd_tail + 1) % MAX_FDNUM;
        if (state.fd_tail == state.fd_head) {
            Genode::log("[[ERROR]]No more file descriptors available");
            return -1;
        }
        a = state.ramfs.opendir(path.string(), 1, &(state.fd_array[fd_target]), 
            state.sliced_heap);
        if (a == MtfDirResult::OPENDIR_OK) {
            Genode::log("Created directory ", path, " for client ", client_id);
            // close it immediately
            state.ramfs.close(state.fd_array[fd_target]);
            state.fd_array[fd_target] = nullptr;
            state.fd_head = (state.fd_head + 1) % MAX_FDNUM;
            return 0;
        }
        else {
            Genode::log("Failed to create directory ", path, " for client ", client_id);
            return -1;
        }
    }

    int rmdir(const FsPathString path) override {
        state.ipc_count[client_id]++;
        // Genode::log("Removing directory ", path, " for client ", client_id);
        int a = -1;
        a = state.ramfs.unlink(path.string());
        if (a == MtfRmResult::UNLINK_OK) {
            Genode::log("Removed directory ", path, " for client ", client_id);
            return 0;
        }
        else {
            Genode::log("Failed to remove directory ", path, " for client ", client_id);
            return -1;
        }
    }

    int unlink(const FsPathString path) override {
        state.ipc_count[client_id]++;
        // Genode::log("Removing file ", path, " for client ", client_id);
        int a = -1;
        a = state.ramfs.unlink(path.string());
        if (a == MtfRmResult::UNLINK_OK) {
            Genode::log("Removed file ", path, " for client ", client_id);
            return 0;
        }
        else {
            Genode::log("Failed to remove file ", path, " for client ", client_id);
            return -1;
        }
    }

    int rename(const FsPathString from, const FsPathString to) override {
        state.ipc_count[client_id]++;
        // Genode::log("Renaming ", from, " to ", to, " for client ", client_id);
        int a = -1;
        a = state.ramfs.rename(from.string(), to.string());
        if (a == MtfMvResult::RENAME_OK) {
            Genode::log("Renamed ", from, " to ", to, " for client ", client_id);
            return 0;
        }
        else {
            Genode::log("Failed to rename ", from, " to ", to, " for client ", client_id);
            return -1;
        }
    }

    int fstat(const FsPathString path, MtfStat &stat) override {
        state.ipc_count[client_id]++;
        // Genode::log("Statting ", path, " for client ", client_id);
        int a = -1;
        a = state.ramfs.stat(path.string(), stat);
        if (a == MtfStatResult::STAT_OK) {
            Genode::log("Statted ", path, " for client ", client_id);
            return 0;
        }
        else {
            Genode::log("Failed to stat ", path, " for client ", client_id);
            return -1;
        }
    }

    int read(int fd, Genode::size_t buf_off, Genode::size_t count) override {
        state.ipc_count[client_id]++;
        // Genode::log("Reading from fd ", fd, " for client ", client_id);
        fd = fd_user2server(fd);
        if (fd < 0 || fd >= MAX_FDNUM || state.fd_array[fd] == nullptr) {
            Genode::log("[[ERROR]]Bad file descriptor: ", fd);
            return -1;
        }
        int a = -1;
        Genode::Byte_range_ptr buff(ds->local_addr<char>() + buf_off, count);
        a = state.ramfs.complete_read(state.fd_array[fd], buff, count);
        if (a == MtfReadResult::READ_OK) {
            Genode::log("Read from fd ", fd, " for client ", client_id);
            return count;
        }
        else {
            Genode::log("Failed to read from fd ", fd, " for client ", client_id);
            return -1;
        }
    }

    int write(int fd, Genode::size_t buf_off, Genode::size_t count) override {
        state.ipc_count[client_id]++;
        // Genode::log("Writing to fd ", fd, " for client ", client_id);
        fd = fd_user2server(fd);
        if (fd < 0 || fd >= MAX_FDNUM || state.fd_array[fd] == nullptr) {
            Genode::log("[[ERROR]]Bad file descriptor: ", fd);
            return -1;
        }
        int a = -1;
        // Genode::log("Writing to fd ", fd, ": ", buf, " char count: ", count);
        Genode::Const_byte_range_ptr buff(ds->local_addr<char>() + buf_off, count);
        a = state.ramfs.write(state.fd_array[fd], buff, count);
        if (a == MtfWriteResult::WRITE_OK) {
            // Genode::log("Wrote to fd ", fd, " for client ", client_id);
            return count;
        }
        else {
            Genode::log("Failed to write to fd ", fd, " for client ", client_id);
            return -1;
        }
    }

    int ftruncate(int fd, Genode::size_t length) override {
        state.ipc_count[client_id]++;
        // Genode::log("Truncating fd ", fd, " for client ", client_id);
        fd = fd_user2server(fd);
        if (fd < 0 || fd >= MAX_FDNUM || state.fd_array[fd] == nullptr) {
            Genode::log("[[ERROR]]Bad file descriptor: ", fd);
            return -1;
        }
        int a = -1;
        a = state.ramfs.ftruncate(state.fd_array[fd], (Vfs::file_size)length);
        if (a == MtfTrunResult::FTRUNCATE_OK) {
            // Genode::log("Truncated fd ", fd, " for client ", client_id);
            return 0;
        }
        else {
            Genode::log("Failed to truncate fd ", fd, " for client ", client_id);
            return -1;
        }
    }
    

    Session_component(int id, Component_state &s)
    : client_id(id),
    state(s)
    { 
        Genode::log("Creating MtsysFs session for client ", client_id);
        ds = new(state.sliced_heap) Genode::Attached_ram_dataspace(
                state.env.ram(), state.env.rm(), FILEIO_DSSIZE);
    }

    ~Session_component() {
        Genode::log("Destroying MtsysFs session for client ", client_id);
        state.env.rm().detach(ds->local_addr<void>());
        state.env.ram().free(ds->cap());
        state.sliced_heap.free(ds, sizeof(Genode::Attached_ram_dataspace));
    }

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
