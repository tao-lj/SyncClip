#pragma once
#include <string>

#ifdef _WIN32
#include <Windows.h>
#include <fcntl.h>
#include <io.h>
#define POPEN _popen
#define PCLOSE _pclose
#else
#include <cstdio> // 添加这个头文件
#define POPEN popen
#define PCLOSE pclose
#endif

// 执行命令并获取输出
std::string exec_command(const char* cmd);

// Windows编码转换函数
#ifdef _WIN32
std::string wstring_to_utf8(const std::wstring& wstr);
std::wstring utf8_to_wstring(const std::string& str);
#endif
