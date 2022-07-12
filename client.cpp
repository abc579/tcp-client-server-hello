#include <iostream>
#include <cstdlib>

#include "client.hpp"

/*
 * @brief This simple client program just connects to the running server.
 */

int
main()
{
    try {
        network::client c;
        c.run();
    } catch(const network::network_runtime_error &err) {
        std::cerr << err.what();
    }

    return EXIT_SUCCESS;
}
