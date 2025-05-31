#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string.hpp>

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace boost::asio;
using namespace boost::system;
using ip::tcp;

// 程序运行模式
enum class Mode { CLIENT, SERVER };

// 全局变量
Mode current_mode;
std::string server_ip;
unsigned short port;
const int BUFFER_SIZE = 1024 * 1024; // 1MB缓冲区，用于处理大剪贴板内容
const int CLIPBOARD_CHECK_INTERVAL = 500; // 剪贴板检查间隔(ms)

// 剪贴板数据结构
struct ClipboardData {
	std::string content;
	std::string mime_type;
	boost::uuids::uuid source_id;
};

// 执行命令行并获取输出
std::string exec_command(const char* cmd) {
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get())) {
		result += buffer.data();
	}
	return result;
}

// 执行命令行不获取输出
void exec_command_no_output(const char* cmd) {
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "w"), pclose);
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}
}

// 剪贴板管理类
class ClipboardManager {
public:
	ClipboardManager() : last_clipboard_hash(0), skip_next_update(false) {
		uuid = boost::uuids::random_generator()();
	}

	// 获取当前剪贴板内容
	ClipboardData get_clipboard_content() {
		ClipboardData data;
		data.source_id = uuid;
		
#ifdef _WIN32
		if (OpenClipboard(nullptr)) {
			if (IsClipboardFormatAvailable(CF_TEXT)) {
				HANDLE hData = GetClipboardData(CF_TEXT);
				if (hData) {
					char* pszText = static_cast<char*>(GlobalLock(hData));
					if (pszText) {
						data.content = pszText;
						data.mime_type = "text/plain";
						GlobalUnlock(hData);
					}
				}
			}
			CloseClipboard();
		}
#else
		try {
			data.content = exec_command("xclip -selection clipboard -o 2>/dev/null");
			data.mime_type = "text/plain";
			
			// 移除可能的多余换行符
			if (!data.content.empty() && data.content.back() == '\n') {
				data.content.pop_back();
			}
		} catch (const std::exception& e) {
			std::cerr << "Error getting clipboard: " << e.what() << std::endl;
		}
#endif
		
		return data;
	}

	// 设置剪贴板内容
	void set_clipboard_content(const ClipboardData& data) {
		// 如果内容来自自己，则跳过
		if (data.source_id == uuid) {
			return;
		}

		// 设置跳过标志，避免循环同步
		skip_next_update = true;
		last_clipboard_content = data.content;
		last_clipboard_hash = std::hash<std::string>{}(data.content);

#ifdef _WIN32
		if (OpenClipboard(nullptr)) {
			EmptyClipboard();
			HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE, data.content.size() + 1);
			if (hClipboardData) {
				char* pchData = static_cast<char*>(GlobalLock(hClipboardData));
				if (pchData) {
					strcpy(pchData, data.content.c_str());
					GlobalUnlock(hClipboardData);
					SetClipboardData(CF_TEXT, hClipboardData);
				}
			}
			CloseClipboard();
		}
#else
		try {
			// 使用临时文件设置剪贴板
			FILE* tmp = tmpfile();
			if (tmp) {
				fwrite(data.content.c_str(), 1, data.content.size(), tmp);
				rewind(tmp);
				
				std::string cmd = "xclip -selection clipboard -i";
				std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "w"), pclose);
				if (pipe) {
					fwrite(data.content.c_str(), 1, data.content.size(), pipe.get());
				}
				fclose(tmp);
			}
		} catch (const std::exception& e) {
			std::cerr << "Error setting clipboard: " << e.what() << std::endl;
		}
#endif
	}

	// 检查剪贴板是否有更新
	bool has_clipboard_changed() {
		if (skip_next_update) {
			skip_next_update = false;
			return false;
		}

		ClipboardData data = get_clipboard_content();
		size_t new_hash = std::hash<std::string>{}(data.content);
		
		// 空内容不算更新
		if (data.content.empty()) {
			return false;
		}
		
		if (new_hash != last_clipboard_hash) {
			last_clipboard_hash = new_hash;
			last_clipboard_content = data.content;
			return true;
		}
		return false;
	}

	// 获取最后已知的剪贴板内容
	std::string get_last_clipboard_content() const {
		return last_clipboard_content;
	}

	// 获取当前客户端的UUID
	std::string get_uuid_str() const {
		return boost::uuids::to_string(uuid);
	}

