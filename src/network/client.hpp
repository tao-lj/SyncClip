#pragma once
#include <boost/asio.hpp>
#include "../clipboard/clipboard_manager.hpp"
#include "../config/config.hpp"

namespace asio = boost::asio;
using asio::ip::tcp;

void run_client(const std::string& server_ip, unsigned short port);
