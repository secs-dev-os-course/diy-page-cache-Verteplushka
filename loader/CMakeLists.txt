get_filename_component(MONOLITH_INCLUDE_PATH "." ABSOLUTE)
get_filename_component(MONOLITH_SOURCE_PATH "./${PROJECT_NAME}" ABSOLUTE)

file(GLOB_RECURSE MONOLITH_SOURCES CONFIGURE_DEPENDS EmaSearchString.hpp EmaSearchString.cpp)

set(ALL_SOURCES ${MONOLITH_SOURCES})

add_library(${PROJECT_NAME} STATIC ${ALL_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${MONOLITH_INCLUDE_PATH})

set(EMA_SEARCH_STRING_APP app/EmaSearchStringApp.cpp)
add_executable(emaSearchStringApp ${EMA_SEARCH_STRING_APP})
set_property(TARGET emaSearchStringApp PROPERTY OUTPUT_NAME "emaSearchStringApp")
target_compile_options(emaSearchStringApp PRIVATE /Od)
target_link_libraries(emaSearchStringApp PRIVATE ${PROJECT_NAME})
