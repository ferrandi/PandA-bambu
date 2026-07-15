cmake_minimum_required(VERSION 3.16)

project(PandA VERSION 2026.06 LANGUAGES C CXX)
include(CTest)
enable_testing()

# Global defaults
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Feature toggles (stage 1: focus on bambu; others to be enabled later)
option(PANDA_BUILD_BAMBU "Build bambu HLS tool" ON)
option(PANDA_BUILD_EUCALYPTUS "Build eucalyptus RTL characterization tool" ON)
option(PANDA_BUILD_CC "Build bambu-cc compiler (required for libbambu)" ON)
option(PANDA_ENABLE_RELEASE "Enable release-style defines (NDEBUG)" ON)
option(PANDA_ENABLE_ASSERTS "Enable runtime asserts" ON)
option(PANDA_ENABLE_OPT "Enable aggressive optimization flags" ON)
option(PANDA_ENABLE_WERROR "Enable warnings-as-errors set" ON)
option(PANDA_ENABLE_DEBUG "Enable debug symbols" OFF)
option(PANDA_ENABLE_UNORDERED "Enable unordered data structure support" OFF)
option(PANDA_SYNC_PARAMETER_METADATA "Rewrite documentation/bambu_parameters_metadata.json during registry generation" ON)
set(PANDA_APPIMAGE_NAME "" CACHE STRING "Name for AppImage package (empty to disable)")
set(PANDA_APPIMAGE_ROOT "" CACHE PATH "AppImage staging root (used when PANDA_APPIMAGE_NAME is set)")
set(PANDA_APPIMAGE_PARALLEL "" CACHE STRING "Parallel jobs for the AppImage build/install step (empty uses the backend default)")
set(PANDA_APPIMAGE_BUILD_ENV "auto" CACHE STRING "AppImage build environment: auto, host, or container")
set(PANDA_APPIMAGE_CONTAINER_RUNTIME "" CACHE STRING "Preferred container runtime for AppImage builds (auto, docker, or podman)")
set(PANDA_APPIMAGE_CONTAINER_IMAGE "bambuhls/dev:xenial-24.08" CACHE STRING "Container image used for portable AppImage builds")
set(PANDA_APPIMAGE_CONTAINER_GENERATOR "Ninja" CACHE STRING "CMake generator used inside the AppImage container build")
set(PANDA_APPIMAGE_MAX_GLIBC "2.17" CACHE STRING "Maximum GLIBC version allowed in AppImage payloads (empty disables the audit)")
set(PANDA_LIBBAMBU_COMPILER "I386_CLANG13" CACHE STRING "Override libbambu compiler selection (defaults to I386_CLANG13, with automatic fallback when unavailable)")
option(PANDA_CFG_FORCE_CLANG_PROBE "Force clang plugin detection even if cached results exist" OFF)
set(PANDA_MIN_CLANG_VERSION "4.0.0" CACHE STRING "Minimum clang version to consider")
set(PANDA_MAX_CLANG_VERSION "20.0.0" CACHE STRING "Maximum clang version to consider")
set(PANDA_DIST_COMPILERS "" CACHE STRING "Comma/space separated compiler bundle names to install under <prefix>/compilers (mirrors Automake DIST_COMPILERS)")
set(PANDA_DIST_COMPILERS_BACKUP "" CACHE STRING "Optional backup location for compiler bundles (mirrors Automake DIST_COMPILERS_BACKUP)")
set(PANDA_SCAN_BUILD_VERSION "" CACHE STRING "Optional clang version suffix for scan-build (e.g. -11)")
set(PANDA_SCAN_BUILD_REPORT_DIR "scanbuild-report" CACHE STRING "Output directory for scan-build reports")

set(PANDA_OPTIMIZED_FLAGS "-Ofast -march=native -mtune=native" CACHE STRING "Compile flags used when PANDA_ENABLE_OPT is ON")
set(PANDA_UNOPTIMIZED_FLAGS "-O1" CACHE STRING "Compile flags used when PANDA_ENABLE_OPT is OFF")
set(PANDA_DEBUG_FLAGS "-g3" CACHE STRING "Compile flags added when PANDA_ENABLE_DEBUG is ON")

