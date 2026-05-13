cmake_minimum_required(VERSION 3.14)
project(espnowdaemon)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(asio.cmake)
include(json.cmake)

add_executable(espnowdaemon 
    main.cpp
    espnow_uart_device.cpp
    device_manager.cpp
    control_session.cpp
    control_server.cpp
)

target_include_directories(espnowdaemon PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../)
target_link_libraries(espnowdaemon PRIVATE asio_interface)
target_link_libraries(espnowdaemon PRIVATE json_interface)
