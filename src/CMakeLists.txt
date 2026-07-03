file(GLOB_RECURSE PANDA_ALL_SOURCES
   "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Exclude mains and standalone tests from the core library
set(PANDA_EXCLUDE_PATTERNS
   "/bambu.cpp"
   "/bambu-cc.cpp"
   "/eucalyptus.cpp"
   "/algorithms/clique_covering/test_"
   "/HLS/hdl_generators/"
)

foreach(pattern IN LISTS PANDA_EXCLUDE_PATTERNS)
   foreach(src IN LISTS PANDA_ALL_SOURCES)
      string(FIND "${src}" "${pattern}" _idx)
      if(NOT _idx EQUAL -1)
         list(REMOVE_ITEM PANDA_ALL_SOURCES "${src}")
      endif()
   endforeach()
endforeach()

add_library(panda_core STATIC ${PANDA_ALL_SOURCES})

set(BAMBU_PARAM_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bambu_parameters_registry")
set(BAMBU_PARAM_REGISTRY_FILES
   "${BAMBU_PARAM_OUTPUT_DIR}/registry_all.cpp"
   "${BAMBU_PARAM_OUTPUT_DIR}/registry_algorithms.cpp"
   "${BAMBU_PARAM_OUTPUT_DIR}/registry_core.cpp"
   "${BAMBU_PARAM_OUTPUT_DIR}/registry_design_flows.cpp"
   "${BAMBU_PARAM_OUTPUT_DIR}/registry_frontend_analysis.cpp"
   "${BAMBU_PARAM_OUTPUT_DIR}/registry_hls.cpp"
   "${BAMBU_PARAM_OUTPUT_DIR}/registry_technology.cpp"
   "${BAMBU_PARAM_OUTPUT_DIR}/registry_wrapper.cpp"
)
set_source_files_properties(${BAMBU_PARAM_REGISTRY_FILES} PROPERTIES GENERATED TRUE)

file(GLOB_RECURSE BAMBU_PARAM_INPUTS
   "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
list(FILTER BAMBU_PARAM_INPUTS EXCLUDE REGEX ".*/bambu_parameters_registry/.*\\.cpp$")
file(GLOB_RECURSE BAMBU_PARAM_DEVICE_XML
   "${CMAKE_SOURCE_DIR}/etc/libtech/*/*-seed.xml"
)

set(BAMBU_PARAM_STAMP "${CMAKE_BINARY_DIR}/bambu_parameters_registry.stamp")
set(_bambu_param_registry_cmd
   "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/etc/scripts/generate_bambu_parameter_registry.py"
   --input "${CMAKE_BINARY_DIR}/bambu_parameters_discovered.json"
   --metadata "${CMAKE_SOURCE_DIR}/documentation/bambu_parameters_metadata.json"
   --output-dir "${BAMBU_PARAM_OUTPUT_DIR}"
)
if(PANDA_SYNC_PARAMETER_METADATA)
   list(APPEND _bambu_param_registry_cmd --sync-metadata)
endif()
add_custom_command(
   OUTPUT "${BAMBU_PARAM_STAMP}"
   COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/etc/scripts/discover_bambu_parameters.py"
           --src-dir "${CMAKE_SOURCE_DIR}/src"
           --build-dir "${CMAKE_BINARY_DIR}"
   COMMAND ${_bambu_param_registry_cmd}
   COMMAND "${CMAKE_COMMAND}" -E touch "${BAMBU_PARAM_STAMP}"
   DEPENDS
      ${BAMBU_PARAM_INPUTS}
      ${BAMBU_PARAM_DEVICE_XML}
      "${CMAKE_SOURCE_DIR}/etc/scripts/discover_bambu_parameters.py"
      "${CMAKE_SOURCE_DIR}/etc/scripts/generate_bambu_parameter_registry.py"
      "${CMAKE_SOURCE_DIR}/documentation/bambu_parameters_metadata.json"
   BYPRODUCTS ${BAMBU_PARAM_REGISTRY_FILES}
   COMMENT "Updating bambu-parameter registry shards"
   VERBATIM
)

add_custom_target(bambu_parameter_registry DEPENDS "${BAMBU_PARAM_STAMP}")
set_source_files_properties(${BAMBU_PARAM_REGISTRY_FILES} PROPERTIES
   OBJECT_DEPENDS "${BAMBU_PARAM_STAMP}"
)
target_sources(panda_core PRIVATE ${BAMBU_PARAM_REGISTRY_FILES})
add_dependencies(panda_core bambu_parameter_registry)
# Generate constraints_STD data only when source changes (mirrors Automake behavior)
function(panda_make_constraints_data xml_name xml_src)
   get_filename_component(xml_abs "${xml_src}" ABSOLUTE)
   set(data_out "${CMAKE_CURRENT_BINARY_DIR}/${xml_name}")
   set(script "${CMAKE_CURRENT_SOURCE_DIR}/../etc/scripts/add_escape.sh")
   add_custom_command(OUTPUT "${data_out}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}"
      COMMAND "${script}" "${xml_abs}" "${data_out}.tmp"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${data_out}.tmp" "${data_out}"
      COMMAND "${CMAKE_COMMAND}" -E remove "${data_out}.tmp"
      DEPENDS "${xml_src}" "${script}"
      VERBATIM
      COMMENT "Generating ${xml_name}"
   )
   list(APPEND __panda_constraints "${data_out}")
   set(__panda_constraints "${__panda_constraints}" PARENT_SCOPE)
endfunction()

set(__panda_constraints)
panda_make_constraints_data("constraints_STD.data" "${CMAKE_CURRENT_SOURCE_DIR}/../etc/libtech/constraints_STD.xml")
panda_make_constraints_data("constraints_STD_ALUs.data" "${CMAKE_CURRENT_SOURCE_DIR}/../etc/libtech/constraints_STD_ALUs.xml")
add_custom_target(generate_constraints_std ALL DEPENDS ${__panda_constraints})
add_dependencies(panda_core generate_constraints_std)

# Generate branch/revision metadata headers
set(BRANCH_NAME_HPP "${CMAKE_CURRENT_BINARY_DIR}/branch_name.hpp")
set(REVISION_HASH_HPP "${CMAKE_CURRENT_BINARY_DIR}/revision_hash.hpp")
set(_panda_revision_git_deps)
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
   execute_process(
      COMMAND git -C "${CMAKE_SOURCE_DIR}" rev-parse --git-path HEAD
      OUTPUT_VARIABLE _panda_git_head_path
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE _panda_git_head_path_res
      ERROR_QUIET
   )
   if(_panda_git_head_path_res EQUAL 0 AND _panda_git_head_path)
      if(NOT IS_ABSOLUTE "${_panda_git_head_path}")
         get_filename_component(_panda_git_head_path "${_panda_git_head_path}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
      endif()
      list(APPEND _panda_revision_git_deps "${_panda_git_head_path}")
   endif()
   execute_process(
      COMMAND git -C "${CMAKE_SOURCE_DIR}" symbolic-ref -q HEAD
      OUTPUT_VARIABLE _panda_git_head_ref
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE _panda_git_head_ref_res
      ERROR_QUIET
   )
   if(_panda_git_head_ref_res EQUAL 0 AND _panda_git_head_ref)
      execute_process(
         COMMAND git -C "${CMAKE_SOURCE_DIR}" rev-parse --git-path "${_panda_git_head_ref}"
         OUTPUT_VARIABLE _panda_git_head_ref_path
         OUTPUT_STRIP_TRAILING_WHITESPACE
         RESULT_VARIABLE _panda_git_head_ref_path_res
         ERROR_QUIET
      )
      if(_panda_git_head_ref_path_res EQUAL 0 AND _panda_git_head_ref_path)
         if(NOT IS_ABSOLUTE "${_panda_git_head_ref_path}")
            get_filename_component(_panda_git_head_ref_path "${_panda_git_head_ref_path}" ABSOLUTE
               BASE_DIR "${CMAKE_SOURCE_DIR}")
         endif()
         list(APPEND _panda_revision_git_deps "${_panda_git_head_ref_path}")
      endif()
   endif()
   execute_process(
      COMMAND git -C "${CMAKE_SOURCE_DIR}" rev-parse --git-path packed-refs
      OUTPUT_VARIABLE _panda_git_packed_refs_path
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE _panda_git_packed_refs_path_res
      ERROR_QUIET
   )
   if(_panda_git_packed_refs_path_res EQUAL 0 AND _panda_git_packed_refs_path)
      if(NOT IS_ABSOLUTE "${_panda_git_packed_refs_path}")
         get_filename_component(_panda_git_packed_refs_path "${_panda_git_packed_refs_path}" ABSOLUTE
            BASE_DIR "${CMAKE_SOURCE_DIR}")
      endif()
      if(EXISTS "${_panda_git_packed_refs_path}")
         list(APPEND _panda_revision_git_deps "${_panda_git_packed_refs_path}")
      endif()
   endif()
elseif(EXISTS "${CMAKE_SOURCE_DIR}/dist_revision_info")
   list(APPEND _panda_revision_git_deps "${CMAKE_SOURCE_DIR}/dist_revision_info")
endif()
# Stamp-file output makes Ninja track idempotent generation correctly
set(REVISION_STAMP "${CMAKE_CURRENT_BINARY_DIR}/revision_stamp")
add_custom_command(
   OUTPUT "${REVISION_STAMP}"
   BYPRODUCTS "${BRANCH_NAME_HPP}" "${REVISION_HASH_HPP}"
   COMMAND bash -c "set -euo pipefail; \
if command -v git >/dev/null 2>&1 && git -C \"${CMAKE_SOURCE_DIR}\" rev-parse --git-dir >/dev/null 2>&1; then \
   printf '\"%s\"\\n' \"$(git -C \"${CMAKE_SOURCE_DIR}\" rev-parse --abbrev-ref HEAD)\" > \"${BRANCH_NAME_HPP}.tmp\"; \
   printf '\"%s\"\\n' \"$(git -C \"${CMAKE_SOURCE_DIR}\" rev-parse HEAD)\" > \"${REVISION_HASH_HPP}.tmp\"; \
elif [ -f \"${CMAKE_SOURCE_DIR}/dist_revision_info\" ]; then \
   sed -n '2s/.*/\"&\"/p' \"${CMAKE_SOURCE_DIR}/dist_revision_info\" > \"${BRANCH_NAME_HPP}.tmp\"; \
   sed -n '1s/.*/\"&\"/p' \"${CMAKE_SOURCE_DIR}/dist_revision_info\" > \"${REVISION_HASH_HPP}.tmp\"; \
else \
   echo \"\\\"unknown\\\"\" > \"${BRANCH_NAME_HPP}.tmp\"; \
   echo \"\\\"unknown\\\"\" > \"${REVISION_HASH_HPP}.tmp\"; \
fi; \
${CMAKE_COMMAND} -E copy_if_different \"${BRANCH_NAME_HPP}.tmp\" \"${BRANCH_NAME_HPP}\"; \
${CMAKE_COMMAND} -E copy_if_different \"${REVISION_HASH_HPP}.tmp\" \"${REVISION_HASH_HPP}\"; \
${CMAKE_COMMAND} -E touch \"${REVISION_STAMP}\""
   DEPENDS ${_panda_revision_git_deps}
   WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
   COMMENT "Updating branch/revision headers"
   VERBATIM
)
add_custom_target(panda_revision_headers DEPENDS "${REVISION_STAMP}")
add_dependencies(panda_core panda_revision_headers)

# License install (mirrors Automake install-data-hook_license)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE_INSTALL"
        DESTINATION "${PANDA_LICENSING_INSTALLDIR_REL}"
        RENAME LICENSE)

# Parsers and lexers (Bison/Flex)
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

function(panda_add_lexer_parser name y_file l_file)
   BISON_TARGET(${name}_parser "${y_file}" "${CMAKE_CURRENT_BINARY_DIR}/${name}Parser.cpp"
      DEFINES_FILE "${CMAKE_CURRENT_BINARY_DIR}/${name}Parser.hpp"
      COMPILE_FLAGS "-d -l --report=state,lookahead,itemset")
   FLEX_TARGET(${name}_lexer "${l_file}" "${CMAKE_CURRENT_BINARY_DIR}/${name}Lexer.cpp"
      COMPILE_FLAGS "-L -B -+")
   ADD_FLEX_BISON_DEPENDENCY(${name}_lexer ${name}_parser)
   target_sources(panda_core PRIVATE ${BISON_${name}_parser_OUTPUTS} ${FLEX_${name}_lexer_OUTPUTS})
endfunction()

panda_add_lexer_parser(ir "${CMAKE_CURRENT_SOURCE_DIR}/parser/compiler/irParser.ypp"
   "${CMAKE_CURRENT_SOURCE_DIR}/parser/compiler/irLexer.lpp")
panda_add_lexer_parser(xml "${CMAKE_CURRENT_SOURCE_DIR}/parser/polixml/xmlParser.ypp"
   "${CMAKE_CURRENT_SOURCE_DIR}/parser/polixml/xmlLexer.lpp")
target_include_directories(panda_core
   PUBLIC
      "${CMAKE_CURRENT_SOURCE_DIR}"
      "${CMAKE_CURRENT_SOURCE_DIR}/algorithms"
      "${CMAKE_CURRENT_SOURCE_DIR}/algorithms/dominance"
      "${CMAKE_CURRENT_SOURCE_DIR}/algorithms/loops_detection"
      "${CMAKE_CURRENT_SOURCE_DIR}/algorithms/clique_covering"
      "${CMAKE_CURRENT_SOURCE_DIR}/algorithms/sdc_solver"
      "${CMAKE_CURRENT_SOURCE_DIR}/algorithms/reachability"
      "${CMAKE_CURRENT_SOURCE_DIR}/algorithms/range_analysis"
      "${CMAKE_CURRENT_SOURCE_DIR}/behavior"
      "${CMAKE_CURRENT_SOURCE_DIR}/circuit"
      "${CMAKE_CURRENT_SOURCE_DIR}/constants"
      "${CMAKE_CURRENT_SOURCE_DIR}/design_flows"
      "${CMAKE_CURRENT_SOURCE_DIR}/design_flows/backend"
      "${CMAKE_CURRENT_SOURCE_DIR}/design_flows/backend/ToHDL"
      "${CMAKE_CURRENT_SOURCE_DIR}/design_flows/backend/ToHDL/writer"
      "${CMAKE_CURRENT_SOURCE_DIR}/design_flows/backend/ToC"
      "${CMAKE_CURRENT_SOURCE_DIR}/design_flows/backend/ToC/progModels"
      "${CMAKE_CURRENT_SOURCE_DIR}/design_flows/backend/ToC/source_code_writers"
      "${CMAKE_CURRENT_SOURCE_DIR}/design_flows/technology"
      "${CMAKE_CURRENT_SOURCE_DIR}/frontend_analysis"
      "${CMAKE_CURRENT_SOURCE_DIR}/frontend_analysis/IR_analysis"
      "${CMAKE_CURRENT_SOURCE_DIR}/frontend_analysis/behavior_analysis"
      "${CMAKE_CURRENT_SOURCE_DIR}/frontend_analysis/IR_manipulation"
      "${CMAKE_CURRENT_SOURCE_DIR}/graph"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/architecture_creation"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/architecture_creation/controller_creation"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/architecture_creation/datapath_creation"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/backend"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/binding"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/binding/interconnection"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/binding/module"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/binding/register"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/binding/register/algorithms"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/binding/storage_value_insertion"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/chaining"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/evaluation"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/evaluation/exact"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/hls_flow"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/hls_flow/synthesis"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/interface"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/interface/minimal"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/interface/WB4"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/function_allocation"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/liveness"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/memory"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/module_allocation"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/hdl_generators"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/scheduling"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/simulation"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/vcd"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/fsm"
      "${CMAKE_CURRENT_SOURCE_DIR}/HLS/virtual_components"
      "${CMAKE_CURRENT_SOURCE_DIR}/parser"
      "${CMAKE_CURRENT_SOURCE_DIR}/parser/compiler"
      "${CMAKE_CURRENT_SOURCE_DIR}/parser/discrepancy"
      "${CMAKE_CURRENT_SOURCE_DIR}/parser/polixml"
      "${CMAKE_CURRENT_SOURCE_DIR}/parser/vcd"
      "${CMAKE_CURRENT_SOURCE_DIR}/polixml"
      "${CMAKE_CURRENT_SOURCE_DIR}/technology"
      "${CMAKE_CURRENT_SOURCE_DIR}/IR"
      "${CMAKE_CURRENT_SOURCE_DIR}/utility"
      "${CMAKE_CURRENT_SOURCE_DIR}/wrapper"
      "${CMAKE_CURRENT_SOURCE_DIR}/wrapper/compiler"
      "${CMAKE_CURRENT_SOURCE_DIR}/wrapper/simulation"
      "${CMAKE_CURRENT_SOURCE_DIR}/wrapper/simulation/csim"
      "${CMAKE_CURRENT_SOURCE_DIR}/wrapper/simulation/modelsim"
      "${CMAKE_CURRENT_SOURCE_DIR}/wrapper/simulation/vcs"
      "${CMAKE_CURRENT_SOURCE_DIR}/wrapper/simulation/verilator"
      "${CMAKE_CURRENT_SOURCE_DIR}/wrapper/simulation/xsim"
      "${CMAKE_CURRENT_SOURCE_DIR}/wrapper/synthesis"
      "${CMAKE_CURRENT_SOURCE_DIR}/wrapper/synthesis/altera"
      "${CMAKE_CURRENT_SOURCE_DIR}/wrapper/synthesis/lattice"
      "${CMAKE_CURRENT_SOURCE_DIR}/wrapper/synthesis/nanoxplore"
      "${CMAKE_CURRENT_SOURCE_DIR}/wrapper/synthesis/xilinx"
      "${CMAKE_CURRENT_SOURCE_DIR}/wrapper/synthesis/xilinx/ise"
      "${CMAKE_CURRENT_SOURCE_DIR}/../etc/libopenmp"
      "${CMAKE_CURRENT_BINARY_DIR}"
   )
target_link_libraries(panda_core
   PUBLIC
      panda_build_flags
      panda_config_headers
      panda_ext_headers
      panda_ext_absl
      panda_ext_pugixml
      panda_ext_mockturtle
      panda_ext_glaze
      panda_ext_buddy
      ZLIB::ZLIB
      Boost::headers
)
add_dependencies(panda_core libtech_data)
include(CheckCXXCompilerFlag)
set(_panda_core_noerror_flags
)
check_cxx_compiler_flag("-Wdangling-reference" PANDA_HAVE_WDANGLING_REFERENCE)
if(PANDA_HAVE_WDANGLING_REFERENCE)
   list(APPEND _panda_core_noerror_flags -Wno-error=dangling-reference)
endif()
target_compile_options(panda_core PRIVATE ${_panda_core_noerror_flags})

# Narrow archive for module generators (mirrors Automake whole-archive of lib_hdl_generators)
file(GLOB PANDA_HDL_GENERATOR_SOURCES
   "${CMAKE_CURRENT_SOURCE_DIR}/HLS/hdl_generators/*.cpp"
)
add_library(panda_hdl_generators STATIC ${PANDA_HDL_GENERATOR_SOURCES})
target_include_directories(panda_hdl_generators PRIVATE
   $<TARGET_PROPERTY:panda_core,INCLUDE_DIRECTORIES>
)
target_compile_options(panda_hdl_generators PRIVATE
   $<TARGET_PROPERTY:panda_core,COMPILE_OPTIONS>
)
target_link_libraries(panda_hdl_generators PRIVATE panda_core)

# Panda unit test executable (mirrors automake check target)
set(PANDA_UNIT_TEST_SOURCES
   "${CMAKE_CURRENT_SOURCE_DIR}/../panda_unit/main_tests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/../panda_unit/algorithms/BasicBlockReachability.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/../panda_unit/algorithms/Dominance.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/../panda_unit/algorithms/Loops.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/../panda_unit/algorithms/SdcSolver.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/../panda_unit/utility/APInt.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/../panda_unit/utility/AreaEstimation.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/../panda_unit/utility/ConstDivMagic.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/../panda_unit/utility/KcmConstMul.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/../panda_unit/utility/Range.cpp"
)

add_executable(libbambu_unit ${PANDA_UNIT_TEST_SOURCES})
target_link_libraries(libbambu_unit PRIVATE panda_core Boost::headers)
add_test(NAME libbambu_unit COMMAND libbambu_unit)

if(PANDA_BUILD_BAMBU)
   add_executable(bambu
      bambu.cpp
      BambuParameter.cpp
      Parameter.cpp
      global_variables.cpp
   )
   target_link_libraries(bambu PRIVATE
      "-Wl,--whole-archive" panda_hdl_generators "-Wl,--no-whole-archive"
      panda_core
   )
   target_link_options(bambu PRIVATE "-Wl,-z,origin")
   set_target_properties(bambu PROPERTIES
      INSTALL_RPATH "\$ORIGIN/../${PANDA_LIB_INSTALLDIR_REL}"
   )
   install(CODE "
      if(NOT EXISTS \"${CMAKE_BINARY_DIR}/src/bambu\")
         execute_process(COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_BINARY_DIR}\" --target bambu)
      endif()
   " )
   install(TARGETS bambu
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
   )
endif()

if(PANDA_BUILD_EUCALYPTUS)
   add_executable(eucalyptus
      eucalyptus.cpp
      EucalyptusParameter.cpp
      Parameter.cpp
      global_variables.cpp
   )
   target_link_libraries(eucalyptus PRIVATE
      "-Wl,--whole-archive" panda_hdl_generators "-Wl,--no-whole-archive"
      panda_core
   )
   target_link_options(eucalyptus PRIVATE "-Wl,-z,origin")
   set_target_properties(eucalyptus PROPERTIES
      INSTALL_RPATH "\$ORIGIN/../${PANDA_LIB_INSTALLDIR_REL}"
   )
   install(CODE "
      if(NOT EXISTS \"${CMAKE_BINARY_DIR}/src/eucalyptus\")
         execute_process(COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_BINARY_DIR}\" --target eucalyptus)
      endif()
   " )
   install(TARGETS eucalyptus
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
   )
endif()

if(PANDA_BUILD_CC)
   add_executable(bambu-cc
      bambu-cc.cpp
      bambu-cc-Parameter.cpp
      Parameter.cpp
      global_variables.cpp
   )
   target_link_libraries(bambu-cc PRIVATE
      "-Wl,--whole-archive" panda_hdl_generators "-Wl,--no-whole-archive"
      panda_core
   )
   target_link_options(bambu-cc PRIVATE "-Wl,-z,origin")
   set_target_properties(bambu-cc PROPERTIES
      INSTALL_RPATH "\$ORIGIN/../${PANDA_LIB_INSTALLDIR_REL}"
   )
   install(CODE "
      if(NOT EXISTS \"${CMAKE_BINARY_DIR}/src/bambu-cc\")
         execute_process(COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_BINARY_DIR}\" --target bambu-cc)
      endif()
   " )
   install(TARGETS bambu-cc
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
   )
endif()
