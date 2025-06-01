#include "server.hpp"
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <memory>
#include <algorithm>

// 添加必要的命名空间引用
using boost::system::error_code;
using boost::system::system_error;

void run_server(unsigned short port) {
	try {
		asio::io_context io_context;
		tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
		ClipboardManager clipboard;

		std::cout << "Clipboard server started. Listening on "
			<< acceptor.local_endpoint().address().to_string()
			<< ":" << acceptor.local_endpoint().port() << std::endl;
		std::cout << "Server UUID: " << clipboard.get_uuid_str() << std::endl;

		std::vector<std::thread> client_threads;
		std::mutex clients_mutex;
		std::vector<std::shared_ptr<tcp::socket>> clients;

		// 接受连接的线程
		std::thread acceptor_thread([&]() {
			while (true) {
				auto socket = std::make_shared<tcp::socket>(io_context);
				acceptor.accept(*socket);

				std::string client_ip = socket->remote_endpoint().address().to_string();
				unsigned short client_port = socket->remote_endpoint().port();
				std::cout << "Client connected from: " << client_ip << ":" << client_port << std::endl;

				// 添加到客户端列表
				{
					std::lock_guard<std::mutex> lock(clients_mutex);
					clients.push_back(socket);
				}

				// 为客户端启动处理线程
				client_threads.emplace_back([socket, &clients, &clients_mutex, &clipboard]() {
					try {
						// 使用动态分配的缓冲区
						std::vector<char> recv_buffer(BUFFER_SIZE);
						while (true) {
							error_code ec;
							size_t length = socket->read_some(asio::buffer(recv_buffer), ec);

							if (ec == asio::error::eof) { // 修复这里
								std::cout << "Client disconnected" << std::endl;
								break;
							}
							else if (ec) {
								throw system_error(ec);
							}

							std::string received(recv_buffer.data(), length);
							std::cout << "Received clipboard content (" << length << " bytes)" << std::endl;

							// 更新服务器剪贴板
							ClipboardData data;
							data.content = received;
							data.mime_type = "text/plain; charset=utf-8";
							clipboard.set_clipboard_content(data);

							// 广播给所有其他客户端
							std::lock_guard<std::mutex> lock(clients_mutex);
							for (auto& client : clients) {
								if (client != socket && client->is_open()) {
									try {
										asio::write(*client, asio::buffer(received), ec);
									}
									catch (...) {
										// 忽略发送失败的客户端
									}
								}
							}
						}
					}
					catch (std::exception& e) {
						std::cerr << "Client handling exception: " << e.what() << std::endl;
					}

					// 从客户端列表中移除
					{
						std::lock_guard<std::mutex> lock(clients_mutex);
						clients.erase(std::remove(clients.begin(), clients.end(), socket), clients.end());
					}
				});
			}
		});

		// 主线程处理服务器剪贴板更新
		while (true) {
			std::this_thread::sleep_for(std::chrono::milliseconds(CLIPBOARD_CHECK_INTERVAL));
			
			if (clipboard.has_clipboard_changed()) {
				std::string content = clipboard.get_last_clipboard_content();
				std::cout << "Server clipboard changed (" << content.size() << " bytes)" << std::endl;

				// 广播给所有客户端
				std::lock_guard<std::mutex> lock(clients_mutex);
				for (auto& client : clients) {
					if (client->is_open()) {
						error_code ec;
						asio::write(*client, asio::buffer(content), ec);
					}
				}
			}
		}

		acceptor_thread.join();
		for (auto& t : client_threads) {
			if (t.joinable()) t.join();
		}
	}
	catch (std::exception& e) {
		std::cerr << "Server exception: " << e.what() << std::endl;
	}
}
