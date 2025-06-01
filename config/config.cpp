#include "config.hpp"
#include <iostream>
#include <string>

bool parse_arguments(int argc, char* argv[], Mode& mode, std::string& server_ip, unsigned short& port) {
	if (argc != 4) {
		std::cerr << "Usage: " << argv[0] << " <-c|-s> <ip> <port>\n";
		std::cerr << "  -c: client mode\n";
		std::cerr << "  -s: server mode\n";
		return false;
	}

	std::string mode_str = argv[1];
	if (mode_str == "-c") {
		mode = Mode::CLIENT;
	} else if (mode_str == "-s") {
		mode = Mode::SERVER;
	} else {
		std::cerr << "Invalid mode. Use -c for client or -s for server.\n";
		return false;
	}

	server_ip = argv[2];
	port = static_cast<unsigned short>(std::stoi(argv[3]));

	return true;
}
