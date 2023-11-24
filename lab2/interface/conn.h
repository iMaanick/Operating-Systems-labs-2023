#ifndef CONN_H
#define CONN_H

#include "message.h"
#include <cstdlib>
#include <string>

class Conn {
public:
    bool Open(int id, bool create);

    bool Read(void *buf, size_t count);

    bool Write(void *buf, size_t count);

    bool Close();

private:
    bool m_owner;
    int m_desc;
};

#endif // CONN_H