# Keep CMake's default -DNDEBUG toggles aligned with PANDA_ENABLE_RELEASE
if(NOT PANDA_ENABLE_RELEASE)
   foreach(_panda_rel_flag
      CMAKE_C_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELEASE
      CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RELWITHDEBINFO
      CMAKE_C_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_MINSIZEREL)
      if(DEFINED ${_panda_rel_flag} AND "${${_panda_rel_flag}}" MATCHES "(^|[ \t])[-/]DNDEBUG([ \t]|$)")
         string(REGEX REPLACE "(^|[ \t])[-/]DNDEBUG([ \t]|$)" " " _panda_clean_flag "${${_panda_rel_flag}}")
         string(STRIP "${_panda_clean_flag}" _panda_clean_flag)
         set(${_panda_rel_flag} "${_panda_clean_flag}" CACHE STRING "Flags for ${_panda_rel_flag} (NDEBUG stripped by PANDA_ENABLE_RELEASE=OFF)" FORCE)
      endif()
   endforeach()
endif()

# AppImage defaults: when enabled and no explicit install prefix, stage under build/appimage-root/usr
if(PANDA_APPIMAGE_NAME)
   if(NOT PANDA_APPIMAGE_ROOT)
      set(PANDA_APPIMAGE_ROOT "${CMAKE_BINARY_DIR}/appimage-root" CACHE PATH "AppImage staging root (used when PANDA_APPIMAGE_NAME is set)" FORCE)
   endif()
   if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
      set(CMAKE_INSTALL_PREFIX "${PANDA_APPIMAGE_ROOT}/usr" CACHE PATH "Install prefix" FORCE)
   endif()
endif()
set_property(CACHE PANDA_APPIMAGE_BUILD_ENV PROPERTY STRINGS auto host container)
set_property(CACHE PANDA_APPIMAGE_CONTAINER_RUNTIME PROPERTY STRINGS "" auto docker podman)

# Derived install directories (relative, as expected by config_* headers)
set(PANDA_INSTALL_NAMESPACE "panda")
set(PANDA_INCLUDE_INSTALLDIR_REL "${CMAKE_INSTALL_INCLUDEDIR}/${PANDA_INSTALL_NAMESPACE}")
set(PANDA_LIB_INSTALLDIR_REL     "${CMAKE_INSTALL_LIBDIR}/${PANDA_INSTALL_NAMESPACE}")
set(PANDA_DATA_INSTALLDIR_REL    "${CMAKE_INSTALL_DATADIR}/${PANDA_INSTALL_NAMESPACE}")
set(PANDA_LICENSING_INSTALLDIR_REL "${PANDA_DATA_INSTALLDIR_REL}/licensing")
set(PANDA_DATA_INSTALLDIR "${PANDA_DATA_INSTALLDIR_REL}")
set(PANDA_LICENSING_INSTALLDIR "${PANDA_LICENSING_INSTALLDIR_REL}")

find_package(Boost 1.48 CONFIG QUIET COMPONENTS headers)
if(NOT Boost_FOUND)
   if(POLICY CMP0167)
      message(FATAL_ERROR
         "BoostConfig.cmake was not found. Install the Boost development package or set Boost_DIR; "
         "CMake 4 no longer provides FindBoost.")
   endif()
   find_package(Boost 1.48 REQUIRED COMPONENTS headers)
