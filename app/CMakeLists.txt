add_executable(app main.cpp)
target_link_libraries(app cache)
target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../cache)


add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/cache/Debug/cache.dll ${CMAKE_BINARY_DIR}/app/Debug/cache.dll
    COMMENT "Moving cache.dll to app/Debug"
)
