if (NOT TARGET sparsepp)
  add_library(sparsepp INTERFACE) # 2018.2
  target_include_directories(sparsepp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/sparsepp)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL "8.1.0")
    target_compile_options(sparsepp INTERFACE -Wno-class-memaccess)
  endif()
endif()

if (NOT TARGET ez)
  add_library(ez INTERFACE) # v1.0
  target_include_directories(ez INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/ez)
endif()

if (NOT TARGET fmt)
  add_library(fmt INTERFACE) # v5.3.0
  target_include_directories(fmt INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/fmt)
  target_compile_definitions(fmt INTERFACE FMT_HEADER_ONLY)
endif()

if (NOT TARGET kitty)
  add_library(kitty INTERFACE) # v0.4
  target_include_directories(kitty INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/kitty)
endif()

if (NOT TARGET range)
  add_library(rang INTERFACE)
  target_include_directories(rang INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/rang)
endif()

if (NOT TARGET lorina)
  add_library(lorina INTERFACE) # v0.1
  target_include_directories(lorina INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lorina)
  target_link_libraries(lorina INTERFACE rang fmt)
endif()

if (NOT TARGET json)
  add_library(json INTERFACE) # v3.5.0
  target_include_directories(json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/json)
endif()

if (NOT TARGET percy)
  add_library(percy INTERFACE) # >v0.1.2
  target_include_directories(percy SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/percy)
  target_compile_definitions(percy INTERFACE DISABLE_NAUTY)

  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  target_link_libraries(percy INTERFACE Threads::Threads)

  set(ABC_USE_NAMESPACE "pabc")
  set(STATIC_LIBABC true)
  add_subdirectory(abcsat)
  if (UNIX)
    target_compile_definitions(libabcsat PUBLIC "LIN64" ABC_NAMESPACE=pabc ABC_NO_USE_READLINE)
  elseif(WIN32)
    target_compile_definitions(libabcsat PUBLIC ABC_NAMESPACE=pabc ABC_USE_NO_READLINE NOMINMAX)
    target_compile_definitions(libabcsat PUBLIC WIN32_NO_DLL)
  endif()
  target_link_libraries(percy INTERFACE libabcsat)
endif()
