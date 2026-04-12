cmake_minimum_required(VERSION 3.10)
project(host_app)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(host_read host_read.cpp)
