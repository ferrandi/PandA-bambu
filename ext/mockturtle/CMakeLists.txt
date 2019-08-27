cmake_minimum_required(VERSION 3.8)
project(mockturtle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(MOCKTURTLE_EXAMPLES "Build examples" ON)
option(MOCKTURTLE_TEST "Build tests" OFF)
option(MOCKTURTLE_EXPERIMENTS "Build experiments" OFF)

if(UNIX)
  # show quite some warnings (but remove some intentionally)
  include(CheckCXXCompilerFlag)
  add_compile_options(-W -Wall -Wextra)
  foreach (WARNING unknown-pragmas gnu-anonymous-struct nested-anon-types)
    check_cxx_compiler_flag("-Wno-${WARNING}" HAS_WNO_${WARNING})
    if (HAS_WNO_${WARNING})
      add_compile_options(-Wno-${WARNING})
    endif()
  endforeach()
endif()

add_subdirectory(include)
add_subdirectory(lib)

if(MOCKTURTLE_EXAMPLES)
  add_subdirectory(examples)
endif()

if(MOCKTURTLE_TEST)
  add_subdirectory(test)
endif()

if(MOCKTURTLE_EXPERIMENTS)
  add_subdirectory(experiments)
endif()
