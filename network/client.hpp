#pragma once
#include <boost/asio.hpp>
#include "../clipboard/clipboard_manager.hpp"
#include "../config/config.hpp"

// 添加命名空间别名
namespace asio = boost::asio;
using asio::ip::tcp;

void run_client(const std::string& server_ip, unsigned short port);
