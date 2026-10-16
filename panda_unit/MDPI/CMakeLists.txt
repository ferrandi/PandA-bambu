foreach(_mdpi_control IN ITEMS banked_memory wrapper_tail)
   add_executable(mdpi_${_mdpi_control}_test ${_mdpi_control}.cpp)
   target_include_directories(mdpi_${_mdpi_control}_test PRIVATE "${PROJECT_SOURCE_DIR}/etc/libmdpi/include")
   add_test(NAME mdpi_${_mdpi_control} COMMAND mdpi_${_mdpi_control}_test)
   set_tests_properties(mdpi_${_mdpi_control} PROPERTIES LABELS "mdpi;frontend")
endforeach()

# The comparison macros are tested through their production header in C simulation mode.
target_compile_definitions(mdpi_wrapper_tail_test PRIVATE BAMBU_CSIM)
