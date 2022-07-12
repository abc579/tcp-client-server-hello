#include <cstdlib>
#include <iostream>
#include "server.hpp"

/*
 * @brief This is a simple server that responds with a "Hello, client." message when someone
 * connects.
 */

int
main()
{
	std::ios_base::sync_with_stdio(false);
	
	try {
		network::server sv;
		sv.run();
	} catch (const network::network_runtime_error &err) {
		std::cerr << "Unhandled exception: " << err.what() << std::endl;
	}

	return EXIT_SUCCESS;
}