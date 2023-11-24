#include <csignal>
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <cmath>
#include <fcntl.h>
#include <cstring>
#include <ctime>
#include <random>

#include "host_impl.h"
#include "../client/client.h"

const char *Host::HOST_SEMAPHORE_NAME = "/host_semaphore";
const char *Host::CLIENT_SEMAPHORE_NAME = "/client_semaphore";

Host::Host() : m_wolf_num(0), m_status(0)
{}

void handler(int signum) {
    static Host &instance = Host::getInstance();
    if (instance.pid > 0)
        kill(instance.pid, signum);
    exit(signum);
}

bool Host::Init() {
    sem_unlink(HOST_SEMAPHORE_NAME);
    sem_unlink(CLIENT_SEMAPHORE_NAME);

    m_host_semaphore = sem_open(HOST_SEMAPHORE_NAME, O_CREAT, 0666, 0); // debug x2
    if (m_host_semaphore == SEM_FAILED) {
        std::cout << "ERROR: Can't open host semaphore, errno = " << strerror(errno);
        return false;
    }

    m_client_semaphore = sem_open(CLIENT_SEMAPHORE_NAME, O_CREAT, 0666, 0);
    if (m_client_semaphore == SEM_FAILED) {
        sem_unlink(HOST_SEMAPHORE_NAME);
        std::cout << "ERROR: Can't open client semaphore, errno = " << strerror(errno);
        return false;
    }

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGKILL);
    sigaddset(&set, SIGSTOP);
    act.sa_mask = set;
    sigaction(SIGTERM, &act, 0);
    sigaction(SIGKILL, &act, 0);
    sigaction(SIGSTOP, &act, 0);

    return true;
}

Host &Host::getInstance() {
    static Host instance;
    return instance;
}

void Host::start() {

    switch (pid = fork()) {
        case -1:
            std::cout << "Can't create client process" << std::endl;
        case 0: {
            Client &client = Client::getInstance();
            if (client.Init(m_host_semaphore, m_client_semaphore)) {
                client.start();
            }
            break;
        }
        default: {
            std::cout << "Successfully created client process" << std::endl;
            if (m_connection.Open(0, true))
            {
                sem_post(m_client_semaphore);
                process();
                close();
            }
            break;
        }
    }
}

void Host::process() {
    Message msg;
    const int TIMEOUT_SEC = 5;

    while (true) {
        struct timespec ts;
        timespec_get(&ts, TIMER_ABSTIME);
        ts.tv_sec += TIMEOUT_SEC;
        if (sem_timedwait(m_host_semaphore, &ts) == -1)
        {
            std::cout << "Timeout" << std::endl;
            msg.num = -1;
            sem_post(m_client_semaphore);
            break;
        }
        getWolfNum();
        if (m_connection.Read(&msg, sizeof(msg))) {
            std::cout << "Goat number: " << msg.num << std::endl;
            if (msg.state == 0) {
                if (std::abs(m_wolf_num - msg.num) > 70)
                    msg.state = 1;
            } else {
                if (std::abs(m_wolf_num - msg.num) <= 20)
                    msg.state = 0;
                else {
                    std::cout << "Game over." << std::endl;
                    msg.num = -1;
                    m_connection.Write(&msg, sizeof(msg));
                    sem_post(m_client_semaphore);
                    break;
                }
            }
            if (msg.state == 0)
                std::cout << "Goat is alive" << std::endl;
            else
                std::cout << "Goat is dead" << std::endl;

            m_connection.Write(&msg, sizeof(msg));
        }
        sem_post(m_client_semaphore);

    }
}

void Host::getWolfNum() {
    std::cout << "Input wolf number:" << std::endl;
    std::string word;
    int num = 0;
    do {
        fd_set fds;
        int console = fileno(stdin);
        FD_ZERO(&fds);
        FD_SET(console, &fds);
        struct timeval timeout;
        timeout.tv_sec = 3; 
        timeout.tv_usec = 0;
        int ready = select(console + 1, &fds, nullptr, nullptr, &timeout);
    
        if (ready == -1) {
            std::cerr << "Error in select()\n";
        }
        else if (ready == 0) {
            std::random_device rd;
            std::mt19937 mt(rd());
            std::uniform_int_distribution<int> dist(1, 100);
            num = dist(mt);
            std::cout << num << std::endl;
        } 
        else {
            if (FD_ISSET(console, &fds)) {
                char buffer[256];
                read(console, buffer, sizeof(buffer));
                try {
                    num = std::atoi(buffer);
                } catch (std::exception &e) {
                    std::cout << "Try again" << std::endl;
                }
                if (num < 1 || num > 100) {
                    std::cout << "Number must be between 1 and 100" << std::endl;
                }
            }
        }
    } while (num < 1 || num > 100);
    m_wolf_num = num;
}

void Host::close()
{
  m_connection.Close();

  if (sem_close(m_host_semaphore) == -1)
    std::cout << "ERROR: failed to close host semaphore, errno = " << strerror(errno) << std::endl;
  if (sem_unlink(HOST_SEMAPHORE_NAME) == -1)
    std::cout << "ERROR: failed to unlink host semaphore, errno = " << strerror(errno) << std::endl;
  if (sem_close(m_client_semaphore) == -1)
    std::cout << "ERROR: failed to close client semaphore, errno = " << strerror(errno) << std::endl;
  if (sem_unlink(CLIENT_SEMAPHORE_NAME) == -1)
    std::cout << "ERROR: failed to unlink client semaphore, errno = " << strerror(errno) << std::endl;
}

