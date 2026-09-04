if(NOT PANDA_APPIMAGE_NAME)
   return()
endif()

set(_appdir "${PANDA_APPIMAGE_ROOT}")
set(_appimage "${_appdir}/${PANDA_APPIMAGE_NAME}.AppImage")
set(_compat_cache_host "${CMAKE_BINARY_DIR}/appimage-compat-cache.cmake")
set(_compat_build_container "/build/appimage-compat-build")
set(_compat_cache_container "/build/appimage-compat-cache.cmake")
set(_compat_appdir_container "/appdir")

# Determine default binary for commander wrapper
if(PANDA_BUILD_BAMBU)
   set(_dist_default_exe "bambu")
elseif(PANDA_BUILD_CC)
   set(_dist_default_exe "bambu-cc")
elseif(PANDA_BUILD_EUCALYPTUS)
   set(_dist_default_exe "eucalyptus")
else()
   set(_dist_default_exe "bambu")
endif()

# Provides list for appdata.xml
set(_provides "")
if(PANDA_BUILD_EUCALYPTUS)
   list(APPEND _provides "<binary>eucalyptus</binary>")
endif()
if(PANDA_BUILD_BAMBU)
   list(APPEND _provides "<binary>bambu</binary>")
endif()
if(PANDA_BUILD_CC)
   list(APPEND _provides "<binary>bambu-cc</binary>")
endif()
string(JOIN "" APPIMAGE_PROVIDES "${_provides}")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/appdata.xml.in"
               "${CMAKE_CURRENT_BINARY_DIR}/appdata.xml" @ONLY)
set(DIST_DEFAULT_EXE "${_dist_default_exe}")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/commander.in"
               "${CMAKE_CURRENT_BINARY_DIR}/commander" @ONLY)

# Determine whether AppImage packaging should run on the host or inside the
# legacy-compatible container used by CI.
set(_panda_appimage_build_env "${PANDA_APPIMAGE_BUILD_ENV}")
string(TOLOWER "${_panda_appimage_build_env}" _panda_appimage_build_env)
set(_panda_container_runtime "${PANDA_APPIMAGE_CONTAINER_RUNTIME}")
string(TOLOWER "${_panda_container_runtime}" _panda_container_runtime)
if(NOT _panda_container_runtime OR _panda_container_runtime STREQUAL "auto")
   find_program(_panda_docker docker)
   find_program(_panda_podman podman)
   if(_panda_docker)
      set(_panda_container_runtime "docker")
   elseif(_panda_podman)
      set(_panda_container_runtime "podman")
   else()
      set(_panda_container_runtime "")
   endif()
endif()

set(_panda_use_container FALSE)
if(_panda_appimage_build_env STREQUAL "container")
   if(NOT _panda_container_runtime)
      message(FATAL_ERROR "PANDA_APPIMAGE_BUILD_ENV=container requires docker or podman")
   endif()
   set(_panda_use_container TRUE)
elseif(_panda_appimage_build_env STREQUAL "auto")
   if(_panda_container_runtime)
      set(_panda_use_container TRUE)
   endif()
elseif(NOT _panda_appimage_build_env STREQUAL "host")
   message(FATAL_ERROR "Unsupported PANDA_APPIMAGE_BUILD_ENV='${PANDA_APPIMAGE_BUILD_ENV}' (expected auto, host, or container)")
endif()

# Propagate container flag so the top-level CMakeLists can skip the
# appimage_bundle → install dependency when the build runs in a container.
set(PANDA_APPIMAGE_USE_CONTAINER ${_panda_use_container} PARENT_SCOPE)

# Fetch AppRun loader
set(_apprun "${CMAKE_CURRENT_BINARY_DIR}/AppRun")
if(NOT EXISTS "${_apprun}")
   file(DOWNLOAD
      "https://github.com/AppImage/AppImageKit/releases/download/continuous/AppRun-x86_64"
      "${_apprun}"
      SHOW_PROGRESS)
endif()
file(CHMOD "${_apprun}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

# Install AppImage scaffold into appdir
install(PROGRAMS "${_apprun}"
        DESTINATION "${_appdir}"
        RENAME AppRun)
install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/commander"
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
        RENAME commander)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/appdata.xml"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
        RENAME eu.bambuhls.panda.appdata.xml)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/panda.png"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps"
        RENAME eu.bambuhls.panda.png)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/panda.desktop"
        DESTINATION "${_appdir}"
        RENAME eu.bambuhls.panda.desktop)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/panda.png"
        DESTINATION "${_appdir}"
        RENAME eu.bambuhls.panda.png)

