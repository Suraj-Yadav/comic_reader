# The Flutter tooling requires that developers have CMake 3.10 or later
# installed. You should not increase this version, as doing so will cause
# the plugin to fail to compile for some customers of the plugin.
cmake_minimum_required(VERSION 3.10)

if(APPLE)
  include(FetchContent)

  FetchContent_Declare(
    libarchive
    GIT_REPOSITORY https://github.com/libarchive/libarchive.git
    GIT_TAG v3.6.2
  )

  FetchContent_GetProperties(libarchive)

  if(NOT libarchive_POPULATED)
    # Fetch the content using previously declared details
    FetchContent_Populate(libarchive)

    set(ENABLE_WERROR OFF CACHE BOOL "" FORCE)
    set(ENABLE_TEST OFF CACHE BOOL "" FORCE)
    set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(ENABLE_CAT OFF CACHE BOOL "" FORCE)
    set(ENABLE_TAR OFF CACHE BOOL "" FORCE)
    set(ENABLE_CPIO OFF CACHE BOOL "" FORCE)

    # Bring the populated content into the build
    add_subdirectory(${libarchive_SOURCE_DIR} ${libarchive_BINARY_DIR})
  endif()

  set_target_properties(archive PROPERTIES
    FRAMEWORK TRUE
    FRAMEWORK_VERSION C
    MACOSX_FRAMEWORK_IDENTIFIER org.libarchive

    # "current version" in semantic format in Mach-O binary file
    VERSION 3.6.2

    # "compatibility version" in semantic format in Mach-O binary file
    SOVERSION 1.0.0
    PUBLIC_HEADER ${libarchive_SOURCE_DIR}/archive.h,${libarchive_SOURCE_DIR}/archive_entry.h
  )

  target_compile_definitions(archive PUBLIC DART_SHARED_LIB)
endif()
