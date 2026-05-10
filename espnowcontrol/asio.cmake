include(FetchContent)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
)
FetchContent_MakeAvailable(asio)

add_library(asio_interface INTERFACE)
target_include_directories(asio_interface INTERFACE ${asio_SOURCE_DIR}/asio/include)
target_compile_definitions(asio_interface INTERFACE ASIO_STANDALONE)

find_package(Threads REQUIRED)
target_link_libraries(asio_interface INTERFACE Threads::Threads)
