# Build the clang frontend plugins (one set per detected clang toolchain).
#
# Plugins are compiled with the project C++ compiler, but against the headers
# of each detected clang version. Output layout mirrors Autotools:
#    <build_dir>/<plugin_dir>/<plugin>.so
# and installs to ${PANDA_LIB_INSTALLDIR_REL}/<plugin_dir>.

set(_panda_clang_plugin_version "1.00")
set(COMPILER_CUSTOMSROA_PLUGIN "customSROA")
set(COMPILER_EMPTY_PLUGIN "dumpBambuIrEmpty")
set(COMPILER_SSA_PLUGIN "dumpBambuIrSSA")
set(COMPILER_SSA_PLUGINCPP "dumpBambuIrSSACpp")
set(COMPILER_TOPFNAME_PLUGIN "topfname")
set(COMPILER_EXPANDMEMOPS_PLUGIN "expandMemOps")
set(COMPILER_OPENMP_PLUGIN "openmpBambu")
set(COMPILER_ASTANALYZER_PLUGIN "ASTAnalyzer")
set(COMPILER_SCALARIZEARRAYOFFIFO_PLUGIN "scalarizeArrayOfFifo")

set(_panda_plugin_base_names
   customSROA
   dumpBambuIrEmpty
   dumpBambuIrSSA
   dumpBambuIrSSACpp
   topfname
   expandMemOps
   openmpBambu
   ASTAnalyzer
)

set(_panda_plugin_sources_dir "${CMAKE_CURRENT_SOURCE_DIR}")
set(_panda_plugin_common_includes
   "${_panda_plugin_sources_dir}"
   "${CMAKE_SOURCE_DIR}/etc/libopenmp"
   "${CMAKE_SOURCE_DIR}/src/IR"
)

set(_panda_plugin_buddy_lib "")
set(_panda_plugin_buddy_includes "")
if(NOT TARGET buddy)
   message(FATAL_ERROR "BuDDy target is required for clang plugins.")
endif()
# Build BuDDy as PIC so it can be linked into the plugin shared objects.
set_property(TARGET buddy PROPERTY POSITION_INDEPENDENT_CODE ON)
set(_panda_plugin_buddy_lib "$<TARGET_FILE:buddy>")
set(_panda_plugin_buddy_includes
   "${CMAKE_SOURCE_DIR}/ext/buddy/src"
   "${CMAKE_BINARY_DIR}/ext/buddy")

