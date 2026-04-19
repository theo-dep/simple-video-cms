set(target_name back)
add_executable(${target_name}
    database.cpp
    logcontroller.cpp
    main.cpp
    search.cpp
    server.cpp
    video.cpp
)

target_link_libraries(${target_name} PRIVATE
    common rapidfuzz::rapidfuzz PNG::PNG ${FFMPEG_LIBRARIES}
    unofficial::sqlite3::sqlite3 sqlite_orm::sqlite_orm
)

add_custom_target(copy_back_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_SOURCE_DIR}/static $<TARGET_FILE_DIR:${target_name}>/static
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_SOURCE_DIR}/templates $<TARGET_FILE_DIR:${target_name}>/templates
    COMMENT "Copying back assets"
)
add_dependencies(${target_name} copy_back_assets)

install(TARGETS ${target_name} DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY static DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY templates DESTINATION ${CMAKE_INSTALL_PREFIX})
