#include "net.hpp"

#include <unistd.h>
#include <iostream>
#include <string>
#include <cstdlib>

int main(int argc, char* argv[]) {
	std::string dest_addr = "127.0.0.1";
	int dest_port = 8080;

	/* Parse CLI arguments. */
	int opt = -1;
	while ((opt = getopt(argc, argv, "a:p:")) != -1) {
		switch (opt) {
			case 'a':
				dest_addr = optarg;

				break;
			case 'p':
				dest_port = std::atoi(optarg);

				break;
			default:
				std::cerr << "Usage: " << argv[0] <<
					" [-a DEST_ADDRESS] [-p PORT]" << std::endl;

				return EXIT_FAILURE;
		}
	}

	client(dest_addr.c_str(), dest_port);
	
	return 0;
}
