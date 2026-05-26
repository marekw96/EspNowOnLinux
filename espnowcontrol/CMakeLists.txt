cmake_minimum_required(VERSION 3.14)
project(espnowcontrol)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(asio.cmake)
include(protobuf.cmake)

add_executable(espnowcontrol main.cpp
                             AddUartDeviceCommand.cpp
                             ConnectionSocket.cpp)

protobuf_generate(TARGET espnowcontrol
                  LANGUAGE cpp
                  IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../messages
                  PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/../messages/control_messages.proto)

target_include_directories(espnowcontrol PRIVATE 
                            ${CMAKE_CURRENT_SOURCE_DIR}/../
                            ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(espnowcontrol PRIVATE asio_interface protobuf::libprotobuf)
