set(target_name front)
add_executable(${target_name}
    client.cpp
    confirmhandler.cpp
    cookie.cpp
    main.cpp
    server.cpp
    session.cpp
    videosession.cpp
)

target_link_libraries(${target_name} PRIVATE common)

install(TARGETS ${target_name} DESTINATION ${CMAKE_INSTALL_PREFIX})
