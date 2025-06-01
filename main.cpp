#include <iostream>
#include <string>
#include "config/config.hpp"
#include "config/config_manager.hpp"
#include "network/server.hpp"
#include "network/client.hpp"

int main(int argc, char* argv[]) {
	// 创建配置管理器
	ConfigManager config_manager;
	
	Mode mode;
	std::string server_ip;
	unsigned short port;
	
	if (!parse_arguments(argc, argv, mode, server_ip, port, config_manager)) {
		return 1;
	}

	std::cout << "Starting in " << (mode == Mode::SERVER ? "server" : "client") << " mode\n";
	std::cout << "Server IP: " << server_ip << "\n";
	std::cout << "Port: " << port << std::endl;

	if (mode == Mode::SERVER) {
		run_server(port);
	} else {
		run_client(server_ip, port);
	}

	return 0;
}
