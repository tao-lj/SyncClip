cmake_minimum_required(VERSION 3.30)
project(SyncClip)

# 设置 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 查找 Boost 组件
find_package(Boost REQUIRED)

# 添加可执行文件
add_executable(SyncClip 
	main.cpp
	config/config.cpp
	utils/command_utils.cpp
	clipboard/clipboard_manager.cpp
	network/server.cpp
	network/client.cpp
)

# 平台特定链接
if(WIN32)
	# Windows 平台链接
	target_link_libraries(SyncClip PRIVATE ws2_32)
elseif(UNIX AND NOT APPLE)
	# Linux 平台链接
	target_link_libraries(SyncClip ${Boost_LIBRARIES} pthread)
endif()

