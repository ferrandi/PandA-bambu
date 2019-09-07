include_directories(catch2) # v2.2.1

file(GLOB_RECURSE FILENAMES *.cpp)

add_executable(run_tests ${FILENAMES})
target_link_libraries(run_tests mockturtle)
target_compile_definitions(run_tests PUBLIC BENCHMARKS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/benchmarks")
