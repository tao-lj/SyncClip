#include "config_manager.hpp"
#include <iostream>
#include <fstream>

ConfigManager::ConfigManager() {
	// 尝试加载默认配置文件
	if (!load_config()) {
		// 如果加载失败，创建默认配置
		if (create_default_config()) {
			load_config();
		}
	}
}

bool ConfigManager::load_config(const std::string& filename) {
	namespace fs = std::filesystem;
	
	try {
		// 检查文件是否存在
		if (!fs::exists(filename)) {
			std::cerr << "Config file not found: " << filename << std::endl;
			return false;
		}
		
		// 读取INI文件
		boost::property_tree::ini_parser::read_ini(filename, config_);
		
		// 验证配置
		valid_ = config_.get_optional<std::string>("General.mode") &&
				 config_.get_optional<std::string>("General.server_ip") &&
				 config_.get_optional<unsigned short>("General.port");
		
		if (!valid_) {
			std::cerr << "Invalid configuration in file: " << filename << std::endl;
		}
		
		return valid_;
	} catch (const std::exception& e) {
		std::cerr << "Error loading config: " << e.what() << std::endl;
		return false;
	}
}

bool ConfigManager::save_config(const std::string& filename) {
	try {
		boost::property_tree::ini_parser::write_ini(filename, config_);
		return true;
	} catch (const std::exception& e) {
		std::cerr << "Error saving config: " << e.what() << std::endl;
		return false;
	}
}

bool ConfigManager::create_default_config(const std::string& filename) {
	try {
		// 设置默认值
		config_.put("General.mode", "server");
		config_.put("General.server_ip", "0.0.0.0");
		config_.put("General.port", 8080);
		
		// 保存默认配置
		return save_config(filename);
	} catch (const std::exception& e) {
		std::cerr << "Error creating default config: " << e.what() << std::endl;
		return false;
	}
}

std::string ConfigManager::get_mode() const {
	return config_.get<std::string>("General.mode", "server");
}

std::string ConfigManager::get_server_ip() const {
	return config_.get<std::string>("General.server_ip", "0.0.0.0");
}

unsigned short ConfigManager::get_port() const {
	return config_.get<unsigned short>("General.port", 8080);
}

void ConfigManager::set_mode(const std::string& mode) {
	config_.put("General.mode", mode);
}

void ConfigManager::set_server_ip(const std::string& ip) {
	config_.put("General.server_ip", ip);
}

void ConfigManager::set_port(unsigned short port) {
	config_.put("General.port", port);
}

bool ConfigManager::is_valid() const {
	return valid_;
}
