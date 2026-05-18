cmake_minimum_required(VERSION 3.10)

add_library(utility INTERFACE)
target_include_directories(utility INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT ESP_PLATFORM)
    add_subdirectory(test)
endif()
