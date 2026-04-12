cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(PROJ_NAME "$ENV{BUILD_APP}")
if("${PROJ_NAME}" STREQUAL "")
    set(PROJ_NAME "hello_world")
endif()
project(${PROJ_NAME})
