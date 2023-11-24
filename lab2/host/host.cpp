#include <cstring>
#include <iostream>

#include "host_impl.h"

int main( int argc, char **argv ) {
    Host &host = Host::getInstance();
    if (host.Init()) {
        host.start();
    } else {
        std::cout << "ERROR: errno = " << strerror(errno) << std::endl;
        return 1;
    }

    return 0;
}
