#include "command_utils.hpp"
#include <stdexcept>
#include <array>
#include <memory>
#include <cstdio>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif

std::string exec_command(const char* cmd) {
	std::array<char, 128> buffer;
	std::string result;
	
	FILE* pipe = POPEN(cmd, "r");
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}
	
	try {
		while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
			result += buffer.data();
		}
	} catch (...) {
		PCLOSE(pipe);
		throw;
	}
	
	PCLOSE(pipe);
	return result;
}

#ifdef _WIN32
// Windows UTF-8 转换函数
std::string wstring_to_utf8(const std::wstring& wstr) {
	if (wstr.empty()) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string str(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);
	return str;
}

std::wstring utf8_to_wstring(const std::string& str) {
	if (str.empty()) return std::wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstr(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
	return wstr;
}
#endif
