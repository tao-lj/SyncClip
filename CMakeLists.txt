cmake_minimum_required(VERSION 3.30)
project(SyncClip)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost REQUIRED)

add_executable(SyncClip 
	main.cpp
	config/config.cpp
	config/config_manager.cpp
	utils/command_utils.cpp
	clipboard/clipboard_manager.cpp
	network/server.cpp
	network/client.cpp
)

if(WIN32)
	target_link_libraries(SyncClip PRIVATE ws2_32)
elseif(UNIX AND NOT APPLE)
	target_link_libraries(SyncClip ${Boost_LIBRARIES} pthread)
endif()

