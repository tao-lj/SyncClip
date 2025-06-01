cmake_minimum_required(VERSION 3.30)
project(SyncClip)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost REQUIRED)

add_executable(SyncClip 
	src/main.cpp
	src/config/config.cpp
	src/config/config_manager.cpp
	src/utils/command_utils.cpp
	src/clipboard/clipboard_manager.cpp
	src/network/server.cpp
	src/network/client.cpp
)

if(WIN32)
	target_link_libraries(SyncClip PRIVATE ws2_32)
elseif(UNIX AND NOT APPLE)
	target_link_libraries(SyncClip ${Boost_LIBRARIES} pthread)
endif()

