set(target_name back_tests)
add_executable(${target_name}
    test_session.cpp
    test_videosession.cpp
)

target_link_libraries(${target_name} PRIVATE
    ${back_lib_name}
    Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(${target_name})
