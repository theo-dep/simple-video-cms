set(back_lib_name back_common)
add_library(${back_lib_name}
    session.cpp
    videosession.cpp
)

target_include_directories(${back_lib_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${back_lib_name} PUBLIC ${common_lib_name})

set(target_name back)
add_executable(${target_name}
    database.cpp
    main.cpp
    server.cpp
    video.cpp
)

target_link_libraries(${target_name} PRIVATE
    ${back_lib_name} PNG::PNG ${FFMPEG_LIBRARIES}
    unofficial::sqlite3::sqlite3 sqlite_orm::sqlite_orm
)

install(TARGETS ${target_name} DESTINATION ${CMAKE_INSTALL_PREFIX})

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
