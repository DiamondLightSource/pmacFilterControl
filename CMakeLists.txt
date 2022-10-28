cmake_minimum_required(VERSION 3.23) # For `CMAKE_C*_STANDARD*` and CMakePresets.json

project(pmacFilterControl)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${pmacFilterControl_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set(CMAKE_CXX_FLAGS "-Wall -Werror")

set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(ZEROMQ 4.1.4 REQUIRED)
include_directories(${ZEROMQ_INCLUDE_DIRS}) # Non-system ZeroMQ (C library)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty) # CPP ZeroMQ bindings

if(EXISTS "/root/lib/ld-linux-armhf.so.3")
    set(ARM TRUE)
    message(STATUS "ARM Build")
endif()

if(ARM)
    # ppmac libs
    include_directories(${CMAKE_SOURCE_DIR}/thirdparty/ppmac/libppmac)
    include_directories(${CMAKE_SOURCE_DIR}/thirdparty/ppmac/rtpmac)
    include_directories(${CMAKE_SOURCE_DIR}/thirdparty/ppmac/libopener)
    link_directories(${CMAKE_SOURCE_DIR}/thirdparty/ppmac/libppmac)
    link_directories(${CMAKE_SOURCE_DIR}/thirdparty/ppmac/libmath)
    link_directories(${CMAKE_SOURCE_DIR}/thirdparty/ppmac/libopener)
    link_directories(${CMAKE_SOURCE_DIR}/thirdparty/xenomai)
endif()

add_subdirectory(src)
add_subdirectory(thirdparty)

if(NOT ARM)
    add_subdirectory(docs)
    add_subdirectory(test)
endif()
