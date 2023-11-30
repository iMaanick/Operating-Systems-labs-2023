#include "../interface/conn.h"
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <cstring>
#include <iostream>

static void *shared_memory;
static const key_t key = 2;

bool Conn::Open(int id, bool create) {
    m_owner = create;

    int shm_flags = 0666;
    if (m_owner)
        shm_flags |= IPC_CREAT;

    if ((m_desc = shmget(key, sizeof(int), shm_flags)) < 0) {
        std::cout << "Can't create shared memory segment strerror:" << strerror(errno) << std::endl;
        return false;
    }

    void *m = shmat(m_desc, NULL, 0);
    if (m == (void *) -1) {
        std::cout << "Can't shmat failed error: " << strerror(errno) << std::endl;

        return false;
    }

    shared_memory = m;
    return true;
}

bool Conn::Write(void *buf, size_t count) {
    if (!shared_memory) {
        std::cout << "No memory to write for seg" << std::endl;
        return false;
    }

    memcpy(shared_memory, buf, count);
    return true;
}

bool Conn::Read(void *buf, size_t count) {
    if (!shared_memory) {
        std::cout << "No memory to read for seg" << std::endl;
        return false;
    }

    memcpy(buf, shared_memory, count);
    return true;
}

bool Conn::Close() {
    if (shared_memory && shmdt(shared_memory) != 0) {
        std::cout << "Failed segment detach error: " << strerror(errno) << std::endl;
        return false;
    }

    if (m_owner && shmctl(m_desc, IPC_RMID, NULL) != 0) {
        std::cout << "Failed to delete segment";
        return false;
    }

    shared_memory = nullptr;
    return true;
}
