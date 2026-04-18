cmake_minimum_required(VERSION 3.26)
project(simple-video-cms LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(httplib REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(inja REQUIRED)
find_package(rapidfuzz REQUIRED)

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
add_subdirectory(front)
