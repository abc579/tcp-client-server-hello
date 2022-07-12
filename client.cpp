#include <iostream>
#include <cstdlib>
#include "client.hpp"

/*
 * @brief This is a simple client program that just connects to the running server and prints whatever
 *        message it received.
 */
int
main()
{
    std::ios_base::sync_with_stdio(false);

    try {
        network::client c;
        c.run();
    } catch(const network::network_runtime_error &err) {
        std::cerr << err.what();
    }

    return EXIT_SUCCESS;
}
