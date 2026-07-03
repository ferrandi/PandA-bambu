cmake_minimum_required(VERSION 3.16)

find_program(GZIP_EXECUTABLE gzip)
if(NOT GZIP_EXECUTABLE)
   message(FATAL_ERROR "gzip is required to build etc/libtech data files but was not found.")
endif()

set(LIBTECH_APPEND_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/append_libraries.sh")

add_custom_target(gzip_check
   COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/etc/libtech"
   COMMAND bash -lc 'set -e; if "${GZIP_EXECUTABLE}" --version >"${CMAKE_CURRENT_BINARY_DIR}/gzip_check.log" 2>&1; then :; else cat "${CMAKE_CURRENT_BINARY_DIR}/gzip_check.log" >&2; echo "gzip is required to build etc/libtech data files but the check failed." >&2; exit 1; fi'
)

set(LIBTECH_DATA_OUTPUTS)
set(LIBTECH_SPEC_OUTPUTS)

set(LIBTECH_DATA_XML
   C_FP_IPs.xml
   C_HLS_IPs.xml
   C_MEM_IPs.xml
   C_OPENMP_IPs.xml
   C_PROFILING_IPs.xml
   C_STD_IPs.xml
   C_VEC_IPs.xml
   NC_AXI_IPs.xml
   NC_CACHE_IPs.xml
   NC_CONCAT_IPs.xml
   NC_DATAFLOW_IPs.xml
   NC_HLS_IPs.xml
   NC_INTERCON_IPs.xml
   NC_MEMMAP_IPs.xml
   NC_MEM_IPs.xml
   NC_OPENMP_IPs.xml
   NC_SF_IPs.xml
   NC_STD_IPs.xml
   NC_TESTBENCH_IPs.xml
   NC_VEC_IPs.xml
   NC_wishbone_IPs.xml
)

function(libtech_generate_data xml_rel)
   get_filename_component(xml_dir "${xml_rel}" DIRECTORY)
   get_filename_component(xml_base "${xml_rel}" NAME_WE)
   if(xml_dir STREQUAL ".")
      set(out_dir "${CMAKE_CURRENT_BINARY_DIR}")
   else()
      set(out_dir "${CMAKE_CURRENT_BINARY_DIR}/${xml_dir}")
   endif()
   set(out_file "${out_dir}/${xml_base}.data")
   add_custom_command(OUTPUT "${out_file}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${out_dir}"
      COMMAND bash -lc "set -euo pipefail; \"${GZIP_EXECUTABLE}\" -c \"${CMAKE_CURRENT_SOURCE_DIR}/${xml_rel}\" > \"${out_file}\""
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${xml_rel}" gzip_check
      VERBATIM
      COMMENT "Generating ${xml_rel}.data"
   )
   list(APPEND LIBTECH_DATA_OUTPUTS "${out_file}")
   set(LIBTECH_DATA_OUTPUTS "${LIBTECH_DATA_OUTPUTS}" PARENT_SCOPE)
endfunction()

function(libtech_generate_spec_data base_xml)
   # Remaining arguments are optional seed/extra XML files
   set(options)
   set(oneValueArgs)
   set(multiValueArgs SEEDS)
   cmake_parse_arguments(SPEC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
   get_filename_component(base_name "${base_xml}" NAME_WE)
   set(out_file "${CMAKE_CURRENT_BINARY_DIR}/${base_name}.spec_data")
   set(cmd "set -euo pipefail; \"${LIBTECH_APPEND_LIBRARIES}\" \"${CMAKE_CURRENT_SOURCE_DIR}/${base_xml}\"")
   foreach(seed_xml IN LISTS SPEC_SEEDS)
      set(cmd "${cmd} \"${CMAKE_CURRENT_SOURCE_DIR}/${seed_xml}\"")
   endforeach()
   # Drop XML declarations that appear inside merged content, but keep the first line
   set(cmd "${cmd} | sed '1b; /^<?xml/d' | \"${GZIP_EXECUTABLE}\" > \"${out_file}\"")
   add_custom_command(OUTPUT "${out_file}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}"
      COMMAND bash -lc "${cmd}"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${base_xml}" ${SPEC_SEEDS}
              "${LIBTECH_APPEND_LIBRARIES}" gzip_check
      VERBATIM
      COMMENT "Generating ${base_name}.spec_data"
   )
   list(APPEND LIBTECH_SPEC_OUTPUTS "${out_file}")
   set(LIBTECH_SPEC_OUTPUTS "${LIBTECH_SPEC_OUTPUTS}" PARENT_SCOPE)
endfunction()

foreach(xml IN LISTS LIBTECH_DATA_XML)
   libtech_generate_data("${xml}")
endforeach()

# Spec data (append libraries + gzip, mirroring Automake rules)
libtech_generate_spec_data(altera/5CSEMA5F31C6.xml SEEDS altera/5CSEMA5F31C6-seed.xml)
libtech_generate_spec_data(altera/5SGXEA7N2F45C1.xml SEEDS altera/5SGXEA7N2F45C1-seed.xml)
libtech_generate_spec_data(altera/EP2C70F896C6.xml SEEDS altera/EP2C70F896C6-seed.xml)
libtech_generate_spec_data(altera/EP4SGX530KH40C2.xml SEEDS altera/EP4SGX530KH40C2-seed.xml)

libtech_generate_spec_data(asic/asap7-BC.xml SEEDS asic/asap7-BC-seed.xml)
libtech_generate_spec_data(asic/asap7-TC.xml SEEDS asic/asap7-TC-seed.xml)
libtech_generate_spec_data(asic/asap7-WC.xml SEEDS asic/asap7-WC-seed.xml)
libtech_generate_spec_data(asic/nangate45.xml SEEDS asic/nangate45-seed.xml)

libtech_generate_spec_data(lattice/LFE335EA8FN484C.xml SEEDS lattice/LFE335EA8FN484C-seed.xml)
libtech_generate_spec_data(lattice/LFE5U85F8BG756C.xml SEEDS lattice/LFE5U85F8BG756C-seed.xml)
libtech_generate_spec_data(lattice/LFE5UM85F8BG756C.xml SEEDS lattice/LFE5UM85F8BG756C-seed.xml)

libtech_generate_spec_data(nanoxplore/nx1h140tsp.xml SEEDS nanoxplore/nx1h140tsp-seed.xml)
libtech_generate_spec_data(nanoxplore/nx1h35S.xml SEEDS nanoxplore/nx1h35S-seed.xml)
libtech_generate_spec_data(nanoxplore/nx2h540tsc.xml SEEDS nanoxplore/nx2h540tsc-seed.xml)

libtech_generate_spec_data(xilinx/xc7a100t-1csg324.xml)
libtech_generate_spec_data(xilinx/xc7vx330t-1ffg1157.xml)
libtech_generate_spec_data(xilinx/xc7vx485t-2ffg1761.xml)
libtech_generate_spec_data(xilinx/xc7vx690t-3ffg1930.xml)
libtech_generate_spec_data(xilinx/xc7z020-1clg484.xml)
libtech_generate_spec_data(xilinx/xc7z020-1clg484-YOSYS.xml)
libtech_generate_spec_data(xilinx/xc7z045-2ffg900.xml)
libtech_generate_spec_data(xilinx/xcku060-3ffva1156.xml)
libtech_generate_spec_data(xilinx/xcu250-2Lfigd2104.xml)
libtech_generate_spec_data(xilinx/xcu280-2Lfsvh2892.xml)
libtech_generate_spec_data(xilinx/xcu50-2fsvh2104.xml)
libtech_generate_spec_data(xilinx/xcu55c-2Lfsvh2892.xml)

add_custom_target(libtech_data ALL DEPENDS ${LIBTECH_DATA_OUTPUTS} ${LIBTECH_SPEC_OUTPUTS})

set(LIBTECH_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/${PANDA_INSTALL_NAMESPACE}/libtech")
set(LIBTECH_TARGETS_INSTALL_DIR "${LIBTECH_INSTALL_DIR}/targets")
# Ensure generated artifacts exist when installing, even if user runs cmake --install directly
install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_BINARY_DIR}\" --target libtech_data --config \"\${CMAKE_INSTALL_CONFIG_NAME}\")")
foreach(_data_file IN LISTS LIBTECH_DATA_OUTPUTS)
   file(RELATIVE_PATH _data_rel "${CMAKE_CURRENT_BINARY_DIR}" "${_data_file}")
   get_filename_component(_data_dir "${_data_rel}" DIRECTORY)
   if(_data_dir STREQUAL ".")
      set(_dest "${LIBTECH_INSTALL_DIR}")
   else()
      set(_dest "${LIBTECH_INSTALL_DIR}/${_data_dir}")
   endif()
   install(FILES "${_data_file}" DESTINATION "${_dest}")
endforeach()
install(FILES ${LIBTECH_SPEC_OUTPUTS} DESTINATION "${LIBTECH_TARGETS_INSTALL_DIR}")
install(DIRECTORY backend/
        DESTINATION "${LIBTECH_INSTALL_DIR}/backend"
        USE_SOURCE_PERMISSIONS
        PATTERN "Makefile.am" EXCLUDE)

set(LIBTECH_LICENSE_DIR "${CMAKE_INSTALL_DATADIR}/${PANDA_INSTALL_NAMESPACE}/licensing/libtech")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE_INSTALL"
        DESTINATION "${LIBTECH_LICENSE_DIR}"
        RENAME LICENSE)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/PANDA_MIT.LICENSE"
        DESTINATION "${LIBTECH_LICENSE_DIR}")
