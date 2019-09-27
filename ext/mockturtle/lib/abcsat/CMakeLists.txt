cmake_minimum_required (VERSION 3.6)

project(libabcsat LANGUAGES CXX)

include_directories(${PROJECT_SOURCE_DIR})
file(GLOB ABC_SRC *.cpp)

# Surpress warnings in external library
if (UNIX)
    set(CMAKE_CXX_FLAGS "-w")
endif()

if (STATIC_LIBABC)
    add_library(libabcsat STATIC EXCLUDE_FROM_ALL ${ABC_SRC})
    set_property(TARGET libabcsat PROPERTY OUTPUT_NAME libabcsat)
else()
    add_library(libabcsat SHARED EXCLUDE_FROM_ALL ${ABC_SRC})
    set_property(TARGET libabcsat PROPERTY OUTPUT_NAME libabcsat)
    set_property(TARGET libabcsat PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()
target_include_directories(libabcsat INTERFACE ${PROJECT_SOURCE_DIR})
