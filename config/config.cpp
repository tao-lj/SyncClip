#include "config.hpp"
#include <iostream>
#include <string>

bool parse_arguments(int argc, char* argv[], Mode& mode, std::string& server_ip, unsigned short& port, ConfigManager& config) {
	
	// 如果没有命令行参数，使用配置文件中的设置
	if (argc == 1) {
		std::string config_mode = config.get_mode();
		if (config_mode == "client") {
			mode = Mode::CLIENT;
		} else if (config_mode == "server") {
			mode = Mode::SERVER;
		} else {
			std::cerr << "Invalid mode in config: " << config_mode << std::endl;
			return false;
		}
		
		server_ip = config.get_server_ip();
		port = config.get_port();
		return true;
	}
	
	// 处理命令行参数
	if (argc != 4) {
		std::cerr << "Usage: " << argv[0] << " <-c|-s> <ip> <port>\n";
		std::cerr << "  -c: client mode\n";
		std::cerr << "  -s: server mode\n";
		std::cerr << "Or run without arguments to use config file settings\n";
		return false;
	}

	std::string mode_str = argv[1];
	if (mode_str == "-c") {
		mode = Mode::CLIENT;
		config.set_mode("client");
	} else if (mode_str == "-s") {
		mode = Mode::SERVER;
		config.set_mode("server");
	} else {
		std::cerr << "Invalid mode. Use -c for client or -s for server.\n";
		return false;
	}

	server_ip = argv[2];
	port = static_cast<unsigned short>(std::stoi(argv[3]));
	
	config.set_server_ip(server_ip);
	config.set_port(port);
	
	// 保存更新后的配置
	config.save_config();

	return true;
}
