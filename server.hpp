#pragma once

/*
 * @brief This is a very simple stream server that sends a message to the first client that connected.
 *
 *        Stream server means that we are going to use STREAM SOCKETS with the goal of using the TCP protocol
 *        which is more reliable for sending and receiving messages than DATAGRAM SOCKETS (UDP).
 */

#include <cstring>
#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include "network.hpp"

namespace network {
    class server {
	public:
		server();
        ~server();
    public:
        void run();
    private:
        void bind_to_first_addr(addrinfo *&);
        int listen_for_connections();
        int setup_signals();
        void process_cli_connections();
        int accept_incoming_connection(socklen_t &, sockaddr_storage &);
    private:
        addrinfo hints;
        addrinfo *res {nullptr};
        struct sigaction server_signals; // Apparently sigaction is a function but also a struct; that's why we use struct.
        int server_sockfd {0};
	};
}

/*
 * @brief Initialize the server.
 *
 * @throws A network runtime error if something goes bad.
 */
network::server::server()
{
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; /* IPv4 or IPv6. */
    hints.ai_socktype = SOCK_STREAM; /* We want to use the TCP protocol. */
    hints.ai_flags = AI_PASSIVE; /* Use our IP by default. */

    int status {0};

    if ((status = getaddrinfo(nullptr, portno, &hints, &res)) != 0) {
        std::stringstream msg;
        msg << "Could not initialize server; getaddrinfo " << gai_strerror(status) << ".\n";
        throw network_runtime_error {msg.str()};
    }
}

/*
 * @brief Releases the linked-list allocated in the constructor by calling GETADDRINFO.
 */
network::server::~server()
{
    freeaddrinfo(res);
}

/*
 * @brief Loop through all the results and bind to the first we can.
 */
void
network::server::bind_to_first_addr(addrinfo *&currinfo)
{
    constexpr int yes {1};

    for (currinfo = res; currinfo != nullptr; currinfo = currinfo->ai_next) {
        if ((server_sockfd = socket(currinfo->ai_family, currinfo->ai_socktype, currinfo->ai_protocol)) == err_code) {
            std::cerr << "Could not create socket " << strerror(errno) << std::endl;
            continue;
        }

        if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == err_code) {
            std::cerr << "Could not set socket option " << strerror(errno) << std::endl;
            continue;
        }

        if (bind(server_sockfd, currinfo->ai_addr, currinfo->ai_addrlen) == err_code) {
            close(server_sockfd);
            std::cerr << "Could not bind socket " << strerror(errno) << std::endl;
            continue;
        }

        break;
    }
}

/*
 * @brief Just a simple wrapper to represent the functionality with a better name.
 *
 *        What we are doing here is marking the socket identified with SERVERSOCKFD as a passive one.
 *
 * @return ERR_CODE on error; 0 otherwise.
 */
int
network::server::listen_for_connections()
{
    const int pending_conns {10};
    return listen(server_sockfd, pending_conns);
}

/*
 * @brief Set up signal SIGCHLD for child processes. Childs will be created after calling fork() and they will
 *        call the function SIGCHLD_HANDLER when they terminate. This other function will get rid of zombie processes.
 *
 * @note SIGCHLD will be fired whenever a child process is terminated.
 * 
 * @return ERR_CODE on error; 0 otherwise.
 */
int
network::server::setup_signals()
{
    sigemptyset(&server_signals.sa_mask);
    server_signals.sa_flags = SA_RESTART;
    server_signals.sa_handler = sigchld_handler;

    return sigaction(SIGCHLD, &server_signals, nullptr);
}

/*
 * @brief Tries to accept the incoming client connection.
 *
 * @return ERR_CODE on error; child's socket file descriptor otherwise.
 */
int
network::server::accept_incoming_connection(socklen_t &sin_size, sockaddr_storage &cliaddr)
{
    sin_size = sizeof cliaddr;

    return accept(server_sockfd, reinterpret_cast<sockaddr *>(&cliaddr), &sin_size);
}

/*
 * @brief Accept an incoming client connection and send a greeting message. After that, exit.
 */
void
network::server::process_cli_connections()
{
    const std::string greeting {"Hello client!"};
    char ipstr[INET6_ADDRSTRLEN]; /* This is just to show the IP to stdout. */
    socklen_t sin_size; /* Size of client's sockaddr. */
    sockaddr_storage cliaddr; /* Client's address. */
    
    while (true) {
        auto client_fd = accept_incoming_connection(sin_size, cliaddr);

        if (client_fd == err_code) {
            std::cerr << "Could not accept connection: " << strerror(errno) << std::endl;
            continue;
        }
        
        /* Print the client IP. */
        inet_ntop(cliaddr.ss_family, get_in_addr(reinterpret_cast<sockaddr *>(&cliaddr)), ipstr, sizeof ipstr);
        std::cout << "Server: got connection from " << ipstr << std::endl;

        /*
         * The reason why we fork() is because the server needs to keep accepting connections.
         * This way, the child manages the process of sending the actual message.
         */
        if (!fork()) { /* Here goes the child process. This is a hackish way to know we're the child. */
            if (auto status = close(server_sockfd) < 0 ) { /* The child doesn't need parent's listener. */
                std::cerr << "Error closing server socket from child: " << strerror(status) << std::endl;
            }

            if (send(client_fd, greeting.c_str(), greeting.length(), 0) == err_code) {
                std::cerr << "Error sending greeting message " << strerror(errno) << std::endl;
            }

            if (auto status = close(client_fd) < 0) {
                std::cerr << "Error closing client socket from child: " << strerror(status) << std::endl;
            }

            break;
        }

        if (auto status = close(client_fd)) { /* Parent doesn't need this file descriptor. */
            std::cerr << "Error closing client socket from parent: " << strerror(status) << std::endl;
        }
    }
}

/*
 * @brief Run the server and wait for connections. When someone connects, send a greeting message and then shutdown.
 */
void
network::server::run()
{
    addrinfo *currinfo {nullptr};
    
    bind_to_first_addr(currinfo);

    if (!currinfo) {
        throw network_runtime_error {"Could not bind server."};
    }

    if (listen_for_connections() == err_code) {
        std::stringstream msg;
        msg << "Could not listen with socket: " << strerror(errno) << "\n.";
        throw network_runtime_error {msg.str()};
    }

    if (setup_signals() == err_code) {
        std::stringstream msg;
        msg << "Could not configure signal: " << strerror(errno) << "\n.";
        throw network_runtime_error {msg.str()};
    }

    std::cout << "Server: waiting for connections...\n" << std::endl;

    process_cli_connections();
}