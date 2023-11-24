#include <iostream>
#include <random>

#include "client.h"

Client &Client::getInstance() {
    static Client instance;
    return instance;
}

Client::Client() : m_host_semaphore(nullptr), m_client_semaphore(nullptr)
{}

bool Client::Init(sem_t *host_sem, sem_t *client_sem) {
    if (host_sem == nullptr || client_sem == nullptr)
        return false;
    m_host_semaphore = host_sem;
    m_client_semaphore = client_sem;
    sem_wait(m_client_semaphore);
    return m_connection.Open(0, false);
}

void Client::start() {
    Message msg;
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(1, 100);
    msg.num = dist(mt);
    m_connection.Write(&msg, sizeof(msg));
    sem_post(m_host_semaphore);

    while (true) {
        sem_wait(m_client_semaphore);

        if (!m_connection.Read(&msg, sizeof(msg))) {
            std::cout << "ERROR: Can't read message" << std::endl;
            sem_post(m_host_semaphore);
            return;
        }

        if (msg.num == -1) {
            sem_post(m_host_semaphore);
            return;
        }

        std::random_device rd;
        std::mt19937 mt(rd());
        if (msg.state == 0) {
            std::uniform_int_distribution<int> dist(1, 100);
            msg.num = dist(mt);
        } else {
            std::uniform_int_distribution<int> dist(1, 50);
            msg.num = dist(mt);
        }

        if (!m_connection.Write(&msg, sizeof(Message))) {
            std::cout << "ERROR: Can't write message" << std::endl;
            sem_post(m_host_semaphore);
            return;
        }

        sem_post(m_host_semaphore);
    }
}

Client::~Client() {
    m_connection.Close();
}
