#pragma once

#include <string>

// 程序运行模式
enum class Mode { CLIENT, SERVER };

// 全局常量配置
const int BUFFER_SIZE = 64 * 1024; // 64KB缓冲区
const int CLIPBOARD_CHECK_INTERVAL = 500; // 剪贴板检查间隔(ms)

// 参数解析函数
bool parse_arguments(int argc, char* argv[], Mode& mode, std::string& server_ip, unsigned short& port);
