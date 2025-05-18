#include <base/component.h>
#include <base/log.h>
#include <base/heap.h>
#include <root/component.h>
#include <base/rpc_server.h>
#include <cpu/atomic.h>

#include <pivot/pivot_session.h>
#include <fs_memory/fs_memory_session.h>
#include <memory/memory_connection.h>
#include <memory/local_allocator.h>
#include <fs/filesys_ram.h>
#include <fs/fs_session.h>

namespace MtsysFsMemory {
    struct Component_state;
    struct Session_component;
    struct Root_component;
    struct Main;
}

const int MAX_FDNUM = 2048;
const int MEM_LEVEL_SIZE = (1 << 27);
const int MAX_MEM_SIZE = MEM_LEVEL_SIZE * DS_SIZE_LEVELS;
const int SINGLE_DS_NUM = (1 << 9);
const int MAX_MEM_CAP = MEM_LEVEL_SIZE / DS_MIN_SIZE * 2 + SINGLE_DS_NUM;
const int MEM_HASH_SIZE = 8192;
const int MEM_HASH_CAPACITY = 32;

inline int fd_user2server(int fd) {
    return fd - 7;
}

inline int fd_server2user(int fd) {
    return fd + 7;
}

typedef Vfs::Vfs_handle MfsHandle;

int start_idxof_level(int level) {
    int x = 0;
    for (int i = 0; i < level; i++) {
        x += (MEM_LEVEL_SIZE / DS_MIN_SIZE) >> i;
    }
    return x;
}

struct MtsysFsMemory::Component_state
{   
    Genode::Env &env;
    Genode::Sliced_heap sliced_heap { env.ram(), env.rm() };
    Genode::Heap heap { env.ram(), env.rm() };
    MfsHandle* fd_array[MAX_FDNUM];
    int fd_tail = 0;
    int fd_head = 0;

    int ipc_count[MAX_USERAPP] = { 0 };

    Mfs::Ram_file_system ramfs;

    genode_uint64_t address_base;
    genode_uint64_t address_end;
    genode_uint64_t address_free;
    genode_uint64_t address_used;

    Genode::Attached_ram_dataspace **ds_list;
    Genode::Attached_ram_dataspace **ds_single;
    int head_idx[DS_SIZE_LEVELS] = { 0 };
    int *used_bitmaps;
    int head_idx_single = 0;
    int *used_bitmaps_single;

    unsigned long *hash_keys;
    int *list_index;

    volatile int activated;

    int memory_ipc_fAPP[MAX_USERAPP] = { 0 };
    int memory_ipc_fSERVICE[MAX_SERVICE] = { 0 };

    Component_state(Genode::Env &env,
        genode_uint64_t base, genode_uint64_t end, 
        genode_uint64_t free, genode_uint64_t used)
    : 
    env(env),
    address_base(base), 
    address_end(end), 
    address_free(free), 
    address_used(used),
    activated(0),
    memory_ipc_fAPP(),
    memory_ipc_fSERVICE(),
    fd_array(),
    ramfs(env, heap)
    {
        Genode::log("Memory server state initializing");
        ds_list = new (sliced_heap) Genode::Attached_ram_dataspace*[MAX_MEM_CAP];
        // fill the list with different levels of dataspace
        // HOWEVER, we DO NOT need to implement the dataspace allocation
        // in this COMBINED service

        int k = 0;
        // for (int i = 0; i < DS_SIZE_LEVELS; i++) {
        //     int j = (MEM_LEVEL_SIZE / DS_MIN_SIZE) >> i;
        //     for (int l = 0; l < j; l++) {
        //         ds_list[k++] = new (sliced_heap) Genode::Attached_ram_dataspace(
        //                 env.ram(), env.rm(), (DS_MIN_SIZE << i));
        //     }
        // }
        used_bitmaps = new (sliced_heap) int[MAX_MEM_CAP];
        for (int i = 0; i < MAX_MEM_CAP; i++) {
            used_bitmaps[i] = 0;
        }
        ds_single = new (sliced_heap) Genode::Attached_ram_dataspace*[SINGLE_DS_NUM];
        // for (int i = 0; i < SINGLE_DS_NUM; i++) {
        //     ds_single[i] = 0;
        // }
        used_bitmaps_single = new (sliced_heap) int[SINGLE_DS_NUM];
        for (int i = 0; i < SINGLE_DS_NUM; i++) {
            used_bitmaps_single[i] = 0;
        }
        hash_keys = new (sliced_heap) unsigned long[MEM_HASH_SIZE * MEM_HASH_CAPACITY];
        list_index = new (sliced_heap) int[MEM_HASH_SIZE * MEM_HASH_CAPACITY];
        for (int i = 0; i < MEM_HASH_SIZE * MEM_HASH_CAPACITY; i++) {
            hash_keys[i] = 0;
            list_index[i] = -1;
        }
        Genode::log("Memory server state initialized with ", k, " dataspace");

        // fs init
        Genode::log("File system server state initializing");
        for (int i = 0; i < MAX_FDNUM; i++) {
            fd_array[i] = nullptr;
        }
   
    }

