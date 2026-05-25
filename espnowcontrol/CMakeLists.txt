cmake_minimum_required(VERSION 3.14)
project(espnowcontrol)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(asio.cmake)

add_executable(espnowcontrol main.cpp
                             AddUartDeviceCommand.cpp
                             ConnectionSocket.cpp)

target_include_directories(espnowcontrol PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../)
target_link_libraries(espnowcontrol PRIVATE asio_interface)
