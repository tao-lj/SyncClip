cmake_minimum_required(VERSION 3.30)
project(SyncClip)

# 设置 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 查找 Boost 组件
find_package(Boost REQUIRED)

# 查找 X11 库（Linux 平台）
if(UNIX AND NOT APPLE)
	find_package(X11 REQUIRED)
endif()

# 添加可执行文件
add_executable(SyncClip ./src/main.cpp)

# 平台特定链接
if(WIN32)
	# Windows 平台链接
	target_link_libraries(SyncClip PRIVATE ws2_32 gdi32)
elseif(UNIX AND NOT APPLE)
	# Linux 平台链接
	target_link_libraries(SyncClip PRIVATE ${X11_LIBRARIES})
	include_directories(${X11_INCLUDE_DIR})
endif()

# 添加编译器定义
if(UNIX AND NOT APPLE)
	target_compile_definitions(SyncClip PRIVATE LINUX_CLIPBOARD)
endif()

# 设置安装目标
install(TARGETS SyncClip RUNTIME DESTINATION bin)
