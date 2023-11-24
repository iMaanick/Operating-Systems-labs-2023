#include <fcntl.h>
#include <mqueue.h>
#include <iostream>
#include <cstring>
#include <sstream>

#include "../interface/conn.h"

static const char *MQ_NAME = "/LAB2_MQ";

bool Conn::Open(int id, bool create ) {
    m_owner = create;

    unsigned flags = O_RDWR;

    if (create) {
        mq_unlink(MQ_NAME);
        flags |= O_CREAT;

        struct mq_attr attr = ((struct mq_attr) {0, 1, sizeof(Message), 0, {0}});
        m_desc = mq_open(MQ_NAME, flags, 0666, &attr);
    } else {
        m_desc = mq_open(MQ_NAME, flags);
    }

    if (m_desc == -1) {
        std::cout << "Can't open message queue" << std::endl;
        return false;
    }

    return true;
}

bool Conn::Read(void *buf, size_t count) {
    if (mq_receive(m_desc, (char *)buf, count, nullptr) == -1) {
        std::cout << "Can't read message" << std::endl;
        return false;
    }
    return true;
}

bool Conn::Write(void *buf, size_t count) {
    if (mq_send(m_desc, (char *)buf, count, 0) == -1) {
        std::cout << "Can't write message" << std::endl;
        return false;
    }
    return true;
}

bool Conn::Close() {
    if (mq_close(m_desc) != 0) {
        std::cout << "Can't close mq descriptor = " << m_desc;
        return false;
    }
    else if (m_owner && mq_unlink(MQ_NAME) != 0) {
        std::cout << "Can't unlink message queue with name = " << MQ_NAME;
        return false;
    }

    return true;
}