function(_panda_collect_clang_flags ver out_cxx out_ld out_libdir out_defs)
   # Derive toolchain locations from detected compiler path.
   if(NOT PANDA_CLANG${ver}_EXE)
      return()
   endif()
   get_filename_component(_clang_bin "${PANDA_CLANG${ver}_EXE}" DIRECTORY)
   get_filename_component(_clang_root "${_clang_bin}" DIRECTORY)
   # Prefer the llvm-config that matches the toolchain root to avoid mixing headers.
   set(_llvmconfig_candidates
      "${_clang_root}/bin/llvm-config-${ver}"
      "${_clang_root}/bin/llvm-config-${ver}.0"
      "${_clang_root}/bin/llvm-config"
      "${_clang_bin}/llvm-config-${ver}"
      "${_clang_bin}/llvm-config-${ver}.0"
      "${_clang_bin}/llvm-config"
   )
   set(_llvmconfig_exe "")
   foreach(_c IN LISTS _llvmconfig_candidates)
      if(EXISTS "${_c}")
         set(_llvmconfig_exe "${_c}")
         break()
      endif()
   endforeach()
   if(NOT _llvmconfig_exe)
      message(STATUS "clang-${ver}: llvm-config not found alongside toolchain; skipping plugins")
      return()
   endif()

   execute_process(COMMAND "${_llvmconfig_exe}" --cxxflags
      OUTPUT_VARIABLE _raw_flags
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE _fres)
   if(NOT _fres EQUAL 0)
      message(STATUS "clang-${ver}: llvm-config --cxxflags failed; skipping plugins")
      return()
   endif()
   execute_process(COMMAND "${_llvmconfig_exe}" --ldflags
      OUTPUT_VARIABLE _raw_ldflags
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE _lres)
   if(NOT _lres EQUAL 0)
      set(_raw_ldflags "")
   endif()
   execute_process(COMMAND "${_llvmconfig_exe}" --libdir
      OUTPUT_VARIABLE _libdir
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE _libres)
   if(NOT _libres EQUAL 0)
      set(_libdir "")
   endif()

   separate_arguments(_raw_flag_list NATIVE_COMMAND "${_raw_flags}")
   set(_cxxflags "")
   foreach(f IN LISTS _raw_flag_list)
      if(f MATCHES "^-W")
         continue()
      endif()
      if(f STREQUAL "-pedantic")
         continue()
      endif()
      if(f MATCHES "^-O[0-9]")
         continue()
      endif()
      if(f MATCHES "^-I(.+)")
         string(REGEX REPLACE "^-I" "-isystem" f "${f}")
      endif()
      list(APPEND _cxxflags "${f}")
   endforeach()

   # Mirror Automake: always add plugin-local PIC/LTO/visibility flags on top of
   # llvm-config output.
   list(APPEND _cxxflags "-fPIC" "-flto" "-fvisibility=hidden" "-fcommon")

   set(_defs "")
   list(APPEND _defs "-DPANDA_CLANG_MAJOR=${ver}")
   if(PANDA_CLANG${ver}_VERSION)
      list(APPEND _defs "-DPANDA_CLANG_VERSION_STR=\"${PANDA_CLANG${ver}_VERSION}\"")
   endif()
   if(DEFINED PANDA_CLANG${ver}_ABI AND NOT "${PANDA_CLANG${ver}_ABI}" STREQUAL "")
      list(APPEND _defs "-D_GLIBCXX_USE_CXX11_ABI=${PANDA_CLANG${ver}_ABI}")
   endif()
   list(APPEND _defs "-DPANDA_PLUGIN_VERSION=\"${_panda_clang_plugin_version}\"")
   set(_ldflags "")
   if(_libdir)
      list(APPEND _ldflags "-L${_libdir}" "-Wl,-rpath,${_libdir}")
   endif()
   if(_raw_ldflags)
      separate_arguments(_raw_ld_list NATIVE_COMMAND "${_raw_ldflags}")
      list(APPEND _ldflags ${_raw_ld_list})
   endif()
   execute_process(COMMAND "${_llvmconfig_exe}" --includedir
      OUTPUT_VARIABLE _incdir
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE _incres)
   if(_incres EQUAL 0 AND _incdir)
      list(APPEND _cxxflags "-isystem" "${_incdir}")
   endif()

   set(${out_cxx} "${_cxxflags}" PARENT_SCOPE)
   set(${out_ld} "${_ldflags}" PARENT_SCOPE)
   set(${out_libdir} "${_libdir}" PARENT_SCOPE)
   set(${out_defs} "${_defs}" PARENT_SCOPE)
endfunction()

