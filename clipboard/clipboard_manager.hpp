#pragma once
#include <string>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "../config/config.hpp"

// 剪贴板数据结构
struct ClipboardData {
	std::string content;
	std::string mime_type;
	boost::uuids::uuid source_id;
};

class ClipboardManager {
public:
	ClipboardManager();
	ClipboardData get_clipboard_content();
	void set_clipboard_content(const ClipboardData& data);
	bool has_clipboard_changed();
	std::string get_last_clipboard_content() const;
	std::string get_uuid_str() const;

private:
	boost::uuids::uuid uuid;
	size_t last_clipboard_hash;
	std::string last_clipboard_content;
	bool skip_next_update;
};
