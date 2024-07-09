cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

include(custom.cmake)
parse_git_tag()

project(comic_reader LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS
    ON
    CACHE INTERNAL "")

find_package(LibArchive REQUIRED)
find_package(Tweeny CONFIG REQUIRED)
find_package(WebP CONFIG REQUIRED)
find_package(wxWidgets CONFIG REQUIRED)

add_library(
  common_lib STATIC
  src/archive.cpp
  src/comic_gallery.cpp
  src/comic_viewer.cpp
  src/comic.cpp
  src/fuzzy.cpp
  src/image_utils.cpp
  src/viewport.cpp
  src/wxUtil.cpp)

target_link_libraries(common_lib PUBLIC LibArchive::LibArchive tweeny
                                        WebP::webp wx::core)

target_include_directories(common_lib PUBLIC "${CMAKE_SOURCE_DIR}/include")

add_executable(comic_reader WIN32 src/main.cpp)
target_link_libraries(comic_reader PRIVATE common_lib)

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  target_sources(comic_reader PRIVATE resource/main.rc resource/main.manifest)
endif()

set(CPACK_GENERATOR "ZIP")
install(
  TARGETS comic_reader
          RUNTIME_DEPENDENCIES
          PRE_EXCLUDE_REGEXES
          "api-ms-"
          "ext-ms-"
          "hvsifiletrust"
          "pdmutilities"
          POST_EXCLUDE_REGEXES
          ".*system32/.*\\.dll"
  RUNTIME DESTINATION .)
set(CPACK_PACKAGE_VERSION ${VERSION_STRING})
include(CPack)

# Setup Test, coverage and benchmarks
find_package(benchmark CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

set(TEST_SRCS src/archive_test.cpp src/comic_test.cpp src/fuzzy_test.cpp
              src/lru_test.cpp)

add_executable(tests ${TEST_SRCS})
target_link_libraries(tests PRIVATE common_lib GTest::gtest
                                    benchmark::benchmark GTest::gtest_main)

add_executable(benchmarks ${TEST_SRCS})
target_link_libraries(
  benchmarks PRIVATE common_lib GTest::gtest benchmark::benchmark
                     benchmark::benchmark_main)

if(MSVC)
  add_custom_target(
    coverage
    OpenCppCoverage
    -q
    --export_type
    cobertura:${CMAKE_BINARY_DIR}/cov.xml
    --sources
    $<SHELL_PATH:${CMAKE_SOURCE_DIR}/src>
    --sources
    $<SHELL_PATH:${CMAKE_SOURCE_DIR}/include>
    --modules
    $<SHELL_PATH:${CMAKE_BINARY_DIR}>
    --cover_children
    --
    $<TARGET_FILE:tests>
    --gtest_output=xml:${CMAKE_BINARY_DIR}/test.xml
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS tests)
endif()
