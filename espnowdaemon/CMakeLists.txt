cmake_minimum_required(VERSION 3.14)
project(espnowdaemon)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(asio.cmake)

add_executable(espnowdaemon main.cpp)

target_include_directories(espnowdaemon PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../)
target_link_libraries(espnowdaemon PRIVATE asio_interface)