function(_panda_add_clang_plugins ver)
   if(NOT PANDA_CLANG${ver}_FOUND)
      return()
   endif()

   _panda_collect_clang_flags(${ver} _clang_cxxflags _clang_ldflags _clang_libdir _clang_defs)
   if(NOT _clang_cxxflags)
      return()
   endif()
   # llvm-config may inject its own -std flag (e.g. c++14 for clang-10/11/12/13).
   # Keep clang-4 on C++14, force newer toolchains to C++17 by appending a final override.
   set(_plugin_std_override "-std=gnu++17")
   if(ver EQUAL 4)
      set(_plugin_std_override "-std=gnu++14")
   endif()

   set(_plugin_dir "${PANDA_CLANG${ver}_PLUGIN_DIR}")
   set(_out_dir "${CMAKE_BINARY_DIR}/${_plugin_dir}")

   # Helper to create one plugin module via native CMake C++ compilation rules,
   # so plugin compile commands are exported in compile_commands.json.
   function(_panda_make_plugin tgt name add_rsp)
      set(_srcs ${ARGN})
      add_library(${tgt} MODULE ${_srcs})
      set_target_properties(${tgt} PROPERTIES
         PREFIX ""
         OUTPUT_NAME "${name}"
         LIBRARY_OUTPUT_DIRECTORY "${_out_dir}"
         CXX_STANDARD 17
         CXX_STANDARD_REQUIRED ON
      )

      target_include_directories(${tgt} PRIVATE
         ${_panda_plugin_common_includes}
         ${_panda_plugin_buddy_includes}
         "${CMAKE_SOURCE_DIR}/ext/pugixml/pugixml/src"
      )
      target_compile_options(${tgt} PRIVATE ${_clang_cxxflags} ${_plugin_std_override} ${_clang_defs})
      target_compile_definitions(${tgt} PRIVATE ADD_RSP=${add_rsp})
      target_link_options(${tgt} PRIVATE
         ${_clang_ldflags}
         -static-libstdc++
         "-Wl,-z,nodelete"
      )
      target_link_libraries(${tgt} PRIVATE buddy)
      if(TARGET pugixml-static)
         target_link_libraries(${tgt} PRIVATE pugixml-static)
      else()
         set(_pugi_archive "${CMAKE_BINARY_DIR}/ext/pugixml/pugixml/libpugixml.a")
         if(EXISTS "${_pugi_archive}")
            target_link_libraries(${tgt} PRIVATE "${_pugi_archive}")
         endif()
      endif()

      foreach(_src IN LISTS _srcs)
         if(_src MATCHES "dumpBambuIr\\.cpp$")
            get_filename_component(_src_dir "${_src}" DIRECTORY)
            set_property(SOURCE "${_src}" APPEND PROPERTY OBJECT_DEPENDS "${_src_dir}/IR.def")
         endif()
      endforeach()

      set_property(TARGET ${tgt} PROPERTY PLUGIN_OUTPUT "${_out_dir}/${name}.so")
   endfunction()

   # Source groups mirroring Automake wiring.
   set(_src_base "${_panda_plugin_sources_dir}")
   set(_dump_sources "${_src_base}/dumpBambuIr.cpp")
   set(_pointer_resolution_src "${_src_base}/pointerResolutionPass.cpp")
   set(_hardekopf_src "${_src_base}/HardekopfLin_AA.cpp")
   set(_customsroa_sources
      "${_src_base}/arrPart.cpp"
      "${_src_base}/loopUnrollArrPartPass.cpp"
      "${_src_base}/plugin_scalarizeArrayOfFifo.cpp"
      "${_src_base}/canonicalizeHLSStreamGEPPass.cpp")
   set(_condinstcomb_src "${_src_base}/condInstCombIfTaggedPass.cpp")

   if(NOT ver EQUAL 4)
      _panda_make_plugin(clang_plugin_${ver}_customsroa "${COMPILER_CUSTOMSROA_PLUGIN}" 1 "${_src_base}/plugin_customSROA.cpp" ${_customsroa_sources})
   endif()
   _panda_make_plugin(clang_plugin_${ver}_empty        "${COMPILER_EMPTY_PLUGIN}"         1 "${_src_base}/plugin_dumpBambuIrEmpty.cpp" ${_dump_sources})
   _panda_make_plugin(clang_plugin_${ver}_ssa          "${COMPILER_SSA_PLUGIN}"           1 "${_src_base}/plugin_dumpBambuIrSSA.cpp" ${_dump_sources} ${_pointer_resolution_src} ${_hardekopf_src} ${_condinstcomb_src})
   _panda_make_plugin(clang_plugin_${ver}_ssacpp       "${COMPILER_SSA_PLUGINCPP}"        1 "${_src_base}/plugin_dumpBambuIrSSACpp.cpp" ${_dump_sources} ${_pointer_resolution_src} ${_hardekopf_src} ${_condinstcomb_src})
   _panda_make_plugin(clang_plugin_${ver}_topfname     "${COMPILER_TOPFNAME_PLUGIN}"      1 "${_src_base}/plugin_topfname.cpp")
   _panda_make_plugin(clang_plugin_${ver}_expandmemops "${COMPILER_EXPANDMEMOPS_PLUGIN}"  1 "${_src_base}/plugin_expandMemOps.cpp" ${_condinstcomb_src})
   _panda_make_plugin(clang_plugin_${ver}_openmp       "${COMPILER_OPENMP_PLUGIN}"        1 "${_src_base}/plugin_openmp.cpp")
   _panda_make_plugin(clang_plugin_${ver}_ast          "${COMPILER_ASTANALYZER_PLUGIN}"   1 "${_src_base}/plugin_ASTAnalyzer.cpp")
   if(ver GREATER_EQUAL 16)
      _panda_make_plugin(clang_plugin_${ver}_scalarizearrayoffifo "${COMPILER_SCALARIZEARRAYOFFIFO_PLUGIN}" 1 "${_src_base}/plugin_scalarizeArrayOfFifo.cpp")
   endif()

   _panda_make_plugin(clang_plugin_${ver}_opt_empty        "opt_${COMPILER_EMPTY_PLUGIN}"         0 "${_src_base}/plugin_dumpBambuIrEmpty.cpp" ${_dump_sources})
   if(NOT ver EQUAL 4)
      _panda_make_plugin(clang_plugin_${ver}_opt_customsroa "opt_${COMPILER_CUSTOMSROA_PLUGIN}" 0 "${_src_base}/plugin_customSROA.cpp" ${_customsroa_sources})
   endif()
   _panda_make_plugin(clang_plugin_${ver}_opt_ssa          "opt_${COMPILER_SSA_PLUGIN}"           0 "${_src_base}/plugin_dumpBambuIrSSA.cpp" ${_dump_sources} ${_pointer_resolution_src} ${_hardekopf_src} ${_condinstcomb_src})
   _panda_make_plugin(clang_plugin_${ver}_opt_ssacpp       "opt_${COMPILER_SSA_PLUGINCPP}"        0 "${_src_base}/plugin_dumpBambuIrSSACpp.cpp" ${_dump_sources} ${_pointer_resolution_src} ${_hardekopf_src} ${_condinstcomb_src})
   _panda_make_plugin(clang_plugin_${ver}_opt_topfname     "opt_${COMPILER_TOPFNAME_PLUGIN}"      0 "${_src_base}/plugin_topfname.cpp")
   _panda_make_plugin(clang_plugin_${ver}_opt_expandmemops "opt_${COMPILER_EXPANDMEMOPS_PLUGIN}"  0 "${_src_base}/plugin_expandMemOps.cpp" ${_condinstcomb_src})
   _panda_make_plugin(clang_plugin_${ver}_opt_openmp       "opt_${COMPILER_OPENMP_PLUGIN}"        0 "${_src_base}/plugin_openmp.cpp")
   _panda_make_plugin(clang_plugin_${ver}_opt_ast          "opt_${COMPILER_ASTANALYZER_PLUGIN}"   0 "${_src_base}/plugin_ASTAnalyzer.cpp")
   if(ver GREATER_EQUAL 16)
      _panda_make_plugin(clang_plugin_${ver}_opt_scalarizearrayoffifo "opt_${COMPILER_SCALARIZEARRAYOFFIFO_PLUGIN}" 0 "${_src_base}/plugin_scalarizeArrayOfFifo.cpp")
   endif()

   set(_all_plugins
      clang_plugin_${ver}_empty
      clang_plugin_${ver}_ssa
      clang_plugin_${ver}_ssacpp
      clang_plugin_${ver}_topfname
      clang_plugin_${ver}_expandmemops
      clang_plugin_${ver}_openmp
      clang_plugin_${ver}_ast
      clang_plugin_${ver}_opt_empty
      clang_plugin_${ver}_opt_ssa
      clang_plugin_${ver}_opt_ssacpp
      clang_plugin_${ver}_opt_topfname
      clang_plugin_${ver}_opt_expandmemops
      clang_plugin_${ver}_opt_openmp
      clang_plugin_${ver}_opt_ast
   )
   if(NOT ver EQUAL 4)
      list(APPEND _all_plugins
         clang_plugin_${ver}_customsroa
         clang_plugin_${ver}_opt_customsroa
      )
   endif()
   if(ver GREATER_EQUAL 16)
      list(APPEND _all_plugins
         clang_plugin_${ver}_scalarizearrayoffifo
         clang_plugin_${ver}_opt_scalarizearrayoffifo
      )
   endif()

   if(_all_plugins)
      install(TARGETS ${_all_plugins}
              LIBRARY DESTINATION "${PANDA_LIB_INSTALLDIR_REL}/${_plugin_dir}")
      # Strip installed plugin shared objects to reduce size.
      install(CODE "
         set(_strip_tool \"${CMAKE_STRIP}\")
         if(NOT _strip_tool)
            set(_strip_tool strip)
         endif()
         set(_plugin_dir \"${PANDA_LIB_INSTALLDIR_REL}/${_plugin_dir}\")
         file(GLOB _outputs \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/\${_plugin_dir}/*.so\")
         foreach(_dst IN LISTS _outputs)
            if(EXISTS \"\${_dst}\")
               execute_process(COMMAND \"\${_strip_tool}\" \"\${_dst}\")
            endif()
         endforeach()
      ")
   endif()
   add_custom_target(panda_clang_plugins_${ver} ALL DEPENDS ${_all_plugins})
endfunction()

# Aggregate target to ensure all detected plugin variants build by default.
add_custom_target(panda_clang_plugins ALL)

foreach(ver IN ITEMS 4 5 6 7 8 9 10 11 12 13 16 19)
   _panda_add_clang_plugins(${ver})
   if(TARGET panda_clang_plugins_${ver})
      add_dependencies(panda_clang_plugins panda_clang_plugins_${ver})
   endif()
endforeach()
