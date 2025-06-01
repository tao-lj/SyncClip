#include <iostream>
#include <string>
#include "config/config.hpp"
#include "network/server.hpp"
#include "network/client.hpp"

int main(int argc, char* argv[]) {
	Mode mode;
	std::string server_ip;
	unsigned short port;
	
	if (!parse_arguments(argc, argv, mode, server_ip, port)) {
		return 1;
	}

	if (mode == Mode::SERVER) {
		run_server(port);
	} else {
		run_client(server_ip, port);
	}

	return 0;
}
