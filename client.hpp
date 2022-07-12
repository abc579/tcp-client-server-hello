#pragma once

#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <unistd.h>

#include "network.hpp"

namespace network {
    class client {
    public:
        client();
        ~client();
    public:
        void run();
        void connect_to_first_addr(addrinfo *&);
        void recv_data_from_server();
    private:
        addrinfo hints;
        addrinfo *res {nullptr};
        int sockfd {0};
    };
}

network::client::client()
{
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; /* IPv4 or IPv6. */
    hints.ai_socktype = SOCK_STREAM; /* We want to use the TCP protocol. */

    int status {0};

    if ((status = getaddrinfo(host, portno, &hints, &res)) != 0) {
        std::stringstream msg;
        msg << "Could not get address info: " << gai_strerror(status) << ".\n";
        throw network_runtime_error {msg.str()};
    }
}

network::client::~client()
{
    freeaddrinfo(res);
    close(sockfd);
}

void
network::client::connect_to_first_addr(addrinfo *&currinfo)
{
    for (currinfo = res; currinfo != nullptr; currinfo = currinfo->ai_next) {
        if ((sockfd = socket(currinfo->ai_family, currinfo->ai_socktype, currinfo->ai_protocol)) == err_code) {
            std::cerr << "Could not create socket " << strerror(errno) << std::endl;
            continue;
        }

        if (connect(sockfd, currinfo->ai_addr, currinfo->ai_addrlen) == err_code) {
            close(sockfd);
            std::cerr << "Could not connect to server " << strerror(errno) << std::endl;
            continue;
        }

        break;
    }
}

void
network::client::recv_data_from_server()
{
    constexpr int maxdatasize {100};
    int numbytes {0};
    char buff[maxdatasize];

    if ((numbytes = recv(sockfd, buff, maxdatasize - 1, 0)) == err_code) {
        close(sockfd);
        std::stringstream msg;
        msg << "Error receiving bytes from host " << strerror(errno);
        throw network_runtime_error {msg.str()};
    }

    buff[numbytes] = '\0';

    std::cout << "Server responded with: " << buff << std::endl;
}

void
network::client::run()
{
    addrinfo *currinfo {nullptr};
    
    connect_to_first_addr(currinfo);

    if (!currinfo) {
        std::stringstream msg;
        msg << "Failed to connect to the server: " << strerror(errno) << std::endl;
        throw network_runtime_error {msg.str()};
    }

    /* This is just to print the IP we are connecting to. */
    char ipstr[INET6_ADDRSTRLEN];
    inet_ntop(currinfo->ai_family, get_in_addr(reinterpret_cast<sockaddr *>(currinfo->ai_addr)), ipstr, sizeof ipstr);
    std::cout << "Connecting to " << ipstr << std::endl;

    recv_data_from_server();
}