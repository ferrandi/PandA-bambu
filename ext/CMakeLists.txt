# Stage-1 dependency setup for CMake port.

add_library(panda_ext_headers INTERFACE)
target_include_directories(panda_ext_headers INTERFACE
   "${CMAKE_CURRENT_SOURCE_DIR}"
   "${CMAKE_CURRENT_SOURCE_DIR}/or-tools"
)
target_include_directories(panda_ext_headers SYSTEM INTERFACE
   "${CMAKE_CURRENT_SOURCE_DIR}/abseil/abseil-cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/or-tools"
)

# abseil
set(ABSL_PROPAGATE_CXX_STD ON CACHE INTERNAL "" FORCE)
add_subdirectory(abseil/abseil-cpp EXCLUDE_FROM_ALL)
add_library(panda_ext_absl INTERFACE)
target_link_libraries(panda_ext_absl INTERFACE absl::hash absl::raw_hash_set absl::flat_hash_map)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/abseil/abseil-cpp/LICENSE"
              "${CMAKE_CURRENT_SOURCE_DIR}/abseil/abseil-cpp/AUTHORS"
        DESTINATION "${PANDA_LICENSING_INSTALLDIR_REL}/abseil")

# pugixml
set(PUGIXML_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(PUGIXML_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(PUGIXML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(pugixml EXCLUDE_FROM_ALL)
add_library(panda_ext_pugixml INTERFACE)
target_link_libraries(panda_ext_pugixml INTERFACE pugixml)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/pugixml/LICENSE.md"
        DESTINATION "${PANDA_LICENSING_INSTALLDIR_REL}/pugixml"
        RENAME LICENSE)
target_compile_definitions(panda_ext_pugixml INTERFACE
   PUGIXML_NO_EXCEPTIONS
   PUGIXML_NO_XPATH
)

# mockturtle (header-only use)
add_library(panda_ext_mockturtle INTERFACE)
target_include_directories(panda_ext_mockturtle SYSTEM INTERFACE
   "${CMAKE_CURRENT_SOURCE_DIR}/mockturtle/mockturtle/include"
   "${CMAKE_CURRENT_SOURCE_DIR}/mockturtle/mockturtle/lib/abcesop"
   "${CMAKE_CURRENT_SOURCE_DIR}/mockturtle/mockturtle/lib/abcsat"
   "${CMAKE_CURRENT_SOURCE_DIR}/mockturtle/mockturtle/lib/bill"
   "${CMAKE_CURRENT_SOURCE_DIR}/mockturtle/mockturtle/lib/fmt"
   "${CMAKE_CURRENT_SOURCE_DIR}/mockturtle/mockturtle/lib/kitty"
   "${CMAKE_CURRENT_SOURCE_DIR}/mockturtle/mockturtle/lib/lorina"
   "${CMAKE_CURRENT_SOURCE_DIR}/mockturtle/mockturtle/lib/parallel_hashmap"
   "${CMAKE_CURRENT_SOURCE_DIR}/mockturtle/mockturtle/lib/percy"
   "${CMAKE_CURRENT_SOURCE_DIR}/mockturtle/mockturtle/lib/rang"
)

# glaze (header-only)
add_library(panda_ext_glaze INTERFACE)
target_include_directories(panda_ext_glaze SYSTEM INTERFACE
   "${CMAKE_CURRENT_SOURCE_DIR}/glaze/include"
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/glaze/LICENSE"
        DESTINATION "${PANDA_LICENSING_INSTALLDIR_REL}/glaze"
        RENAME LICENSE)

# BuDDy packaged build
add_library(panda_ext_buddy INTERFACE)
add_subdirectory(buddy EXCLUDE_FROM_ALL)
target_link_libraries(panda_ext_buddy INTERFACE buddy)
set(buddy_license "${CMAKE_CURRENT_BINARY_DIR}/buddy/LICENSE")
add_custom_command(OUTPUT "${buddy_license}"
   COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/buddy"
   COMMAND bash -lc "set -euo pipefail; head -n32 \"${CMAKE_CURRENT_SOURCE_DIR}/buddy/README\" > \"${buddy_license}\""
   DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/buddy/README"
   VERBATIM
   COMMENT "Preparing BuDDy license snippet")
add_custom_target(panda_buddy_license ALL DEPENDS "${buddy_license}")
install(FILES "${buddy_license}"
        DESTINATION "${PANDA_LICENSING_INSTALLDIR_REL}/libbdd"
        RENAME LICENSE)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/mockturtle/mockturtle/LICENSE"
        DESTINATION "${PANDA_LICENSING_INSTALLDIR_REL}/mockturtle"
        RENAME LICENSE)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/or-tools/LICENSE"
        DESTINATION "${PANDA_LICENSING_INSTALLDIR_REL}/or-tools"
        RENAME LICENSE)
# Aggregate target used by scan-build (works with Ninja/Make)
add_custom_target(ext)
add_dependencies(ext
   panda_ext_headers
   panda_ext_absl
   panda_ext_pugixml
   panda_ext_mockturtle
   panda_ext_glaze
   panda_ext_buddy
)
