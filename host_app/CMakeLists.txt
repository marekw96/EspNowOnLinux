cmake_minimum_required(VERSION 3.10)
project(host_app)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(host_read host_read.cpp serial_port.cpp tap_device.cpp)

target_include_directories(host_read PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../)
