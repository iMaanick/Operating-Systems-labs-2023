#ifndef LAB2_CLIENT_H
#define LAB2_CLIENT_H

#include <string>
#include <semaphore.h>

#include "../interface/conn.h"

class Client {
private:
    Conn m_connection;
    sem_t *m_host_semaphore;
    sem_t *m_client_semaphore;

public:
    Client(const Client &host) = delete;

    Client &operator=(const Client &host) = delete;

    static Client &getInstance();

    Client();

    ~Client();

    void start();

    bool Init(sem_t *host_sem, sem_t *client_sem);
};

#endif //LAB2_CLIENT_H