# Package AppImage on the host (requires appimagetool)
set(_runtime_arg "")
set(_runtime_path "${PANDA_APPIMAGE_RUNTIME_FILE}")
if(NOT _runtime_path AND NOT _panda_use_container)
   set(_runtime_path "${_appdir}/AppRun.runtime")
   if(NOT EXISTS "${_runtime_path}")
      file(DOWNLOAD
         "https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64"
         "${_runtime_path}"
         SHOW_PROGRESS)
      file(CHMOD "${_runtime_path}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
   endif()
endif()
if(_runtime_path AND EXISTS "${_runtime_path}")
   set(_runtime_arg --runtime-file "${_runtime_path}")
endif()

set(_appimage_arch "x86_64")
find_program(APPIMAGETOOL appimagetool)
if(APPIMAGETOOL)
   add_custom_target(appimage_bundle_host
      COMMAND "${CMAKE_COMMAND}" -E rm -f "${_appimage}"
      COMMAND bash "${CMAKE_SOURCE_DIR}/etc/scripts/appimage_check_glibc.sh" "${_appdir}/usr" "${PANDA_APPIMAGE_MAX_GLIBC}"
      COMMAND env APPIMAGE_EXTRACT_AND_RUN=1 APPIMAGE_ALLOW_UNSUPPORTED_FILESYSTEMS=1 ARCH=${_appimage_arch} "${APPIMAGETOOL}" -n ${_runtime_arg} "${_appdir}" "${_appimage}"
      DEPENDS "${_apprun}"
              "${CMAKE_CURRENT_BINARY_DIR}/appdata.xml"
              "${CMAKE_CURRENT_BINARY_DIR}/commander"
              "${CMAKE_SOURCE_DIR}/etc/scripts/appimage_check_glibc.sh"
      BYPRODUCTS "${_appimage}"
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
      VERBATIM
   )
   add_dependencies(appimage_bundle_host install)
else()
   if(NOT _panda_use_container)
      message(STATUS "appimagetool not found; host AppImage bundle targets not created")
   endif()
endif()

if(_panda_use_container)
   configure_file("${CMAKE_CURRENT_SOURCE_DIR}/appimage_compat_cache_init.cmake.in"
                  "${_compat_cache_host}" @ONLY)
   set(_container_binds "")
   if(PANDA_DIST_COMPILERS_BACKUP AND IS_ABSOLUTE "${PANDA_DIST_COMPILERS_BACKUP}")
      list(APPEND _container_binds --bind "${PANDA_DIST_COMPILERS_BACKUP}")
   endif()
   if(_runtime_path AND IS_ABSOLUTE "${_runtime_path}")
      list(APPEND _container_binds --bind "${_runtime_path}")
   endif()
   add_custom_target(appimage_bundle
      COMMAND bash "${CMAKE_SOURCE_DIR}/etc/scripts/appimage_build_in_container.sh"
              --source-dir "${CMAKE_SOURCE_DIR}"
              --build-dir "${CMAKE_BINARY_DIR}"
              --appdir "${_appdir}"
              --cache-init "${_compat_cache_host}"
              --cache-init-in-container "${_compat_cache_container}"
              --build-dir-in-container "${_compat_build_container}"
              --appdir-in-container "${_compat_appdir_container}"
              --generator "${PANDA_APPIMAGE_CONTAINER_GENERATOR}"
              --parallel "${PANDA_APPIMAGE_PARALLEL}"
              --compilers "${PANDA_DIST_COMPILERS}"
              --compilers-backup "${PANDA_DIST_COMPILERS_BACKUP}"
              --appimage-name "${PANDA_APPIMAGE_NAME}"
              --appimage-arch "${_appimage_arch}"
              --max-glibc "${PANDA_APPIMAGE_MAX_GLIBC}"
              --runtime-file "${_runtime_path}"
              --runtime "${_panda_container_runtime}"
              --image "${PANDA_APPIMAGE_CONTAINER_IMAGE}"
              ${_container_binds}
      DEPENDS "${_compat_cache_host}"
              "${CMAKE_SOURCE_DIR}/etc/scripts/appimage_build_in_container.sh"
              "${CMAKE_SOURCE_DIR}/etc/scripts/appimage_check_glibc.sh"
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
      VERBATIM
   )
elseif(TARGET appimage_bundle_host)
   add_custom_target(appimage_bundle DEPENDS appimage_bundle_host)
endif()

if(TARGET appimage_bundle)
   add_custom_target(appimage DEPENDS appimage_bundle)
endif()
