get_filename_component(MONOLITH_INCLUDE_PATH "." ABSOLUTE)
get_filename_component(MONOLITH_SOURCE_PATH "./${PROJECT_NAME}" ABSOLUTE)

file(GLOB_RECURSE MONOLITH_SOURCES CONFIGURE_DEPENDS ema_search_string.hpp ema_search_string.cpp ema_search_string_cache.hpp ema_search_string_cache.cpp)

set(ALL_SOURCES ${MONOLITH_SOURCES})

add_library(${PROJECT_NAME} STATIC ${ALL_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${MONOLITH_INCLUDE_PATH})

set(EMA_SEARCH_STRING_APP app/ema_search_string_app.cpp)
add_executable(emaSearchStringApp ${EMA_SEARCH_STRING_APP})
set_property(TARGET emaSearchStringApp PROPERTY OUTPUT_NAME "emaSearchStringApp")
target_link_libraries(emaSearchStringApp PRIVATE ${PROJECT_NAME})

set(EMA_SEARCH_STRING_CACHE_APP app/ema_search_string_cache_app.cpp)
add_executable(emaSearchStringCacheApp ${EMA_SEARCH_STRING_CACHE_APP})
set_property(TARGET emaSearchStringCacheApp PROPERTY OUTPUT_NAME "emaSearchStringCacheApp")
target_link_libraries(emaSearchStringCacheApp PRIVATE ${PROJECT_NAME} cache)

target_include_directories(emaSearchStringCacheApp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../cache)
add_custom_command(TARGET emaSearchStringCacheApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/cache/Debug/cache.dll ${CMAKE_BINARY_DIR}/loader/Debug/cache.dll
    COMMENT "Moving cache.dll to loader/Debug"
)
