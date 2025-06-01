#include "clipboard_manager.hpp"
#include "../utils/command_utils.hpp"
#include <iostream>
#include <codecvt>
#include <locale>
#include <cstring>
#include <functional> // for std::hash

#ifdef _WIN32
#include <Windows.h>
#endif

ClipboardManager::ClipboardManager() : last_clipboard_hash(0), skip_next_update(false) {
	uuid = boost::uuids::random_generator()();
}

ClipboardData ClipboardManager::get_clipboard_content() {
	ClipboardData data;
	data.source_id = uuid;
	
#ifdef _WIN32
	if (OpenClipboard(nullptr)) {
		std::string clipboard_text;
		
		// 优先尝试获取Unicode文本
		if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
			HANDLE hData = GetClipboardData(CF_UNICODETEXT);
			if (hData) {
				wchar_t* wstr = static_cast<wchar_t*>(GlobalLock(hData));
				if (wstr) {
					clipboard_text = wstring_to_utf8(wstr);
					GlobalUnlock(hData);
				}
			}
		}
		// 如果没有Unicode文本，尝试获取ANSI文本
		else if (IsClipboardFormatAvailable(CF_TEXT)) {
			HANDLE hData = GetClipboardData(CF_TEXT);
			if (hData) {
				char* cstr = static_cast<char*>(GlobalLock(hData));
				if (cstr) {
					// 将ANSI文本转换为UTF-8
					std::wstring wstr;
					int size = MultiByteToWideChar(CP_ACP, 0, cstr, -1, NULL, 0);
					if (size > 0) {
						wstr.resize(size);
						MultiByteToWideChar(CP_ACP, 0, cstr, -1, &wstr[0], size);
					}
					clipboard_text = wstring_to_utf8(wstr);
					GlobalUnlock(hData);
				}
			}
		}
		
		if (!clipboard_text.empty()) {
			data.content = clipboard_text;
			data.mime_type = "text/plain; charset=utf-8";
		}
		
		CloseClipboard();
	}
#else
	try {
		// 使用xclip获取剪贴板内容，并指定UTF-8编码
		data.content = exec_command("xclip -selection clipboard -o 2>/dev/null");
		data.mime_type = "text/plain; charset=utf-8";
		
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

void ClipboardManager::set_clipboard_content(const ClipboardData& data) {
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
		
		// 转换为宽字符（UTF-16）
		std::wstring wstr = utf8_to_wstring(data.content);
		
		// 分配内存并设置剪贴板数据
		HGLOBAL hClipboardData = GlobalAlloc(GMEM_MOVEABLE, (wstr.size() + 1) * sizeof(wchar_t));
		if (hClipboardData) {
			wchar_t* pchData = static_cast<wchar_t*>(GlobalLock(hClipboardData));
			if (pchData) {
				wcscpy_s(pchData, wstr.size() + 1, wstr.c_str());
				GlobalUnlock(hClipboardData);
				SetClipboardData(CF_UNICODETEXT, hClipboardData);
			}
		}
		
		CloseClipboard();
	}
#else
	try {
		// 使用管道写入xclip，确保UTF-8编码
		FILE* pipe = POPEN("xclip -selection clipboard -i", "w");
		if (pipe) {
			fwrite(data.content.c_str(), 1, data.content.size(), pipe);
			PCLOSE(pipe);
		}
	} catch (const std::exception& e) {
		std::cerr << "Error setting clipboard: " << e.what() << std::endl;
	}
#endif
}

bool ClipboardManager::has_clipboard_changed() {
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

std::string ClipboardManager::get_last_clipboard_content() const {
	return last_clipboard_content;
}

std::string ClipboardManager::get_uuid_str() const {
	return boost::uuids::to_string(uuid);
}
