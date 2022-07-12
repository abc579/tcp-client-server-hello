#pragma once

/*
 * @brief This is a very simple stream server. When someone connects to this server it just sends
 * a greeting message.
 */

#include <errno.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <csignal>
#include <sys/wait.h>

namespace network {
	const char *portno {"666"}; // Port number users will connect to.
    constexpr int err_code {-1};
    constexpr int yes {1};
    constexpr int pending_conns {10};

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
     * @brief For zombie processes after fork() calls.
     */
    void
    sigchld_handler(int)
    {
        int preverrno = errno;

        while (waitpid(-1, nullptr, WNOHANG) > 0)
            ;
        
        errno = preverrno;
    }

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

    class server {
	public:
		server();
        ~server();
    public:
        void run();
    private:
        addrinfo hints; // @TODO: What does this mean?
        addrinfo *info; // @TODO: What does this mean?
        struct sigaction sa; // Apparently sigaction is a function but also a struct; that's why we use struct.
        socklen_t sin_size; // @TODO: What does this mean?
        sockaddr_storage cliaddr; // Addresses of clients.
	};
};

network::server::server()
{
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // Use my IP by default.

    if (getaddrinfo(nullptr, portno, &hints, &info) != 0) {
        std::stringstream msg;
        msg << "Could not get address info for port " << portno << ".\n";
        throw network_runtime_error {msg.str()};
    }
}

network::server::~server()
{
    freeaddrinfo(info);
}

void
network::server::run()
{
    addrinfo *p {nullptr};
    int sockfd {0};
    
    // @TODO: Chance to refactor. #2 naming, function
    // Loop through all the results and bind to the first we can.
    for (p = info; p != nullptr; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == err_code) { // @TODO: Chance to refactor. #1 object?
            perror("Could not create socket; ");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == err_code) {
            perror("Could not set socket option; ");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == err_code) {
            close(sockfd); // @TODO: Chance to refactor. #1 object?
            perror("Could not bind socket; ");
            continue;
        }

        break;
    }

    if (!p) {
        throw network_runtime_error {"Could not bind server."};
    }

    if (listen(sockfd, pending_conns) == err_code) {
        std::stringstream msg;
        msg << "Could not listen with socket: " << strerror(errno) << "\n.";
        throw network_runtime_error {msg.str()};
    }

    // @TODO: Chance to refactor. #3 naming, function
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = sigchld_handler;

    if (sigaction(SIGCHLD, &sa, nullptr) == err_code) {
        // @TODO: Chance to refactor. #4 encapsulate this process inside a function for every repetition.
        std::stringstream msg;
        msg << "Could not configure signal: " << strerror(errno) << "\n.";
        throw network_runtime_error {msg.str()};
    }

    std::cout << "Server: waiting for connections...\n" << std::endl;

    char s[INET6_ADDRSTRLEN]; // @TODO: rename.
    const std::string greeting {"Hello client!"};

    // @TODO: Chance to refactor. #5 naming, function
    while (true) {
        sin_size = sizeof cliaddr;
        int newfd = accept(sockfd, reinterpret_cast<sockaddr *>(&cliaddr), &sin_size);

        if (newfd == err_code) {
            std::cerr << "Could not accept connection: " << strerror(errno) << std::endl;
            continue;
        }

        inet_ntop(cliaddr.ss_family, get_in_addr(reinterpret_cast<sockaddr *>(&cliaddr)), s, sizeof s);

        std::cout << "Server: got connection from " << s << std::endl;

        if (fork()) {
            close(sockfd); // The child doesn't need the listener.

            if (send(newfd, greeting.c_str(), greeting.length(), 0) == err_code) {
                std::cerr << "Error sending greeting message " << strerror(errno) << std::endl;
            }

            close(newfd);
            break;
        }

        close(newfd); // Parent doesn't need this file descriptor.
    }
}