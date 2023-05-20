cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(comic_reader LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

find_package(wxWidgets CONFIG REQUIRED)
find_package(cxxopts REQUIRED)

add_executable(comic_reader WIN32 src/main.cpp src/image_viewer.cpp src/viewport.cpp)

if(MSVC)
    target_sources(comic_reader PRIVATE src/main.manifest)
endif()

target_link_libraries(comic_reader PRIVATE wx::core wx::base)
target_include_directories(comic_reader PRIVATE "${CMAKE_SOURCE_DIR}/include")
