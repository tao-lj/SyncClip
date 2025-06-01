#include "client.hpp"
#include <iostream>
#include <thread>

// 添加必要的命名空间引用
using boost::system::error_code;
using boost::system::system_error;

void run_client(const std::string& server_ip, unsigned short port) {
	try {
		asio::io_context io_context;
		tcp::resolver resolver(io_context);
		tcp::resolver::results_type endpoints = resolver.resolve(server_ip, std::to_string(port));

		tcp::socket socket(io_context);
		asio::connect(socket, endpoints);

		ClipboardManager clipboard;

		std::cout << "Connected to clipboard server: "
			<< socket.remote_endpoint().address().to_string()
			<< ":" << socket.remote_endpoint().port() << std::endl;
		std::cout << "Client UUID: " << clipboard.get_uuid_str() << std::endl;

		// 启动接收线程
		std::thread receiver([&socket, &clipboard]() {
			// 使用动态分配的缓冲区
			std::vector<char> recv_buffer(BUFFER_SIZE);
			while (true) {
				error_code ec;
				size_t length = socket.read_some(asio::buffer(recv_buffer), ec);

				if (ec == asio::error::eof) { // 修复这里
					std::cout << "Server disconnected" << std::endl;
					break;
				}
				else if (ec) {
					std::cerr << "Receive error: " << ec.message() << std::endl;
					break;
				}

				std::string received(recv_buffer.data(), length);
				std::cout << "Received clipboard update (" << length << " bytes)" << std::endl;

				// 更新本地剪贴板
				ClipboardData data;
				data.content = received;
				data.mime_type = "text/plain; charset=utf-8";
				clipboard.set_clipboard_content(data);
			}
		});

		// 主线程处理本地剪贴板更新
		while (true) {
			std::this_thread::sleep_for(std::chrono::milliseconds(CLIPBOARD_CHECK_INTERVAL));
			
			if (clipboard.has_clipboard_changed()) {
				std::string content = clipboard.get_last_clipboard_content();
				std::cout << "Local clipboard changed (" << content.size() << " bytes), sending to server" << std::endl;

				// 发送给服务器
				error_code ec;
				asio::write(socket, asio::buffer(content), ec);
				if (ec) {
					std::cerr << "Send error: " << ec.message() << std::endl;
					break;
				}
			}
		}

		socket.close();
		if (receiver.joinable()) {
			receiver.join();
		}
	}
	catch (std::exception& e) {
		std::cerr << "Client exception: " << e.what() << std::endl;
	}
}
