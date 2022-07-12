#pragma once

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

namespace network {
    const char *portno {"6666"}; /* Less than 1024 require superuser privileges. */
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
     *
     * @param[in] SA Socket address.
     * 
     * @return The corresponding address in IPv4 or IPv6.
     */
    void *
    get_in_addr(sockaddr *sa)
    {
        if (sa->sa_family == AF_INET) {
            return &(reinterpret_cast<sockaddr_in *>(sa)->sin_addr);
        }

        return &(reinterpret_cast<sockaddr_in6 *>(sa)->sin6_addr);
    }

    /*
     * @brief Get rid of zombie processes after fork() calls.
     */
    void
    sigchld_handler(int)
    {
        int preverrno = errno; /* Save previous errno because WAITPID could change its value. */

        while (waitpid(-1, nullptr, WNOHANG) > 0)
            ;
        
        errno = preverrno;
    }
}