cmake_minimum_required(VERSION 3.16)
project(BlockCache)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_subdirectory(cache)
add_subdirectory(app)
