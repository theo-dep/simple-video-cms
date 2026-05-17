set(target_name back)
add_executable(${target_name}
    database.cpp
    main.cpp
    server.cpp
    session.cpp
    video.cpp
    videosession.cpp
)

target_link_libraries(${target_name} PRIVATE
    common PNG::PNG ${FFMPEG_LIBRARIES}
    unofficial::sqlite3::sqlite3 sqlite_orm::sqlite_orm
)

install(TARGETS ${target_name} DESTINATION ${CMAKE_INSTALL_PREFIX})
