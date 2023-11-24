#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <iostream>

#include "../interface/conn.h"

static const char * const FIFO_PATH = "/tmp/fifo_file";

bool Conn::Open( int id, bool create ) {
    m_owner = create;

    if (m_owner) {
        unlink(FIFO_PATH);
        int flags = S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH;
        if (mkfifo(FIFO_PATH, flags) == -1) {
            std::cout << "ERROR: Cann't create fifo file" << std::endl;
            return false;
        }
    }

    if ((m_desc = ::open(FIFO_PATH, O_RDWR)) == -1) {
        std::cout << "ERROR: Cann't open fifo file" << std::endl;
        return false;
    }

    return true;
}

bool Conn::Write( void *buf, size_t count) {
    Message *msg = (Message *) buf;
    if (::write(m_desc, msg, count) == -1) {
        std::cout << "ERROR: Can't write message" << std::endl;
        return false;
    }
    return true;
}

bool Conn::Read(void *buf, size_t count) {
    if (::read(m_desc, buf, count) == -1) {
        std::cout << "ERROR: Can't read message" << std::endl;
        return false;
    }

    return true;
}

bool Conn::Close() {
    bool ans = true;
    if (::close(m_desc) == -1) {
        std::cout << "ERROR: Can't close file, errno = " << strerror(errno) << std::endl;
        ans = false;
    }

    if (m_owner && unlink(FIFO_PATH) == -1) {
        std::cout << "ERROR: Can't unlink file, errno = " << strerror(errno) << std::endl;
        ans = false;
    }

    return ans;
}