    ~Component_state() {
        Genode::log("Memory server state destroying");
        for (int i = 0; i < MAX_MEM_CAP; i++) {
            if (ds_list[i] != nullptr) {
                env.rm().detach(ds_list[i]->local_addr<void>());
                env.ram().free(ds_list[i]->cap());
                sliced_heap.free(ds_list[i], sizeof(Genode::Attached_ram_dataspace));
            }
        }
        for (int i = 0; i < SINGLE_DS_NUM; i++) {
            if (ds_single[i] != nullptr) {
                env.rm().detach(ds_single[i]->local_addr<void>());
                env.ram().free(ds_single[i]->cap());
                sliced_heap.free(ds_single[i], sizeof(Genode::Attached_ram_dataspace));
            }
        }
    }

};


struct MtsysFsMemory::Session_component : Genode::Rpc_object<Session>
{
    int client_id;
    Component_state &state;
    Genode::Attached_ram_dataspace *ds;

    int Fs_hello() override {
        Genode::log("Hi, MtsysFsMemory server for client ", client_id); 
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
            // Genode::log("Opened file ", path, " for client ", client_id);
            return fd_server2user(fd_target);
        }
        else {
            Genode::log("Failed to open file ", path, " for client ", client_id);
            return -1;
        }
    }

    int close(int fd) override {
        state.ipc_count[client_id]++;
        // Genode::log("Closing fd ", fd, " for client ", client_id);
        fd = fd_user2server(fd);
        if (fd < 0 || fd >= MAX_FDNUM || state.fd_array[fd] == nullptr) {
            Genode::log("[[ERROR]]Bad file descriptor: ", fd);
            return -1;
        }
        state.ramfs.close(state.fd_array[fd]);
        state.fd_array[fd] = nullptr;
        state.fd_head = (state.fd_head + 1) %
            MAX_FDNUM;
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

    int Transform_activation(int flag) override {
        Genode::log("[INFO] Transform activation flag: ", flag);
        int res = 0;
        int original;
        while (!res){
            original = state.activated;
            res = Genode::cmpxchg(&(state.activated), original, flag);
        } 
        return (!res);
    }

    int Memory_hello() override {
        if (state.activated == 0) {
            Genode::log("[INFO] Memory server not activated");
            return -1;
        }
        Genode::log("Hi, Mtsys Memory server for client ", client_id);
        return client_id;
    }

    genode_uint64_t query_free_space () override{
        if (state.activated == 0) {
            Genode::log("[INFO] Memory server not activated");
            return -1;
        }
        // change ipc stats 
        state.memory_ipc_fAPP[client_id]++;
        // a fake implementation now
        state.address_free -= 0x1000;
        return state.address_free;
    }

    Genode::Ram_dataspace_capability Memory_alloc(int size, Genode::addr_t &addr) override {
        if (state.activated == 0) {
            Genode::log("[INFO] Memory server not activated");
            return (state.ds_list[MAX_MEM_CAP - 1])->cap();
        }
        // change ipc stats 
        state.memory_ipc_fAPP[client_id]++;
        // allocate a dataspace and return it with addr
        int level = DS_SIZE2LEVEL(size);
        // Genode::log("Allocating dataspace size: ", size, " level: ", level);
        if (level < 0 || level >= DS_SIZE_LEVELS) {
            Genode::log("memory server: about to use single dataspace for size: ", size);
            // use single dataspace
            int target_id = -1;
            int found = 0;
            for (int i = 0; i < SINGLE_DS_NUM; i++) {
                int idx = (state.head_idx_single + i) % SINGLE_DS_NUM;
                if (state.used_bitmaps_single[idx] == 0) {
                    state.used_bitmaps_single[idx] = 1;
                    state.ds_single[idx] = new (state.sliced_heap) Genode::Attached_ram_dataspace(
                        state.env.ram(), state.env.rm(), size);
                    addr = (Genode::addr_t)(state.ds_single[idx]->local_addr<void>());
                    target_id = idx;
                    found = 1;
                    state.head_idx_single = (state.head_idx_single + 1) % SINGLE_DS_NUM;
                    break;
                }
            }
            if (!found) {
                Genode::log("[[ERROR]]No more dataspace available for allocation");
                return (state.ds_list[MAX_MEM_CAP - 1])->cap();
            }
            // insert the addr into hash table
            unsigned long h = addr;
            int b = hash_bucket(h, MEM_HASH_SIZE);
            int success = 0;
            for (int i = 0; i < MEM_HASH_CAPACITY; i++) {
                if (state.hash_keys[b * MEM_HASH_CAPACITY + i] == 0) {
                    state.hash_keys[b * MEM_HASH_CAPACITY + i] = h;
                    state.list_index[b * MEM_HASH_CAPACITY + i] = target_id + 
                                            MEM_LEVEL_SIZE / DS_MIN_SIZE * 2;
                    success = 1;
                    break;
                }
            }
            if (!success) {
                Genode::log("[[ERROR]] Mem Hash table full");
                return (state.ds_list[MAX_MEM_CAP - 1])->cap();
            }
            return state.ds_single[target_id]->cap();
            // Genode::log("[[ERROR]]Size too large for dataspace allocation");
            // return (state.ds_list[MAX_MEM_CAP - 1])->cap();
        }
        int idx = start_idxof_level(level);
        int target_id = -1;
        int found = 0;
        for (int i = 0; i < (MEM_LEVEL_SIZE / DS_MIN_SIZE) >> level; i++) {
            if (state.used_bitmaps[idx + i] == 0) {
                state.used_bitmaps[idx + i] = 1;
                addr = (Genode::addr_t)(state.ds_list[idx + i]->local_addr<void>());
                target_id = idx + i;
                found = 1;
                state.head_idx[level] = (state.head_idx[level] + 1) 
                                % ((MEM_LEVEL_SIZE / DS_MIN_SIZE) >> level);
                break;
            }
        }
        if (!found) {
            Genode::log("[[ERROR]]No more dataspace available for allocation");
            return (state.ds_list[MAX_MEM_CAP - 1])->cap();
        }
        // insert the addr into hash table
        unsigned long h = addr;
        int b = hash_bucket(h, MEM_HASH_SIZE);
        int success = 0;
        for (int i = 0; i < MEM_HASH_CAPACITY; i++) {
            if (state.hash_keys[b * MEM_HASH_CAPACITY + i] == 0) {
                state.hash_keys[b * MEM_HASH_CAPACITY + i] = h;
                state.list_index[b * MEM_HASH_CAPACITY + i] = target_id;
                success = 1;
                break;
            }
        }
        if (!success) {
            Genode::log("[[ERROR]] Mem Hash table full");
            return (state.ds_list[MAX_MEM_CAP - 1])->cap();
        }
        return state.ds_list[target_id]->cap();
    }

    int Memory_free(Genode::addr_t addr) override {
        if (state.activated == 0) {
            Genode::log("[INFO] Memory server not activated");
            return -1;
        }
        // change ipc stats 
        state.memory_ipc_fAPP[client_id]++;
        // free the dataspace
        unsigned long h = (unsigned long)addr;
        int b = hash_bucket(h, MEM_HASH_SIZE);
        int success = 0;
        for (int i = 0; i < MEM_HASH_CAPACITY; i++) {
            if (state.hash_keys[b * MEM_HASH_CAPACITY + i] == h) {
                int target_id = state.list_index[b * MEM_HASH_CAPACITY + i];
                if (target_id >= MEM_LEVEL_SIZE / DS_MIN_SIZE * 2) {
                    // single dataspace
                    state.used_bitmaps_single[target_id - MEM_LEVEL_SIZE / DS_MIN_SIZE * 2] = 0;
                    state.env.ram().free(state.ds_single[target_id - MEM_LEVEL_SIZE / DS_MIN_SIZE * 2]->cap());
                    state.sliced_heap.free(state.ds_single[target_id - MEM_LEVEL_SIZE / DS_MIN_SIZE * 2], 
                        sizeof(Genode::Attached_ram_dataspace));
                    state.ds_single[target_id - MEM_LEVEL_SIZE / DS_MIN_SIZE * 2] = 0;
                }
                else {
                    state.used_bitmaps[target_id] = 0;
                }
                state.hash_keys[b * MEM_HASH_CAPACITY + i] = 0;
                state.list_index[b * MEM_HASH_CAPACITY + i] = -1;
                success = 1;
                break;
            }
        }
        if (!success) {
            Genode::log("[[ERROR]] Address not found in hash table");
            return -1;
        }
        return 0;
    }

    Session_component(int id, Component_state &s)
    : client_id(id),
    state(s)
    { 
        Genode::log("Creating MtsysFsMemory session for client ", client_id);
        ds = new(state.sliced_heap) Genode::Attached_ram_dataspace(
                state.env.ram(), state.env.rm(), FILEIO_DSSIZE);
    }

    ~Session_component() {
        Genode::log("Destroying MtsysFsMemory session for client ", client_id);
        state.env.rm().detach(ds->local_addr<void>());
        state.env.ram().free(ds->cap());
        state.sliced_heap.free(ds, sizeof(Genode::Attached_ram_dataspace));
    }

};


