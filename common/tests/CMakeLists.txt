set(target_name common_tests)
add_executable(${target_name}
	test_cookie.cpp
	test_stringutils.cpp
)

target_link_libraries(${target_name} PRIVATE
	${common_lib_name}
	Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(${target_name})
