set(target_name ${common_lib_name})
add_library(${target_name}
    cookie.cpp
    crypto.cpp
    filesystem.cpp
    logging.cpp
    servercommon.cpp
    stringutils.cpp
)

target_include_directories(${target_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set(payload_max_length 1610612736) # 1.5 GB

target_compile_definitions(${target_name} PUBLIC
    $<$<CONFIG:Debug>:_DEBUG DEBUG_LOG>
    $<$<AND:$<CONFIG:Release>,$<BOOL:${NO_SECURE}>>:NO_SECURE>
    CPPHTTPLIB_USE_POLL CPPHTTPLIB_ZLIB_SUPPORT
    CPPHTTPLIB_PAYLOAD_MAX_LENGTH=${payload_max_length}
    CPPHTTPLIB_FORM_URL_ENCODED_PAYLOAD_MAX_LENGTH=${payload_max_length}
)

target_link_libraries(${target_name} PUBLIC httplib::httplib ZLIB::ZLIB)
target_link_libraries(${target_name} PRIVATE OpenSSL::Crypto)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