endif()
find_package(ZLIB REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Build flags aggregator
add_library(panda_build_flags INTERFACE)
target_include_directories(panda_build_flags INTERFACE
   "${CMAKE_CURRENT_BINARY_DIR}/config_headers"
   "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

if(PANDA_ENABLE_DEBUG)
   separate_arguments(_panda_debug_flags NATIVE_COMMAND "${PANDA_DEBUG_FLAGS}")
   target_compile_options(panda_build_flags INTERFACE ${_panda_debug_flags})
endif()
if(PANDA_ENABLE_RELEASE)
   target_compile_definitions(panda_build_flags INTERFACE NDEBUG)
endif()
if(PANDA_ENABLE_OPT)
   separate_arguments(_panda_opt_flags NATIVE_COMMAND "${PANDA_OPTIMIZED_FLAGS}")
   target_compile_options(panda_build_flags INTERFACE ${_panda_opt_flags})
else()
   separate_arguments(_panda_non_opt_flags NATIVE_COMMAND "${PANDA_UNOPTIMIZED_FLAGS}")
   target_compile_options(panda_build_flags INTERFACE ${_panda_non_opt_flags})
endif()

if(PANDA_ENABLE_WERROR)
   target_compile_options(panda_build_flags INTERFACE
      -Wall -Werror -Wextra
      -Wconversion -Wsign-conversion -Wsign-promo
      -Winit-self
      -Wredundant-decls
      -Wvla -Wshadow -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual
      -Wpointer-arith
      -Wcast-qual -Wcast-align
      -Wdouble-promotion
      -Wold-style-cast -Wno-error=old-style-cast
      -Wswitch-enum
      -Wswitch-default
      -Wundef
      -fdiagnostics-show-option
   )
else()
   target_compile_options(panda_build_flags INTERFACE -Wall -Wextra)
endif()

# std::filesystem linkage check: add -lstdc++fs if needed on older libstdc++
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_LIBRARIES_BACKUP "${CMAKE_REQUIRED_LIBRARIES}")
check_cxx_source_compiles("
   #include <filesystem>
   int main() { std::filesystem::path p{\"/tmp\"}; return 0; }"
   PANDA_FILESYSTEM_WORKS)
set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES_BACKUP}")
if(NOT PANDA_FILESYSTEM_WORKS)
   target_link_libraries(panda_build_flags INTERFACE stdc++fs)
endif()

# Generate config headers (mirrors Autotools-generated config_*.hpp set)
include(ConfigHeaders)
panda_generate_config_headers(
   BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/config_headers"
   PROJECT_NAME "${PROJECT_NAME}"
   PROJECT_VERSION "${PROJECT_VERSION}"
   INSTALL_INCLUDE_REL "${PANDA_INCLUDE_INSTALLDIR_REL}"
   INSTALL_LIB_REL "${PANDA_LIB_INSTALLDIR_REL}"
   INSTALL_DATA_REL "${PANDA_DATA_INSTALLDIR_REL}"
   INSTALL_LICENSING_REL "${PANDA_LICENSING_INSTALLDIR_REL}"
   SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
   BUILD_BAMBU "${PANDA_BUILD_BAMBU}"
   BUILD_EUCALYPTUS "${PANDA_BUILD_EUCALYPTUS}"
   BUILD_CC "${PANDA_BUILD_CC}"
   ENABLE_ASSERTS "${PANDA_ENABLE_ASSERTS}"
   ENABLE_RELEASE "${PANDA_ENABLE_RELEASE}"
   ENABLE_UNORDERED "${PANDA_ENABLE_UNORDERED}"
   ENABLE_DEBUG "${PANDA_ENABLE_DEBUG}"
   APPIMAGE_NAME "${PANDA_APPIMAGE_NAME}"
   MIN_CLANG_VERSION "${PANDA_MIN_CLANG_VERSION}"
   MAX_CLANG_VERSION "${PANDA_MAX_CLANG_VERSION}"
   LIBBAMBU_COMPILER "${PANDA_LIBBAMBU_COMPILER}"
)

add_library(panda_config_headers INTERFACE)
target_include_directories(panda_config_headers INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/config_headers")

# Install-equivalent of Automake install-data-local snippets (licenses/readmes)
install(FILES 
         "${CMAKE_CURRENT_SOURCE_DIR}/LICENSES/Apache-2.0.txt"
         "${CMAKE_CURRENT_SOURCE_DIR}/LICENSES/LLVM-exception.txt"
         "${CMAKE_CURRENT_SOURCE_DIR}/LICENSES/MIT.txt"
        DESTINATION "${PANDA_LICENSING_INSTALLDIR_REL}")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/etc/libopenmp/LICENSE_INSTALL"
        DESTINATION "${PANDA_DATA_INSTALLDIR_REL}/libopenmp"
        RENAME LICENSE)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/etc/ac_types/LICENSE_INSTALL"
        DESTINATION "${PANDA_DATA_INSTALLDIR_REL}/ac_types"
        RENAME LICENSE)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/etc/ac_math/LICENSE_INSTALL"
        DESTINATION "${PANDA_DATA_INSTALLDIR_REL}/ac_math"
        RENAME LICENSE)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/etc/bambu_directives/LICENSE_INSTALL"
        DESTINATION "${PANDA_DATA_INSTALLDIR_REL}/bambu_directives"
        RENAME LICENSE)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/etc/bambu_directives/csroa_directives.h"
        DESTINATION "${PANDA_INCLUDE_INSTALLDIR_REL}")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/etc/clang_plugin/LICENSE_INSTALL"
        DESTINATION "${PANDA_LICENSING_INSTALLDIR_REL}/clang_plugins"
        RENAME LICENSE)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/etc/libbambu/LICENSE_INSTALL"
        DESTINATION "${PANDA_LICENSING_INSTALLDIR_REL}/libbambu"
        RENAME LICENSE)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/etc/libbambu/softfloat/LICENSE_INSTALL"
        DESTINATION "${PANDA_LICENSING_INSTALLDIR_REL}/softfloat"
        RENAME LICENSE)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/etc/libbambu/softint/LICENSE_INSTALL"
        DESTINATION "${PANDA_LICENSING_INSTALLDIR_REL}/softint"
        RENAME LICENSE)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/etc/libbambu/libm/hls/LICENSE_INSTALL"
        DESTINATION "${PANDA_LICENSING_INSTALLDIR_REL}/libm/hls"
        RENAME LICENSE)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/etc/libbambu/libm/musl/musl_math.h"
        DESTINATION "${PANDA_INCLUDE_INSTALLDIR_REL}/musl"
        RENAME math.h)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/etc/libbambu/libm/musl/LICENSE_INSTALL"
        DESTINATION "${PANDA_LICENSING_INSTALLDIR_REL}/libm/musl"
        RENAME LICENSE)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/etc/libbambu/libm/newlib/newlib_math.h"
        DESTINATION "${PANDA_INCLUDE_INSTALLDIR_REL}/newlib"
        RENAME math.h)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/etc/libbambu/libm/newlib/LICENSE_INSTALL"
        DESTINATION "${PANDA_LICENSING_INSTALLDIR_REL}/libm/newlib"
        RENAME LICENSE)

