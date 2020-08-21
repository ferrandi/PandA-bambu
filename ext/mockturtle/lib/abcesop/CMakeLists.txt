cmake_minimum_required (VERSION 3.6)

project(libabcesop LANGUAGES CXX)

include_directories(${PROJECT_SOURCE_DIR})
file(GLOB ABC_SRC *.cpp)

# Surpress warnings in external library
if (UNIX)
    add_compile_options("-w")
elseif (MSVC)
    string(REPLACE "/W3" "/w" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
endif()

if (STATIC_LIBABC)
    add_library(libabcesop STATIC EXCLUDE_FROM_ALL ${ABC_SRC})
    set_property(TARGET libabcesop PROPERTY OUTPUT_NAME libabcesop)
else()
    add_library(libabcesop SHARED EXCLUDE_FROM_ALL ${ABC_SRC})
    set_property(TARGET libabcesop PROPERTY OUTPUT_NAME libabcesop)
    set_property(TARGET libabcesop PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()
target_include_directories(libabcesop INTERFACE ${PROJECT_SOURCE_DIR})

if (UNIX)
    target_compile_definitions(libabcesop PUBLIC "LIN64" ABC_NO_USE_READLINE)
elseif(WIN32)
    target_compile_definitions(libabcesop PUBLIC ABC_USE_NO_READLINE NOMINMAX  WIN32_NO_DLL _CRT_SECURE_NO_WARNINGS)
endif()
