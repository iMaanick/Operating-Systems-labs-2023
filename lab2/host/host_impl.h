#ifndef LAB2_HOST_H
#define LAB2_HOST_H

#include <semaphore.h>

#include "../interface/conn.h"

class Host {
private:
    static const char *HOST_SEMAPHORE_NAME;
    static const char *CLIENT_SEMAPHORE_NAME;

    Conn m_connection;
    sem_t *m_host_semaphore;
    sem_t *m_client_semaphore;

    int m_wolf_num;
    int m_status;

    Host();

    void getWolfNum();

    void process();

    void close();

public:
    pid_t pid;

    Host(const Host &host) = delete;

    Host &operator=(const Host &host) = delete;

    static Host &getInstance();

    bool Init();

    void start();
};

#endif //LAB2_HOST_H
