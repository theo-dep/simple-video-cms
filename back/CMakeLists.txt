add_library(sqlite3-lib OBJECT ${CMAKE_SOURCE_DIR}/common/third-party/sqlite3.c)

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
    $<TARGET_OBJECTS:sqlite3-lib>
    common PNG::PNG ${FFMPEG_LIBRARIES}
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
