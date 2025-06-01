#pragma once
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <string>
#include <filesystem>

class ConfigManager {
public:
	ConfigManager();
	
	bool load_config(const std::string& filename = "syncclip_config.ini");
	bool save_config(const std::string& filename = "syncclip_config.ini");
	bool create_default_config(const std::string& filename = "syncclip_config.ini");
	
	// 获取配置值
	std::string get_mode() const;
	std::string get_server_ip() const;
	unsigned short get_port() const;
	
	// 设置配置值
	void set_mode(const std::string& mode);
	void set_server_ip(const std::string& ip);
	void set_port(unsigned short port);
	
	// 检查配置是否有效
	bool is_valid() const;

private:
	boost::property_tree::ptree config_;
	bool valid_ = false;
};
