#include "net.hpp"

#include <print>
#include <unistd.h>
#include <cstdlib>
#include <thread>

int main(int argc, char* argv[]) {
	FabricServer& server = FabricServer::instance();
	FabricClient& client = FabricClient::instance();

	/* Parse CLI options. */
	int opt;
	while ((opt = getopt(argc, argv, "ot")) != -1) {
		switch(opt) {
			case 'o':
				/* One way communication is the default setting. */
				break;
			case 't':
				server.set_comm_mode(true);
				client.set_comm_mode(true);
				break;
			default:
				std::print(stderr, "Error: unrecognized option\n");
				abort();
		}
	}

	std::thread server_thread(&FabricServer::fabric_server, &server, 8080);
	server_thread.detach();
	std::this_thread::sleep_for(std::chrono::seconds(1));

	client.fabric_client("127.0.0.1", 8080);

	return 0;
}
