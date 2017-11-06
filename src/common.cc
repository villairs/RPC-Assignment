#include "common.h"

ssize_t send_debug(int sockfd, const void *buf, size_t len, int flags) {
    //std::cout << "Sending " << len << " bytes.. ";
    //std::cout.flush();
    ssize_t result = send(sockfd, buf, len, flags);
    //if (result < 0) std::cout << "Fail!" << std::endl;
    //else std::cout << "OK!" << std::endl;
    return result;
}

ssize_t recv_debug(int sockfd, void *buf, size_t len, int flags) {
    //std::cout << "Receiving " << len << " bytes..";
    //std::cout.flush();
    ssize_t result = recv(sockfd, buf, len, flags);
    //if (result < 0) std::cout << "Fail!" << std::endl;
    //else std::cout << "OK!" << std::endl;
    return result;
}

// Establishes a connection to the server at (address, port)
int establish(std::string address, std::string port) {
    int status;
    int sock_fd;
    struct addrinfo hints, *res, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // get info
    if ((status = getaddrinfo(address.c_str(), port.c_str(), &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        // create socket
        if ((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            continue;
        }

        // connect to server
        if (connect(sock_fd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sock_fd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        return -1;
    }

    //std::cout << "Connected to " << address << ":" << port << std::endl;

    freeaddrinfo(res);

    return sock_fd;
}

// Sets up a listener; arguments are modified to store address and port
int setup(std::string& address, std::string& port) {
    int status;
    int sock_fd;
    int yes = 1;
    struct addrinfo hints, *res, *p;
    struct sockaddr_in sin;
    socklen_t len = sizeof sin;
    char hostname[1024];
    hostname[1023] = '\0';

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    gethostname(hostname, 1023);

    // get info
    if ((status = getaddrinfo(NULL, "0", &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        // create socket
        if ((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            continue;
        }

        // prevent "Address already in use."
        if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0) {
            perror("setsockopt");
            return -1;
        }

        // bind server to a port
        if (bind(sock_fd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sock_fd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        return -1;
    }

    freeaddrinfo(res);

    if (getsockname(sock_fd, (struct sockaddr *)&sin, &len) < 0) {
        perror("getsockname");
        return -1;
    }
    
    address = std::string(hostname);
    std::stringstream ss;
    ss << ntohs(sin.sin_port);
    port = ss.str();

    // listen to incoming connections
    if (listen(sock_fd, 100) < 0) {
        perror("listen");
        return -1;
    }

    return sock_fd;
}

