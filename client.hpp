#pragma once

#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <errno.h>
#include <unistd.h>

namespace network {
    const char *portno {"666"}; // This is the port to which the client will connect to.
    constexpr int maxdatasize {100};
    const char *host {"127.0.0.1"};
    constexpr int err_code {-1};

    class network_runtime_error : public std::exception {
    public:
        network_runtime_error(const std::string &m)
            :msg{m}
        {

        }
    public:
        const char *what() const throw() override
        {
            return msg.c_str();
        }
    private:
        std::string msg;
    };

    /*
     * @brief Get socket address in IPv4 or IPv6 depending on how SA was configured.
     */
    void *
    get_in_addr(sockaddr *sa)
    {
        if (sa->sa_family == AF_INET) {
            return &(reinterpret_cast<sockaddr_in *>(sa)->sin_addr);
        }

        return &(reinterpret_cast<sockaddr_in6 *>(sa)->sin6_addr);
    }

    class client {
    public:
        client();
        ~client();
    public:
        void run();
    private:
        addrinfo hints;
        addrinfo *serverinfo;
    };
}

network::client::client()
{
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status {0};

    if ((status = getaddrinfo(host, portno, &hints, &serverinfo)) != 0) {
        std::stringstream msg;
        msg << "Could not get address info: " << gai_strerror(status) << ".\n";
        throw network_runtime_error {msg.str()};
    }
}

network::client::~client()
{
    freeaddrinfo(serverinfo);
}

void
network::client::run()
{
    addrinfo *p {nullptr};
    int sockfd {0};

    for (p = serverinfo; p != nullptr; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == err_code) {
            std::cerr << "Could not create socket " << strerror(errno) << std::endl;
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == err_code) {
            close(sockfd);
            std::cerr << "Could not connect to server " << strerror(errno) << std::endl;
            continue;
        }

        break;
    }

    if (!p) {
        std::stringstream msg;
        msg << "Failed to connect to any server: " << strerror(errno);
        throw network_runtime_error {msg.str()};
    }

    char ipstr[INET6_ADDRSTRLEN];

    inet_ntop(p->ai_family, get_in_addr(reinterpret_cast<sockaddr *>(p->ai_addr)), ipstr, sizeof ipstr);

    std::cout << "Connecting to " << ipstr << std::endl;

    int numbytes {0};
    char buff[maxdatasize];

    if ((numbytes = recv(sockfd, buff, maxdatasize - 1, 0)) == err_code) {
        std::stringstream msg;
        msg << "Error receiving bytes from host " << strerror(errno);
        throw network_runtime_error {msg.str()};
    }

    buff[numbytes] = '\0';

    std::cout << "Server responded with: " << buff << std::endl;

    close(sockfd);
}