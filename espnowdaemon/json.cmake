include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

add_library(json_interface INTERFACE)
target_include_directories(json_interface INTERFACE ${json_SOURCE_DIR}/include)
