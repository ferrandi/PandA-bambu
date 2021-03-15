include_directories(catch2) # v2.2.1

file(GLOB_RECURSE FILENAMES *.cpp)

add_executable(run_tests ${FILENAMES})
target_link_libraries(run_tests mockturtle)
if (ENABLE_COVERAGE)
  target_link_libraries(run_tests gcov)
endif()
target_compile_definitions(run_tests PUBLIC BENCHMARKS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/benchmarks")
target_compile_definitions(run_tests PUBLIC CATCH_CONFIG_CONSOLE_WIDTH=300)
