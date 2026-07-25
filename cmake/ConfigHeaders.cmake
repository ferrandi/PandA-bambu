function(panda_generate_config_headers)
   set(options)
   set(oneValueArgs
      BINARY_DIR
      PROJECT_NAME
      PROJECT_VERSION
      INSTALL_INCLUDE_REL
      INSTALL_LIB_REL
      INSTALL_DATA_REL
      INSTALL_LICENSING_REL
      SOURCE_DIR
      BUILD_BAMBU
      BUILD_EUCALYPTUS
      BUILD_CC
      ENABLE_ASSERTS
      ENABLE_RELEASE
      ENABLE_UNORDERED
      APPIMAGE_NAME
      ENABLE_DEBUG
      MIN_CLANG_VERSION
      MAX_CLANG_VERSION
      LIBBAMBU_COMPILER
   )
   cmake_parse_arguments(PANDA_CFG "${options}" "${oneValueArgs}" "" ${ARGN})

   if(NOT PANDA_CFG_BINARY_DIR)
      message(FATAL_ERROR "panda_generate_config_headers: BINARY_DIR is required")
   endif()
   if(NOT PANDA_CFG_SOURCE_DIR)
      set(PANDA_CFG_SOURCE_DIR "${CMAKE_SOURCE_DIR}")
   endif()
   if(NOT DEFINED PANDA_CFG_FORCE_CLANG_PROBE)
      set(PANDA_CFG_FORCE_CLANG_PROBE OFF)
   endif()

   file(MAKE_DIRECTORY "${PANDA_CFG_BINARY_DIR}")

   # Ensure flex/bison find_package variables are available for version checks.
   if(NOT DEFINED FLEX_FOUND)
      find_package(FLEX QUIET)
   endif()
   if(NOT DEFINED BISON_FOUND)
      find_package(BISON QUIET)
   endif()

   # Helper to emit a single config_<NAME>.hpp
   function(_panda_write_config name value)
      set(_dest "${PANDA_CFG_BINARY_DIR}/config_${name}.hpp")
      set(_tmp "${_dest}.tmp")
      file(WRITE "${_tmp}" "#define ${name} ${value}\n")
      execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_tmp}" "${_dest}")
      file(REMOVE "${_tmp}")
      set(PANDA_ALL_CONFIGS "${PANDA_ALL_CONFIGS}${name}\n" PARENT_SCOPE)
   endfunction()

   function(_panda_bool_to_int val out)
      if(${val})
         set(${out} 1 PARENT_SCOPE)
      else()
         set(${out} 0 PARENT_SCOPE)
      endif()
   endfunction()

   # Accumulate a concise clang probe status line for end-of-config summary.
   function(_panda_probe_log entry)
      set_property(GLOBAL APPEND PROPERTY PANDA_CLANG_PROBE_SUMMARY "${entry}")
   endfunction()

   function(_panda_write_clang_matrix ver exe cpp_exe cpp_cpp llvm_link llvm_opt plugin_dir version_str)
      get_filename_component(_clang_exe_name "${exe}" NAME)
      get_filename_component(_clangpp_name "${cpp_exe}" NAME)
      get_filename_component(_clangcpp_name "${cpp_cpp}" NAME)
      get_filename_component(_llvmlink_name "${llvm_link}" NAME)
      get_filename_component(_llvmopt_name "${llvm_opt}" NAME)
      _panda_write_config("HAVE_I386_CLANG${ver}_COMPILER" 1)
      _panda_write_config("I386_CLANG${ver}_EXE" "\"${_clang_exe_name}\"")
      _panda_write_config("I386_CLANG${ver}_PLUGIN_DIR" "\"${plugin_dir}\"")
      _panda_write_config("I386_CLANG${ver}_VERSION" "\"${version_str}\"")
      _panda_write_config("I386_CLANGPP${ver}_EXE" "\"${_clangpp_name}\"")
      _panda_write_config("I386_CLANG_CPP${ver}_EXE" "\"${_clangcpp_name}\"")
      _panda_write_config("I386_LLVM${ver}_LINK_EXE" "\"${_llvmlink_name}\"")
      _panda_write_config("I386_LLVM${ver}_OPT_EXE" "\"${_llvmopt_name}\"")
   endfunction()

   function(_panda_write_clang_matrix_empty ver)
      _panda_write_config("HAVE_I386_CLANG${ver}_COMPILER" 0)
      _panda_write_config("I386_CLANG${ver}_EXE" "\"\"")
      _panda_write_config("I386_CLANG${ver}_PLUGIN_DIR" "\"\"")
      _panda_write_config("I386_CLANG${ver}_VERSION" "\"\"")
      _panda_write_config("I386_CLANGPP${ver}_EXE" "\"\"")
      _panda_write_config("I386_CLANG_CPP${ver}_EXE" "\"\"")
      _panda_write_config("I386_LLVM${ver}_LINK_EXE" "\"\"")
      _panda_write_config("I386_LLVM${ver}_OPT_EXE" "\"\"")
   endfunction()

   _panda_bool_to_int("${PANDA_CFG_ENABLE_RELEASE}" _rel)
   _panda_bool_to_int("${PANDA_CFG_ENABLE_ASSERTS}" _asserts)
   _panda_bool_to_int("${PANDA_CFG_ENABLE_UNORDERED}" _unordered)
   _panda_bool_to_int("${PANDA_CFG_ENABLE_DEBUG}" _debug)
   _panda_bool_to_int("${PANDA_CFG_BUILD_BAMBU}" _bambu)
   _panda_bool_to_int("${PANDA_CFG_BUILD_EUCALYPTUS}" _euca)
   _panda_bool_to_int("${PANDA_CFG_BUILD_CC}" _treecc)

   # Directory-related defines
   _panda_write_config(PANDA_INCLUDE_INSTALLDIR "\"${PANDA_CFG_INSTALL_INCLUDE_REL}\"")
   _panda_write_config(PANDA_LIB_INSTALLDIR "\"${PANDA_CFG_INSTALL_LIB_REL}\"")
   _panda_write_config(PANDA_DATA_INSTALLDIR "\"${PANDA_CFG_INSTALL_DATA_REL}\"")
   _panda_write_config(PANDA_LICENSING_INSTALLDIR "\"${PANDA_CFG_INSTALL_LICENSING_REL}\"")
   _panda_write_config(PANDA_ETC_BUILDDIR "\"${PANDA_CFG_SOURCE_DIR}/etc\"")

   # Core toggles
   _panda_write_config(RELEASE ${_rel})
   _panda_write_config(NPROFILE ${_rel})
   _panda_write_config(HAVE_ASSERTS ${_asserts})
   _panda_write_config(HAVE_UNORDERED ${_unordered})
   _panda_write_config(HAVE_PRINT_STACK ${_debug})

   # Tool gates
   _panda_write_config(HAVE_BAMBU_RESULTS_XML ${_bambu})
   _panda_write_config(HAVE_HLS_BUILT ${_bambu})
   _panda_write_config(HAVE_TECHNOLOGY_BUILT ${_bambu})
   _panda_write_config(HAVE_IR_BUILT ${_bambu})
   _panda_write_config(HAVE_IR_MANIPULATION_BUILT ${_bambu})
   _panda_write_config(HAVE_BEHAVIOR_BUILT ${_bambu})
   _panda_write_config(HAVE_FRONTEND_ANALYSIS_BUILT ${_bambu})
   _panda_write_config(HAVE_DESIGN_FLOWS_BUILT ${_bambu})
   _panda_write_config(HAVE_CIRCUIT_BUILT ${_bambu})
   _panda_write_config(HAVE_UTILITY_BUILT ${_bambu})
   _panda_write_config(HAVE_VCD_BUILT ${_bambu})
   _panda_write_config(HAVE_SIMULATION_WRAPPER_BUILT ${_bambu})
   _panda_write_config(HAVE_FROM_C_BUILT ${_bambu})
   _panda_write_config(HAVE_FROM_DISCREPANCY_BUILT ${_bambu})
   _panda_write_config(HAVE_POLIXML_BUILT ${_bambu})
   _panda_write_config(HAVE_IR_MANIPULATION_BUILT ${_bambu})
   _panda_write_config(HAVE_IR_PARSER_BUILT ${_bambu})
   _panda_write_config(HAVE_LIBRARY_CHARACTERIZATION_BUILT ${_bambu})
   _panda_write_config(HAVE_RTL_CHARACTERIZATION_BUILT ${_bambu})
   _panda_write_config(HAVE_TO_C_BUILT ${_bambu})
   _panda_write_config(HAVE_TO_HDL_BUILT ${_bambu})
   _panda_write_config(HAVE_HOST_PROFILING_BUILT ${_bambu})

   # Third-party toggles (default on for bambu stage)
   _panda_write_config(HAVE_ABSEIL ${_bambu})
   _panda_write_config(HAVE_PUGIXML ${_bambu})
   _panda_write_config(HAVE_MOCKTURTLE ${_bambu})
   # Check hexfloat support (matches AX_HEXFLOAT intent)
   include(CheckCXXSourceCompiles)
   set(_hexfloat_src "
      #include <sstream>
      int main(){ std::stringstream ss; ss << std::hexfloat << 1.5; return 0; }
   ")
   check_cxx_source_compiles("${_hexfloat_src}" _panda_hexfloat_ok_native)
   if(NOT _panda_hexfloat_ok_native)
      set(_save_cxx_flags \"${CMAKE_CXX_FLAGS}\")
      set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} -std=c++11\")
      check_cxx_source_compiles(\"${_hexfloat_src}\" _panda_hexfloat_ok_cxx11)
      set(CMAKE_CXX_FLAGS \"${_save_cxx_flags}\")
   endif()
   if(_panda_hexfloat_ok_native OR _panda_hexfloat_ok_cxx11)
      _panda_write_config(HAVE_HEXFLOAT 1)
   else()
      _panda_write_config(HAVE_HEXFLOAT 0)
   endif()
   # Detect bison version (mirrors configure.ac)
   set(_bison_ok_27 0)
   if(BISON_FOUND)
      set(_bison_version "${BISON_VERSION}")
      if(NOT _bison_version AND BISON_EXECUTABLE)
         execute_process(COMMAND "${BISON_EXECUTABLE}" --version
            OUTPUT_VARIABLE _bison_out
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
         string(REGEX MATCH "([0-9]+\\.[0-9]+\\.[0-9]+|[0-9]+\\.[0-9]+)" _bv "${_bison_out}")
         if(_bv)
            set(_bison_version "${CMAKE_MATCH_1}")
         endif()
      endif()
      if(_bison_version AND _bison_version VERSION_GREATER_EQUAL "2.7")
         set(_bison_ok_27 1)
      endif()
   endif()
   _panda_write_config(HAVE_BISON_2_7_OR_GREATER ${_bison_ok_27})
   # Detect flex version (mirrors configure.ac checks)
   if(FLEX_FOUND)
      set(_flex_ok_234 0)
      set(_flex_ok_235 0)
      if(FLEX_VERSION)
         if(FLEX_VERSION VERSION_GREATER_EQUAL "2.5.34")
            set(_flex_ok_234 1)
         endif()
         if(FLEX_VERSION VERSION_GREATER_EQUAL "2.5.35")
            set(_flex_ok_235 1)
         endif()
      endif()
      _panda_write_config(HAVE_FLEX_2_5_34_OR_GREATER ${_flex_ok_234})
      _panda_write_config(HAVE_FLEX_2_5_35_OR_GREATER ${_flex_ok_235})
   else()
      _panda_write_config(HAVE_FLEX_2_5_34_OR_GREATER 0)
      _panda_write_config(HAVE_FLEX_2_5_35_OR_GREATER 0)
   endif()

   # AppImage and plugin placeholders
   if(PANDA_CFG_APPIMAGE_NAME)
      _panda_write_config(BUILD_APPIMAGE 1)
   else()
      _panda_write_config(BUILD_APPIMAGE 0)
   endif()
   _panda_write_config(COMPILER_CUSTOMSROA_PLUGIN "\"customSROA\"")
   _panda_write_config(COMPILER_EMPTY_PLUGIN "\"dumpBambuIrEmpty\"")
   _panda_write_config(COMPILER_SSA_PLUGIN "\"dumpBambuIrSSA\"")
   _panda_write_config(COMPILER_SSA_PLUGINCPP "\"dumpBambuIrSSACpp\"")
   _panda_write_config(COMPILER_TOPFNAME_PLUGIN "\"topfname\"")
   _panda_write_config(COMPILER_EXPANDMEMOPS_PLUGIN "\"expandMemOps\"")
   _panda_write_config(COMPILER_OPENMP_PLUGIN "\"openmpBambu\"")
   _panda_write_config(COMPILER_ASTANALYZER_PLUGIN "\"ASTAnalyzer\"")
   _panda_write_config(PACKAGE_NAME "\"${PANDA_CFG_PROJECT_NAME}\"")
   _panda_write_config(PACKAGE_VERSION "\"${PANDA_CFG_PROJECT_VERSION}\"")
   _panda_write_config(PACKAGE_STRING "\"${PANDA_CFG_PROJECT_NAME} ${PANDA_CFG_PROJECT_VERSION}\"")
   _panda_write_config(PACKAGE_BUGREPORT "\"panda-info@polimi.it\"")

   # Helper: check if compiler supports a flag by compiling and linking.
   function(_panda_check_flag out_var compiler flag workdir)
      file(WRITE "${workdir}/panda_flag_test.c" "int main(void){return 0;}\n")
      execute_process(COMMAND "${compiler}" "${flag}" "${workdir}/panda_flag_test.c" -o "${workdir}/panda_flag_test.out"
         RESULT_VARIABLE _res OUTPUT_QUIET ERROR_QUIET)
      if(_res EQUAL 0)
         set(${out_var} 1 PARENT_SCOPE)
      else()
         set(${out_var} 0 PARENT_SCOPE)
      endif()
   endfunction()

   # Detect clang toolchain matrix (closely follows ACX_PROG_CLANG)
   if(NOT PANDA_CFG_MIN_CLANG_VERSION)
      set(PANDA_CFG_MIN_CLANG_VERSION "4.0.0")
   endif()
   if(NOT PANDA_CFG_MAX_CLANG_VERSION)
      set(PANDA_CFG_MAX_CLANG_VERSION "20.0.0")
   endif()

   set(_panda_plugin_test_cpp "${PANDA_CFG_BINARY_DIR}/clang_plugin_test.cpp")
   file(WRITE "${_panda_plugin_test_cpp}" "
#include <cstdint>
#include \"clang/Frontend/FrontendPluginRegistry.h\"
#include \"clang/AST/ASTConsumer.h\"
#include \"clang/AST/Decl.h\"
#include \"clang/Frontend/CompilerInstance.h\"
#include \"llvm/Support/raw_ostream.h\"

using namespace clang;

namespace {
class ProbeConsumer : public ASTConsumer
{
public:
  bool HandleTopLevelDecl(DeclGroupRef DG) override
  {
    for (DeclGroupRef::iterator I = DG.begin(), E = DG.end(); I != E; ++I)
    {
      if (const auto *ND = dyn_cast<NamedDecl>(*I))
      {
        llvm::errs() << ND->getDeclName().getAsString() << \"\\n\";
      }
    }
    return true;
  }
};

class ProbeAction : public PluginASTAction
{
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance&, llvm::StringRef) override
  {
    return std::unique_ptr<ASTConsumer>(new ProbeConsumer());
  }

  bool ParseArgs(const CompilerInstance&, const std::vector<std::string>&) override
  {
    return true;
  }

  PluginASTAction::ActionType getActionType() override
  {
    return AddAfterMainAction;
  }
};
}

static FrontendPluginRegistry::Add<ProbeAction> X1(\"print-fns\", \"probe plugin\");
")

   set(_panda_plugin_client_cpp "${PANDA_CFG_BINARY_DIR}/clang_plugin_client.cpp")
   file(WRITE "${_panda_plugin_client_cpp}" "int main(){return 0;}\n")

   function(_panda_detect_clang ver plugin_dir min_ver max_ver)
      set(_found "")
      set(_version_str "")
      set(_clangpp_exe "")
      set(_clangcpp_exe "")
      set(_llvmlink_exe "")
      set(_llvmopt_exe "")
      set(_m32 0)
      set(_m64 0)
      set(_mx32 0)
      set(_abi "")
      set(_plugin_dir "${plugin_dir}")
      set(_plugin_abi "")

      # Reuse cached detection if available and not forced
      if(DEFINED PANDA_CLANG${ver}_FOUND AND NOT PANDA_CFG_FORCE_CLANG_PROBE)
         set(_cache_ok 1)
         if(PANDA_CLANG${ver}_FOUND)
            if(PANDA_CLANG${ver}_LLVMOPT STREQUAL "" OR PANDA_CLANG${ver}_LLVMLINK STREQUAL "" OR PANDA_CLANG${ver}_CLANGCPP STREQUAL "")
               set(_cache_ok 0)
            endif()
         endif()
         if(_cache_ok)
            if(PANDA_CLANG${ver}_FOUND)
               set(PANDA_CLANG${ver}_FOUND 1 PARENT_SCOPE)
               set(PANDA_CLANG${ver}_EXE "${PANDA_CLANG${ver}_EXE}" PARENT_SCOPE)
               set(PANDA_CLANG${ver}_CLANGPP "${PANDA_CLANG${ver}_CLANGPP}" PARENT_SCOPE)
               set(PANDA_CLANG${ver}_CLANGCPP "${PANDA_CLANG${ver}_CLANGCPP}" PARENT_SCOPE)
               set(PANDA_CLANG${ver}_LLVMLINK "${PANDA_CLANG${ver}_LLVMLINK}" PARENT_SCOPE)
               set(PANDA_CLANG${ver}_LLVMOPT "${PANDA_CLANG${ver}_LLVMOPT}" PARENT_SCOPE)
               set(PANDA_CLANG${ver}_PLUGIN_DIR "${PANDA_CLANG${ver}_PLUGIN_DIR}" PARENT_SCOPE)
               set(PANDA_CLANG${ver}_VERSION "${PANDA_CLANG${ver}_VERSION}" PARENT_SCOPE)
               set(PANDA_CLANG${ver}_M32 ${PANDA_CLANG${ver}_M32} PARENT_SCOPE)
               set(PANDA_CLANG${ver}_M64 ${PANDA_CLANG${ver}_M64} PARENT_SCOPE)
               set(PANDA_CLANG${ver}_MX32 ${PANDA_CLANG${ver}_MX32} PARENT_SCOPE)
               set(PANDA_CLANG${ver}_ABI ${PANDA_CLANG${ver}_ABI} PARENT_SCOPE)
               _panda_write_clang_matrix(${ver} "${PANDA_CLANG${ver}_EXE}" "${PANDA_CLANG${ver}_CLANGPP}" "${PANDA_CLANG${ver}_CLANGCPP}" "${PANDA_CLANG${ver}_LLVMLINK}" "${PANDA_CLANG${ver}_LLVMOPT}" "${PANDA_CLANG${ver}_PLUGIN_DIR}" "${PANDA_CLANG${ver}_VERSION}")
               return()
            else()
               set(PANDA_CLANG${ver}_FOUND 0 PARENT_SCOPE)
               _panda_write_clang_matrix_empty(${ver})
               return()
            endif()
         else()
            message(STATUS "clang-${ver}: cached detection incomplete, re-probing")
         endif()
      endif()

      unset(_clang_exe CACHE)
      set(_cands "clang-${ver}" "clang-${ver}.0" "clang${ver}")
      foreach(cand IN LISTS _cands)
         find_program(_clang_exe NAMES "${cand}" QUIET)
         if(NOT _clang_exe)
            continue()
         endif()
         execute_process(COMMAND "${_clang_exe}" --version
            OUTPUT_VARIABLE _out
            RESULT_VARIABLE _res
            OUTPUT_STRIP_TRAILING_WHITESPACE)
         if(NOT _res EQUAL 0)
            continue()
         endif()
         string(REGEX MATCH "version[ ]*([0-9]+\\.[0-9]+\\.[0-9]+|[0-9]+\\.[0-9]+)" _vm "${_out}")
         if(_vm)
            set(_version_str "${CMAKE_MATCH_1}")
         else()
            set(_version_str "${ver}")
         endif()
         string(REGEX MATCH "version[ ]*([0-9]+)" _m "${_out}")
         if(NOT _m)
            continue()
         endif()
         string(REGEX REPLACE ".*version[ ]*([0-9]+).*" "\\1" _maj "${_out}")
         if(NOT _maj STREQUAL "${ver}")
            continue()
         endif()
         if(NOT _version_str VERSION_GREATER_EQUAL "${min_ver}")
            continue()
         endif()
         if(_version_str VERSION_GREATER_EQUAL "${max_ver}")
            continue()
         endif()
         set(_found "${_clang_exe}")
         break()
      endforeach()

      if(NOT _found)
         message(STATUS "clang-${ver} not found within path; skipping")
         _panda_probe_log("clang-${ver}: not found")
         set(PANDA_CLANG${ver}_FOUND 0 PARENT_SCOPE)
         set(PANDA_CLANG${ver}_FOUND 0 CACHE INTERNAL "")
         set(PANDA_CLANG${ver}_EXE "" CACHE INTERNAL "")
         set(PANDA_CLANG${ver}_CLANGPP "" CACHE INTERNAL "")
         set(PANDA_CLANG${ver}_CLANGCPP "" CACHE INTERNAL "")
         set(PANDA_CLANG${ver}_LLVMLINK "" CACHE INTERNAL "")
         set(PANDA_CLANG${ver}_LLVMOPT "" CACHE INTERNAL "")
         set(PANDA_CLANG${ver}_PLUGIN_DIR "" CACHE INTERNAL "")
         set(PANDA_CLANG${ver}_VERSION "" CACHE INTERNAL "")
         set(PANDA_CLANG${ver}_ABI "" CACHE INTERNAL "")
         _panda_write_clang_matrix_empty(${ver})
         return()
      endif()

   get_filename_component(_clang_dir "${_found}" DIRECTORY)
   get_filename_component(_clang_root "${_clang_dir}" DIRECTORY)
      unset(_clangpp_exe CACHE)
      set(_clangpp_names "clang++-${ver}" "clang++-${ver}.0" "clang++")
   find_program(_clangpp_exe NAMES ${_clangpp_names} HINTS "${_clang_dir}" "${_clang_root}/bin" QUIET)
   if(NOT _clangpp_exe)
      foreach(_cand IN LISTS _clangpp_names)
         set(_probe "${_clang_dir}/${_cand}")
         if(EXISTS "${_probe}")
            set(_clangpp_exe "${_probe}")
            break()
         endif()
      endforeach()
   endif()
      if(NOT _clangpp_exe)
         message(STATUS "clang-${ver}: missing clang++ companion; skipping")
         set(PANDA_CLANG${ver}_FOUND 0 PARENT_SCOPE)
         _panda_write_clang_matrix_empty(${ver})
         return()
      endif()

      unset(_clangcpp_exe CACHE)
      unset(_clangcpp_exe)
      set(_clangcpp_names "clang-cpp-${ver}" "clang-cpp-${ver}.0" "clang-cpp")
      find_program(_clangcpp_exe NAMES ${_clangcpp_names}
         HINTS "${_clang_dir}" "${_clang_root}/bin"
         PATHS ENV PATH
         NO_CACHE
         QUIET)
      if(NOT _clangcpp_exe)
         set(_clangcpp_exe "")
      endif()

      unset(_llvmlink_exe CACHE)
      unset(_llvmlink_exe)
      set(_llvmlink_names "llvm-link-${ver}" "llvm-link-${ver}.0" "llvm-link")
      find_program(_llvmlink_exe NAMES ${_llvmlink_names} HINTS "${_clang_dir}" "${_clang_root}/bin" QUIET)
      if(NOT _llvmlink_exe)
         set(_llvmlink_exe "")
      endif()
      unset(_llvmopt_exe CACHE)
      unset(_llvmopt_exe)
      set(_llvmopt_names "opt-${ver}" "opt-${ver}.0" "opt")
      find_program(_llvmopt_exe NAMES ${_llvmopt_names} HINTS "${_clang_dir}" "${_clang_root}/bin" QUIET)
      if(NOT _llvmopt_exe)
         foreach(_cand IN LISTS _llvmopt_names)
            set(_probe "${_clang_dir}/${_cand}")
            if(EXISTS "${_probe}")
               set(_llvmopt_exe "${_probe}")
               break()
            endif()
            set(_probe_root "${_clang_root}/bin/${_cand}")
            if(EXISTS "${_probe_root}")
               set(_llvmopt_exe "${_probe_root}")
               break()
            endif()
         endforeach()
      endif()
      if(NOT _llvmopt_exe)
         set(_llvmopt_exe "NOTFOUND")
      endif()
      if(NOT _llvmlink_exe)
         foreach(_cand IN LISTS _llvmlink_names)
            set(_probe "${_clang_dir}/${_cand}")
            if(EXISTS "${_probe}")
               set(_llvmlink_exe "${_probe}")
               break()
            endif()
            set(_probe_root "${_clang_root}/bin/${_cand}")
            if(EXISTS "${_probe_root}")
               set(_llvmlink_exe "${_probe_root}")
               break()
            endif()
         endforeach()
      endif()
      if(NOT _llvmlink_exe)
         set(_llvmlink_exe "NOTFOUND")
      endif()

      # Build plugin to verify plugin support
      unset(_llvmconfig_exe CACHE)
      set(_llvmconfig_names "llvm-config-${ver}" "llvm-config-${ver}.0" "llvm-config")
      find_program(_llvmconfig_exe NAMES ${_llvmconfig_names} HINTS "${_clang_dir}" "${_clang_root}/bin" QUIET)
      if(NOT _llvmconfig_exe)
         message(STATUS "clang-${ver}: llvm-config not found; skipping")
         _panda_probe_log("clang-${ver}: failed (llvm-config missing)")
         set(PANDA_CLANG${ver}_FOUND 0 PARENT_SCOPE)
         _panda_write_clang_matrix_empty(${ver})
         return()
      endif()

      execute_process(COMMAND "${_llvmconfig_exe}" --cxxflags
         OUTPUT_VARIABLE _raw_flags
         OUTPUT_STRIP_TRAILING_WHITESPACE
         RESULT_VARIABLE _fres)
      if(NOT _fres EQUAL 0)
         message(STATUS "clang-${ver}: llvm-config --cxxflags failed; skipping")
         _panda_probe_log("clang-${ver}: failed (llvm-config --cxxflags)")
         set(PANDA_CLANG${ver}_FOUND 0 PARENT_SCOPE)
         _panda_write_clang_matrix_empty(${ver})
         return()
      endif()
      execute_process(COMMAND "${_llvmconfig_exe}" --libdir
         OUTPUT_VARIABLE _libdir
         OUTPUT_STRIP_TRAILING_WHITESPACE
         RESULT_VARIABLE _libres)
      if(NOT _libres EQUAL 0)
         set(_libdir "")
      endif()
      execute_process(COMMAND "${_llvmconfig_exe}" --ldflags
         OUTPUT_VARIABLE _ldflags_raw
         OUTPUT_STRIP_TRAILING_WHITESPACE
         RESULT_VARIABLE _ldres)
      if(NOT _ldres EQUAL 0)
         set(_ldflags_raw "")
      endif()

      separate_arguments(_raw_flags_list NATIVE_COMMAND "${_raw_flags}")
      set(_plugin_flags "")
      foreach(f IN LISTS _raw_flags_list)
         if(f MATCHES "^-W") # drop warnings
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
         list(APPEND _plugin_flags "${f}")
      endforeach()

      # ABI/flag prep
      list(APPEND _plugin_flags "-D__clang_major__=${ver}" "-D__clang_version__=\"${_version_str}\"")
      if(ver GREATER 7)
         list(APPEND _plugin_flags "-DNDEBUG")
      endif()

      if(_libdir)
         list(APPEND _plugin_flags "-L${_libdir}" "-Wl,-rpath,${_libdir}")
      endif()
      if(_ldflags_raw)
         separate_arguments(_ldflags_list NATIVE_COMMAND "${_ldflags_raw}")
         list(APPEND _plugin_flags ${_ldflags_list})
      endif()

      if(ver EQUAL 4)
         set(_std_flag "-std=c++14")
      else()
         set(_std_flag "-std=c++17")
      endif()

      set(_plugin_flags_base "${_plugin_flags}")
      set(_abi_candidates "0;1")
      set(_panda_abi_symbol_cxx11_declname
         "_ZNK5clang15DeclarationName11getAsStringB5cxx11Ev")
      set(_panda_abi_symbol_legacy_declname
         "_ZNK5clang15DeclarationName11getAsStringEv")
      # Prefer probing the actual LLVM/Clang runtime library because that is
      # where plugin-facing std::string ABI boundaries live. The compiler
      # executable alone is too weak a signal and can mis-detect minimal probes.
      set(_abi_probe_libs
         "${_libdir}/libclang-cpp.so"
         "${_libdir}/libclang-cpp.so.${ver}"
         "${_libdir}/libLLVM.so")
      set(_abi_from_runtime "")
      foreach(_abi_probe_lib IN LISTS _abi_probe_libs)
         if(NOT EXISTS "${_abi_probe_lib}")
            continue()
         endif()
         execute_process(COMMAND nm -D "${_abi_probe_lib}"
            OUTPUT_VARIABLE _abi_nm_out
            RESULT_VARIABLE _abi_nm_res
            ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
         if(NOT _abi_nm_res EQUAL 0)
            continue()
         endif()
         if(_abi_nm_out MATCHES "${_panda_abi_symbol_cxx11_declname}")
            set(_abi_from_runtime "1")
            break()
         endif()
         if(_abi_nm_out MATCHES "${_panda_abi_symbol_legacy_declname}")
            set(_abi_from_runtime "0")
            break()
         endif()
      endforeach()
      if(_abi_from_runtime STREQUAL "1")
         set(_abi_candidates "1;0")
      elseif(_abi_from_runtime STREQUAL "0")
         set(_abi_candidates "0;1")
      else()
         execute_process(COMMAND nm -D "${_found}"
            OUTPUT_VARIABLE _nm_out
            RESULT_VARIABLE _nm_res
            ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
         if(_nm_res EQUAL 0 AND _nm_out MATCHES "N4llvm" AND _nm_out MATCHES "__cxx11")
            set(_abi_candidates "1;0")
         endif()
      endif()

      set(_plugin_ok 0)
      foreach(_abi IN LISTS _abi_candidates)
         set(_plugin_flags "${_plugin_flags_base}")
         list(APPEND _plugin_flags "-D_GLIBCXX_USE_CXX11_ABI=${_abi}")
         set(_plugin_so "${PANDA_CFG_BINARY_DIR}/clang_plugin_${ver}.so")
         set(_plugin_env)
         if(_libdir)
            set(_plugin_env "LD_LIBRARY_PATH=${_libdir}")
         endif()
         execute_process(COMMAND "${CMAKE_COMMAND}" -E env ${_plugin_env} "${CMAKE_CXX_COMPILER}" ${_plugin_flags} "${_std_flag}" "-fPIC" "-shared" "${_panda_plugin_test_cpp}" -o "${_plugin_so}"
            WORKING_DIRECTORY "${PANDA_CFG_BINARY_DIR}"
            RESULT_VARIABLE _plugin_build_res
            OUTPUT_VARIABLE _plugin_build_out
            ERROR_VARIABLE _plugin_build_err
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE
         )
         if(NOT _plugin_build_res EQUAL 0)
            if(PANDA_CFG_VERBOSE_CLANG_PROBE)
               message(STATUS "clang-${ver}: plugin build failed (abi=${_abi}) rc=${_plugin_build_res}")
               if(_plugin_build_out)
                  message(STATUS "clang-${ver}: plugin build stdout (abi=${_abi}): ${_plugin_build_out}")
               endif()
               if(_plugin_build_err)
                  message(STATUS "clang-${ver}: plugin build stderr (abi=${_abi}): ${_plugin_build_err}")
               endif()
            endif()
            continue()
         endif()
         execute_process(COMMAND "${CMAKE_COMMAND}" -E env ${_plugin_env} "${_clangpp_exe}" "${_std_flag}" "-fplugin=${_plugin_so}" "-Xclang" "-add-plugin" "-Xclang" "print-fns"
            "-c" "${_panda_plugin_client_cpp}" "-o" "${PANDA_CFG_BINARY_DIR}/clang_plugin_client_${ver}.o"
            RESULT_VARIABLE _plugin_use_res
            OUTPUT_VARIABLE _plugin_use_out
            ERROR_VARIABLE _plugin_use_err
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE)
         if(_plugin_use_res EQUAL 0)
            set(_plugin_ok 1)
            set(_plugin_abi "${_abi}")
            if(PANDA_CFG_VERBOSE_CLANG_PROBE)
               message(STATUS "clang-${ver}: plugin test succeeded (abi=${_abi})")
            endif()
            break()
         endif()
         if(PANDA_CFG_VERBOSE_CLANG_PROBE)
            message(STATUS "clang-${ver}: plugin exec failed (abi=${_abi}) rc=${_plugin_use_res}")
            if(_plugin_use_out)
               message(STATUS "clang-${ver}: plugin exec stdout (abi=${_abi}): ${_plugin_use_out}")
            endif()
            if(_plugin_use_err)
               message(STATUS "clang-${ver}: plugin exec stderr (abi=${_abi}): ${_plugin_use_err}")
            endif()
         endif()
      endforeach()
      if(NOT _plugin_ok)
         message(STATUS "clang-${ver}: plugin execution failed; skipping (try -DPANDA_CFG_VERBOSE_CLANG_PROBE=ON for details)")
         _panda_probe_log("clang-${ver}: failed (plugin build/exec)")
         set(PANDA_CLANG${ver}_FOUND 0 PARENT_SCOPE)
         set(PANDA_CLANG${ver}_FOUND 0 CACHE INTERNAL "")
         set(PANDA_CLANG${ver}_EXE "" CACHE INTERNAL "")
         set(PANDA_CLANG${ver}_CLANGPP "" CACHE INTERNAL "")
         set(PANDA_CLANG${ver}_CLANGCPP "" CACHE INTERNAL "")
         set(PANDA_CLANG${ver}_LLVMLINK "" CACHE INTERNAL "")
         set(PANDA_CLANG${ver}_LLVMOPT "" CACHE INTERNAL "")
         set(PANDA_CLANG${ver}_PLUGIN_DIR "" CACHE INTERNAL "")
         set(PANDA_CLANG${ver}_VERSION "" CACHE INTERNAL "")
         set(PANDA_CLANG${ver}_ABI "" CACHE INTERNAL "")
         _panda_write_clang_matrix_empty(${ver})
         return()
      endif()
      set(PANDA_CLANG${ver}_ABI ${_plugin_abi} PARENT_SCOPE)
      set(PANDA_CLANG${ver}_ABI ${_plugin_abi} CACHE INTERNAL "" FORCE)

      # Check arch flags
      _panda_check_flag(_m32 "${_found}" "-m32" "${PANDA_CFG_BINARY_DIR}")
      _panda_check_flag(_m64 "${_found}" "-m64" "${PANDA_CFG_BINARY_DIR}")
      _panda_check_flag(_mx32 "${_found}" "-mx32" "${PANDA_CFG_BINARY_DIR}")

      message(STATUS "clang-${ver}: detected (exe=${_found}, clang++=${_clangpp_exe}, plugin_dir=${_plugin_dir}, version=${_version_str}, m32=${_m32}, m64=${_m64}, mx32=${_mx32})")
      _panda_probe_log("clang-${ver}: OK (version ${_version_str}, abi=${_plugin_abi}, exe=${_found})")
      set(PANDA_CLANG${ver}_FOUND 1 PARENT_SCOPE)
      set(PANDA_CLANG${ver}_EXE "${_found}" PARENT_SCOPE)
      set(PANDA_CLANG${ver}_CLANGPP "${_clangpp_exe}" PARENT_SCOPE)
      set(PANDA_CLANG${ver}_CLANGCPP "${_clangcpp_exe}" PARENT_SCOPE)
      set(PANDA_CLANG${ver}_LLVMLINK "${_llvmlink_exe}" PARENT_SCOPE)
      set(PANDA_CLANG${ver}_LLVMOPT "${_llvmopt_exe}" PARENT_SCOPE)
      set(PANDA_CLANG${ver}_PLUGIN_DIR "${_plugin_dir}" PARENT_SCOPE)
      set(PANDA_CLANG${ver}_VERSION "${_version_str}" PARENT_SCOPE)
      set(PANDA_CLANG${ver}_M32 ${_m32} PARENT_SCOPE)
      set(PANDA_CLANG${ver}_M64 ${_m64} PARENT_SCOPE)
      set(PANDA_CLANG${ver}_MX32 ${_mx32} PARENT_SCOPE)
      set(PANDA_CLANG${ver}_FOUND 1 CACHE INTERNAL "")
      set(PANDA_CLANG${ver}_EXE "${_found}" CACHE INTERNAL "")
      set(PANDA_CLANG${ver}_CLANGPP "${_clangpp_exe}" CACHE INTERNAL "")
      set(PANDA_CLANG${ver}_CLANGCPP "${_clangcpp_exe}" CACHE INTERNAL "")
      set(PANDA_CLANG${ver}_LLVMLINK "${_llvmlink_exe}" CACHE INTERNAL "")
      set(PANDA_CLANG${ver}_LLVMOPT "${_llvmopt_exe}" CACHE INTERNAL "")
      set(PANDA_CLANG${ver}_PLUGIN_DIR "${_plugin_dir}" CACHE INTERNAL "")
      set(PANDA_CLANG${ver}_VERSION "${_version_str}" CACHE INTERNAL "")
      set(PANDA_CLANG${ver}_M32 ${_m32} CACHE INTERNAL "")
      set(PANDA_CLANG${ver}_M64 ${_m64} CACHE INTERNAL "")
      set(PANDA_CLANG${ver}_MX32 ${_mx32} CACHE INTERNAL "")
      _panda_write_clang_matrix(${ver} "${_found}" "${_clangpp_exe}" "${_clangcpp_exe}" "${_llvmlink_exe}" "${_llvmopt_exe}" "${_plugin_dir}" "${_version_str}")
   endfunction()

   # Probe requested clang versions
   foreach(_pair IN ITEMS
         "4;clang-4.0"
         "5;clang-5.0"
         "6;clang-6.0"
         "7;clang-7"
         "8;clang-8"
         "9;clang-9"
         "10;clang-10"
         "11;clang-11"
         "12;clang-12"
         "13;clang-13"
         "16;clang-16"
         "19;clang-19")
      list(GET _pair 0 ver)
      list(GET _pair 1 plugin_dir)
      _panda_detect_clang(${ver} "${plugin_dir}" "${PANDA_CFG_MIN_CLANG_VERSION}" "${PANDA_CFG_MAX_CLANG_VERSION}")
   endforeach()

   # Emit a concise per-version probe summary for clarity.
   get_property(_probe_summary GLOBAL PROPERTY PANDA_CLANG_PROBE_SUMMARY)
   if(_probe_summary)
      string(REPLACE ";" "\n  " _probe_lines "${_probe_summary}")
      message(STATUS "Clang probe summary:\n  ${_probe_lines}")
   endif()

   # Pick analyzer plugin dir (prefers newer)
   set(_analyzer_dir "\"\"")
   foreach(ver IN ITEMS 19 16 13 12 11 10 9 8 7 6 5 4)
      if(PANDA_CLANG${ver}_FOUND)
         set(_analyzer_dir "\"${PANDA_CLANG${ver}_PLUGIN_DIR}\"")
         break()
      endif()
   endforeach()
   _panda_write_config(ANALYZER_COMPILER_PLUGINS_DIR ${_analyzer_dir})

   # Choose libbambu compiler (override respected)
   set(_bambu_choice "")
   set(_bambu_auto_order 16 19 12 11 10 9 8 7 6 5 4)
   if(PANDA_CFG_LIBBAMBU_COMPILER)
      string(REGEX REPLACE "I386_CLANG" "" _sel_ver "${PANDA_CFG_LIBBAMBU_COMPILER}")
      if(PANDA_CLANG${_sel_ver}_FOUND)
         set(_bambu_choice "${_sel_ver}")
      elseif("${PANDA_CFG_LIBBAMBU_COMPILER}" STREQUAL "I386_CLANG13")
         foreach(ver IN LISTS _bambu_auto_order)
            if(PANDA_CLANG${ver}_FOUND)
               set(_bambu_choice "${ver}")
               message(STATUS "Requested LIBBAMBU compiler I386_CLANG13 not found; falling back to I386_CLANG${ver}")
               break()
            endif()
         endforeach()
      else()
         message(FATAL_ERROR "Requested LIBBAMBU compiler ${PANDA_CFG_LIBBAMBU_COMPILER} not found")
      endif()
   else()
      foreach(ver IN LISTS _bambu_auto_order)
         if(PANDA_CLANG${ver}_FOUND)
            set(_bambu_choice "${ver}")
            break()
         endif()
      endforeach()
   endif()

   if(NOT _bambu_choice)
      message(FATAL_ERROR "No supported frontend clang compiler was found")
   endif()

   set(_bambu_id "I386_CLANG${_bambu_choice}")
   _panda_write_config(LIBBAMBU_COMPILER "\"${_bambu_id}\"")
   _panda_write_config(LIBBAMBU_COMPILER_DIR "\"${PANDA_CLANG${_bambu_choice}_PLUGIN_DIR}\"")

   if(PANDA_CLANG${_bambu_choice}_M64)
      _panda_write_config(HAVE_LIBBAMBU_M64 1)
   else()
      _panda_write_config(HAVE_LIBBAMBU_M64 0)
   endif()
   if(PANDA_CLANG${_bambu_choice}_M32)
      _panda_write_config(HAVE_LIBBAMBU_M32 1)
   else()
      _panda_write_config(HAVE_LIBBAMBU_M32 0)
   endif()
   if(PANDA_CLANG${_bambu_choice}_MX32)
      _panda_write_config(HAVE_LIBBAMBU_MX32 1)
   else()
      _panda_write_config(HAVE_LIBBAMBU_MX32 0)
   endif()

   # Create a consolidated lconfig.h for compatibility
   set(_lconfig "${PANDA_CFG_BINARY_DIR}/lconfig.h")
   file(WRITE "${_lconfig}" "/* Generated by CMake (panda_generate_config_headers) */\n")
   file(APPEND "${_lconfig}" "#pragma once\n")
   foreach(_cfg ${PANDA_ALL_CONFIGS})
      file(APPEND "${_lconfig}" "#include \"config_${_cfg}.hpp\"\n")
   endforeach()
endfunction()