private:
	boost::uuids::uuid uuid;
	size_t last_clipboard_hash;
	std::string last_clipboard_content;
	bool skip_next_update;
};

// 解析命令行参数
bool parse_arguments(int argc, char* argv[]) {
	if (argc != 4) {
		std::cerr << "Usage: " << argv[0] << " <-c|-s> <ip> <port>\n";
		std::cerr << "  -c: client mode\n";
		std::cerr << "  -s: server mode\n";
		return false;
	}

	std::string mode_str = argv[1];
	if (mode_str == "-c") {
		current_mode = Mode::CLIENT;
	} else if (mode_str == "-s") {
		current_mode = Mode::SERVER;
	} else {
		std::cerr << "Invalid mode. Use -c for client or -s for server.\n";
		return false;
	}

	server_ip = argv[2];
	port = static_cast<unsigned short>(std::stoi(argv[3]));

	return true;
}

// 服务器端实现
void run_server() {
	try {
		io_context io_context;
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
						char recv_buffer[BUFFER_SIZE];
						while (true) {
							error_code ec;
							size_t length = socket->read_some(boost::asio::buffer(recv_buffer), ec);
							
							if (ec == error::eof) {
								std::cout << "Client disconnected" << std::endl;
								break;
							} else if (ec) {
								throw system_error(ec);
							}

							std::string received(recv_buffer, length);
							std::cout << "Received clipboard content (" << length << " bytes)" << std::endl;

							// 更新服务器剪贴板
							ClipboardData data;
							data.content = received;
							data.mime_type = "text/plain";
							clipboard.set_clipboard_content(data);

							// 广播给所有其他客户端
							std::lock_guard<std::mutex> lock(clients_mutex);
							for (auto& client : clients) {
								if (client != socket && client->is_open()) {
									try {
										write(*client, boost::asio::buffer(received), ec);
									} catch (...) {
										// 忽略发送失败的客户端
									}
								}
							}
						}
					} catch (std::exception& e) {
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
						write(*client, boost::asio::buffer(content), ec);
					}
				}
			}
		}

		acceptor_thread.join();
		for (auto& t : client_threads) {
			if (t.joinable()) t.join();
		}
	} catch (std::exception& e) {
		std::cerr << "Server exception: " << e.what() << std::endl;
	}
}

// 客户端实现
void run_client() {
	try {
		io_context io_context;
		tcp::resolver resolver(io_context);
		tcp::resolver::results_type endpoints = resolver.resolve(server_ip, std::to_string(port));

		tcp::socket socket(io_context);
		connect(socket, endpoints);

		ClipboardManager clipboard;
		
		std::cout << "Connected to clipboard server: " 
				  << socket.remote_endpoint().address().to_string() 
				  << ":" << socket.remote_endpoint().port() << std::endl;
		std::cout << "Client UUID: " << clipboard.get_uuid_str() << std::endl;

		// 启动接收线程
		std::thread receiver([&socket, &clipboard]() {
			char recv_buffer[BUFFER_SIZE];
			while (true) {
				error_code ec;
				size_t length = socket.read_some(boost::asio::buffer(recv_buffer), ec);
				
				if (ec == error::eof) {
					std::cout << "Server disconnected" << std::endl;
					break;
				} else if (ec) {
					std::cerr << "Receive error: " << ec.message() << std::endl;
					break;
				}

				std::string received(recv_buffer, length);
				std::cout << "Received clipboard update (" << length << " bytes)" << std::endl;

				// 更新本地剪贴板
				ClipboardData data;
				data.content = received;
				data.mime_type = "text/plain";
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
				write(socket, boost::asio::buffer(content), ec);
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
	} catch (std::exception& e) {
		std::cerr << "Client exception: " << e.what() << std::endl;
	}
}

int main(int argc, char* argv[]) {
	if (!parse_arguments(argc, argv)) {
		return 1;
	}

	if (current_mode == Mode::SERVER) {
		run_server();
	} else {
		run_client();
	}

	return 0;
}
