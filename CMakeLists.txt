project(pmacFilterControl)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_MODULE_PATH ${pmacFilterControl_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
find_package(ZEROMQ 4.1.4 REQUIRED)

include_directories(${ZEROMQ_INCLUDE_DIRS})  # Needed to find non-system ZeroMQ C library
include_directories(thirdparty)

add_subdirectory(src)
add_subdirectory(test)