# Top-level environment settings (mirrors Automake install-data-hook_settings)
install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/etc/scripts/settings.sh"
        DESTINATION "."
        RENAME settings.sh)

# Inflate requested compiler bundles (mirrors Automake install-data-hook_compilers)
if(PANDA_DIST_COMPILERS)
   # Normalize separators to semicolons
   string(REPLACE "," ";" _panda_compilers "${PANDA_DIST_COMPILERS}")
   string(REPLACE " " ";" _panda_compilers "${_panda_compilers}")
   list(REMOVE_ITEM _panda_compilers "") # drop empties
   list(REMOVE_DUPLICATES _panda_compilers)
   if(EXISTS "${CMAKE_SOURCE_DIR}/.devcontainer/library-scripts/compiler-download.sh")
      install(CODE "
         file(MAKE_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/compilers\")
         foreach(_c ${_panda_compilers})
            execute_process(
               COMMAND bash \"${CMAKE_SOURCE_DIR}/.devcontainer/library-scripts/compiler-download.sh\"
                       \"\${CMAKE_INSTALL_PREFIX}/compilers\" \"\${_c}\" \"${PANDA_DIST_COMPILERS_BACKUP}\"
               WORKING_DIRECTORY \"${CMAKE_SOURCE_DIR}\"
               RESULT_VARIABLE _panda_compiler_download_result
            )
            if(NOT _panda_compiler_download_result EQUAL 0)
               message(FATAL_ERROR \"Failed to install compiler bundle \${_c} into \${CMAKE_INSTALL_PREFIX}/compilers (exit code: \${_panda_compiler_download_result})\")
            endif()
         endforeach()
      ")
      # Cleanup compiler bundles: remove unneeded binaries and static libs
      # (mirrors Automake install-data-hook_cleanup)
      install(CODE "
         execute_process(
            COMMAND bash \"${CMAKE_SOURCE_DIR}/etc/scripts/dist_cleanup.sh\"
                    \"\${CMAKE_INSTALL_PREFIX}\"
            WORKING_DIRECTORY \"${CMAKE_SOURCE_DIR}\"
            RESULT_VARIABLE _panda_cleanup_result
         )
         if(NOT _panda_cleanup_result EQUAL 0)
            message(WARNING \"dist_cleanup.sh returned exit code \${_panda_cleanup_result}\")
         endif()
      ")
      # Wrap compiler bundles in AppImage staging root (if AppImage enabled)
      if(PANDA_APPIMAGE_NAME)
         install(CODE "
            execute_process(
               COMMAND bash \"${CMAKE_SOURCE_DIR}/etc/scripts/appimage_wrap_clang_binaries.sh\"
                       \"\${CMAKE_INSTALL_PREFIX}\"
               WORKING_DIRECTORY \"${CMAKE_SOURCE_DIR}\"
               RESULT_VARIABLE _panda_appimage_wrap_result
            )
            if(NOT _panda_appimage_wrap_result EQUAL 0)
               message(FATAL_ERROR \"Failed to wrap clang compiler bundles in AppImage staging root \${CMAKE_INSTALL_PREFIX} (exit code: \${_panda_appimage_wrap_result})\")
            endif()
         ")
      endif()
   else()
      message(WARNING \"compiler-download.sh not found; skipping PANDA_DIST_COMPILERS install\")
   endif()
endif()

add_subdirectory(etc/libtech)
add_subdirectory(etc/libopenmp)
add_subdirectory(etc/libmdpi)
add_subdirectory(etc/ac_types)
add_subdirectory(etc/ac_math)
add_subdirectory(etc/scripts)
add_subdirectory(ext)
add_subdirectory(etc/clang_plugin)
add_subdirectory(src)
add_subdirectory(etc/libbambu)
add_subdirectory(etc/appimage)

install(CODE [[
   set(_exes bambu eucalyptus bambu-cc)
   set(_libdir "${CMAKE_INSTALL_LIBDIR}")
   if(NOT _libdir)
      set(_libdir "lib")
   endif()
   set(_panda_rel "${PANDA_LIB_INSTALLDIR_REL}")
   if(NOT _panda_rel)
      set(_panda_rel "${_libdir}/panda")
   endif()
   set(_destinations
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${_panda_rel}")
   foreach(_dest IN LISTS _destinations)
      file(MAKE_DIRECTORY "${_dest}")
   endforeach()
   foreach(_exe IN LISTS _exes)
      set(_path "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/${_exe}")
      if(NOT EXISTS "${_path}")
         continue()
      endif()
      string(REPLACE ";" " " _dest_str "${_destinations}")
      # Bundle non-system shared libraries required by each executable.
      # The exclusion list covers only glibc-provided libraries that must come
      # from the host (or the AppImage ld-linux runtime) to avoid ABI mismatches.
      execute_process(COMMAND bash "-c"
          "ldd \"${_path}\" \
           | grep -Ev '(linux-vdso|ld-linux|lib(c|m|dl|pthread|rt|resolv|nss_|nsl)\\.so)' \
           | sed -E 's/.*=> //;s/(.*) .*/\\1/' \
           | grep '^/' \
           | sort -u \
           | while read lib; do \
                if [ ! -e \"$lib\" ]; then continue; fi; \
                real=$(readlink -f \"$lib\"); \
                for dest in ${_dest_str}; do \
                   target=\"$dest/$(basename \"$real\")\"; \
                   if [ ! -e \"$target\" ]; then \
                      install -D \"$real\" \"$target\"; \
                   fi; \
                   base_lib=$(basename \"$lib\"); \
                   if [ \"$(basename \"$real\")\" != \"$base_lib\" ] && [ ! -e \"$dest/$base_lib\" ]; then \
                      ln -sf \"$(basename \"$real\")\" \"$dest/$base_lib\"; \
                   fi; \
                done; \
            done"
      )
   endforeach()
]])

if(PANDA_APPIMAGE_NAME)
   install(CODE [[
      set(_libdir "${CMAKE_INSTALL_LIBDIR}")
      if(NOT _libdir)
         set(_libdir "lib")
      endif()
      set(_panda_rel "${PANDA_LIB_INSTALLDIR_REL}")
      if(NOT _panda_rel)
         set(_panda_rel "${_libdir}/panda")
      endif()
      set(_runtime_libdir "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${_panda_rel}")
      if(EXISTS "${_runtime_libdir}/libstdc++.so.6" AND NOT EXISTS "${_runtime_libdir}/libstdc++.so")
         file(CREATE_LINK "libstdc++.so.6" "${_runtime_libdir}/libstdc++.so" SYMBOLIC)
      endif()
      if(EXISTS "${_runtime_libdir}/libgcc_s.so.1" AND NOT EXISTS "${_runtime_libdir}/libgcc_s.so")
         file(CREATE_LINK "libgcc_s.so.1" "${_runtime_libdir}/libgcc_s.so" SYMBOLIC)
      endif()
   ]])
endif()

# For AppImage builds, fix RPATH of all bundled ELF objects so they can find
# each other inside the mounted AppImage filesystem.
if(PANDA_APPIMAGE_NAME)
   install(CODE "
      execute_process(
         COMMAND bash \"${CMAKE_SOURCE_DIR}/etc/scripts/appimage_fix_rpath.sh\"
                 \"\${CMAKE_INSTALL_PREFIX}\"
         WORKING_DIRECTORY \"${CMAKE_SOURCE_DIR}\"
         RESULT_VARIABLE _panda_rpath_result
      )
      if(NOT _panda_rpath_result EQUAL 0)
         message(WARNING \"appimage_fix_rpath.sh returned exit code \${_panda_rpath_result}\")
      endif()
   ")
endif()

# In host mode the AppImage bundles whatever cmake --install placed into the
# AppDir, so appimage_bundle must run after install.  In container mode the
# container script does its own configure/build/install, so the host install
# step is unnecessary (and its output is discarded by the container cleanup).
if(PANDA_APPIMAGE_NAME AND TARGET appimage_bundle AND TARGET install AND NOT PANDA_APPIMAGE_USE_CONTAINER)
   add_dependencies(appimage_bundle install)
endif()

add_custom_target(check
   COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
   DEPENDS libbambu_unit
)

# Git submodule update helper (mirrors Automake update-submodules)
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git" AND EXISTS "${CMAKE_SOURCE_DIR}/.gitmodules")
   execute_process(
      COMMAND "${GIT_EXECUTABLE}" submodule status --recursive
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      RESULT_VARIABLE _git_submodules_result
      OUTPUT_VARIABLE _git_submodules_output
      ERROR_VARIABLE _git_submodules_error
   )
   if(_git_submodules_result EQUAL 0)
      string(REPLACE "\r\n" "\n" _git_submodules_output "${_git_submodules_output}")
      string(REPLACE "\n" ";" _git_submodules_lines "${_git_submodules_output}")
      set(_git_submodules_bad "")
      foreach(_line IN LISTS _git_submodules_lines)
         if(_line STREQUAL "")
            continue()
         endif()
         string(SUBSTRING "${_line}" 0 1 _git_submodule_status)
         if(_git_submodule_status STREQUAL "-" OR _git_submodule_status STREQUAL "+" OR _git_submodule_status STREQUAL "U")
            set(_git_submodules_bad "1")
            break()
         endif()
      endforeach()
      if(_git_submodules_bad)
         message(FATAL_ERROR
            "Git submodules are not initialized or out of date:\n"
            "${_git_submodules_output}\n"
            "Run: git submodule update --init --recursive")
      endif()
   else()
      message(WARNING "Failed to check git submodules: ${_git_submodules_error}")
   endif()

   add_custom_target(update-submodules
      COMMAND "${GIT_EXECUTABLE}" submodule init
      COMMAND "${GIT_EXECUTABLE}" submodule update
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      COMMENT "Updating git submodules"
      VERBATIM
   )
   add_custom_target(submodules DEPENDS update-submodules)
endif()

# scan-build helper (leverages compile_commands.json generated by CMake)
set(PANDA_SCAN_BUILD_ANALYZER "" CACHE STRING "Override scan-build --use-analyzer path (defaults to CMAKE_C_COMPILER)")
set(PANDA_SCAN_BUILD_CC "" CACHE STRING "Override scan-build --use-cc path (defaults to CMAKE_C_COMPILER)")
set(PANDA_SCAN_BUILD_CXX "" CACHE STRING "Override scan-build --use-c++ path (defaults to CMAKE_CXX_COMPILER)")
set(PANDA_SCAN_BUILD_TARGET "bambu" CACHE STRING "Target to analyze with scan-build (default: bambu)")
set(_panda_sb_analyzer "${PANDA_SCAN_BUILD_ANALYZER}")
if(NOT _panda_sb_analyzer)
   set(_panda_sb_analyzer "${CMAKE_C_COMPILER}")
endif()
set(_panda_sb_cc "${PANDA_SCAN_BUILD_CC}")
if(NOT _panda_sb_cc)
   set(_panda_sb_cc "${CMAKE_C_COMPILER}")
endif()
set(_panda_sb_cxx "${PANDA_SCAN_BUILD_CXX}")
if(NOT _panda_sb_cxx)
   set(_panda_sb_cxx "${CMAKE_CXX_COMPILER}")
endif()
add_custom_target(scan-build
   COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target ext
   COMMAND scan-build${PANDA_SCAN_BUILD_VERSION} -v -v
           --use-analyzer=${_panda_sb_analyzer}
           --use-cc=${_panda_sb_cc}
           --use-c++=${_panda_sb_cxx}
           -o "${PANDA_SCAN_BUILD_REPORT_DIR}"
           ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target ${PANDA_SCAN_BUILD_TARGET}
   WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
   COMMENT "Running scan-build over src (reports in ${PANDA_SCAN_BUILD_REPORT_DIR})"
)

# Package helper (install-strip into /tmp and archive)
set(_panda_dist_name "bambuhls-${PROJECT_VERSION}")
add_custom_target(package
   COMMAND ${CMAKE_COMMAND} -E remove_directory "/tmp/${_panda_dist_name}"
   COMMAND ${CMAKE_COMMAND} -E env DESTDIR="/tmp/${_panda_dist_name}"
           ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target install/strip
   COMMAND ${CMAKE_COMMAND} -E tar "cfJ" "${_panda_dist_name}.tar.xz" "${_panda_dist_name}"
           --format=gnutar
           WORKING_DIRECTORY "/tmp"
   COMMAND ${CMAKE_COMMAND} -E remove_directory "/tmp/${_panda_dist_name}"
   COMMENT "Packaging install/strip into ${_panda_dist_name}.tar.xz"
)

message(STATUS "PandA CMake configuration initialized (bambu, eucalyptus, bambu-cc enabled).")
