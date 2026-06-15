cmake_minimum_required(VERSION 3.10)
project(utility LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(utility STATIC event_dispatcher.cpp)
target_include_directories(utility PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT ESP_PLATFORM)
    add_subdirectory(test)
endif()
