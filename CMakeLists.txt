cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(comic_reader LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

find_package(wxWidgets CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(LibArchive REQUIRED)

add_library(common_lib STATIC
    src/archive.cpp
    src/comic_gallery.cpp
    src/comic_viewer.cpp
    src/comic.cpp
    src/fuzzy.cpp
    src/image_viewer.cpp
    src/viewport.cpp
)
target_link_libraries(common_lib PUBLIC wx::core wx::base wx::ribbon LibArchive::LibArchive)
target_include_directories(common_lib PUBLIC "${CMAKE_SOURCE_DIR}/include")

add_executable(comic_reader WIN32 src/main.cpp)
target_link_libraries(comic_reader PRIVATE common_lib)

if(MSVC)
    target_sources(comic_reader PRIVATE src/main.manifest)
endif()

# Setup Test and coverage
add_executable(tests
    src/archive_test.cpp
    src/fuzzy_test.cpp
)
target_link_libraries(tests PRIVATE common_lib GTest::gtest_main)

if(MSVC)
    add_custom_target(
        coverage
        OpenCppCoverage
        -q --export_type cobertura:${CMAKE_BINARY_DIR}/cov.xml
        --sources $<SHELL_PATH:${CMAKE_SOURCE_DIR}/src>
        --sources $<SHELL_PATH:${CMAKE_SOURCE_DIR}/include>
        --modules $<SHELL_PATH:${CMAKE_BINARY_DIR}>
        --cover_children -- $<TARGET_FILE:tests>
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS tests
    )
else()
    target_compile_options(common_lib PRIVATE -O0 -g --coverage)
    target_compile_options(tests PRIVATE -O0 -g --coverage)
    target_link_libraries(tests PRIVATE --coverage)
    add_custom_target(
        coverage
        $<TARGET_FILE:tests>
        COMMAND gcovr --root ${CMAKE_SOURCE_DIR} --cobertura-pretty --output ${CMAKE_BINARY_DIR}/cov.xml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS tests
    )
endif()