class MtsysFsMemory::Root_component
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
            Genode::log("Creating MtsysFsMemory session");
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

        void _destroy_session(Session_component* session) override {
            // we should free client id here
            auto& cid = session->client_id;

            if (cid < 0 || cid >= MAX_USERAPP || !client_used[cid]) {
                Genode::log("[Critical] _destroy_session: Bad client id: %d\n", cid);
                goto END;
            }
            
            client_used[cid] = 0;
            Genode::log("Destroying MtsysFsMemory session for client ", cid);

        END:
            // call super method
            Genode::Root_component<Session_component>::_destroy_session(session);
        }
    
    public:

        Root_component(Genode::Env &env,
                    Genode::Entrypoint &ep,
                    Genode::Allocator &alloc)
        :
            Genode::Root_component<Session_component>(ep, alloc),
            env(env),
            stat(env, 0x400000000, 0xfffffffff, 0xc00000000, 0),
            client_used()
        {
            Genode::log("Creating MtsysFsMemory root component");
        }
};

struct MtsysFsMemory::Main
{
    Genode::Env &env;
    /*
     * A sliced heap is used for allocating session objects - thereby we
     * can release objects separately.
     */
    Genode::Sliced_heap sliced_heap { env.ram(), env.rm() };

    MtsysFsMemory::Root_component root { env, env.ep(), sliced_heap };

    Main(Genode::Env &env) : env(env)
    {
        env.parent().announce(env.ep().manage(root));
        Genode::log("MtsysFsMemory service started");
    }

};


void Component::construct(Genode::Env &env)
{
    Genode::log("MtsysFsMemory service constructing");
    static MtsysFsMemory::Main main(env);
}

