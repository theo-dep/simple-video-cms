cmake_minimum_required(VERSION 3.26)
project(simple-video-cms LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(is_multi_config)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
endif()

find_package(httplib REQUIRED)
find_package(nlohmann_json REQUIRED)

find_package(unofficial-sqlite3 REQUIRED)
find_package(SqliteOrm REQUIRED)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(FFMPEG REQUIRED COMPONENTS avformat avdevice avcodec avfilter avutil swresample swscale)

add_compile_options(-Wall -Wextra -Werror -pedantic)
add_link_options(-static)
link_libraries(stdc++exp)

add_subdirectory(common)
add_subdirectory(back)
