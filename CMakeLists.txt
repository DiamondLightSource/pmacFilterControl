project(pmacFilterControl)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_MODULE_PATH ${pmacFilterControl_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

find_package(ZEROMQ 4.1.4 REQUIRED)
include_directories(${ZEROMQ_INCLUDE_DIRS}) # Non-system ZeroMQ (C library)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty) # CPP ZeroMQ bindings

# ppmac libs
include_directories(${CMAKE_SOURCE_DIR}/thirdparty/ppmac/libppmac)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty/ppmac/rtpmac)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty/ppmac/libopener)
link_directories(${CMAKE_SOURCE_DIR}/thirdparty/ppmac/libppmac)
link_directories(${CMAKE_SOURCE_DIR}/thirdparty/ppmac/libmath)
link_directories(${CMAKE_SOURCE_DIR}/thirdparty/ppmac/libopener)
link_directories(${CMAKE_SOURCE_DIR}/thirdparty/xenomai)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(thirdparty)
