if (NOT TARGET parallel_hashmap)
  add_library(parallel_hashmap INTERFACE) # 2020.11
  target_include_directories(parallel_hashmap SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/parallel_hashmap)
endif()

if (NOT TARGET fmt)
  add_library(fmt INTERFACE) # v6.3.0
  target_include_directories(fmt SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/fmt)
  target_compile_definitions(fmt INTERFACE FMT_HEADER_ONLY)
endif()

if (NOT TARGET kitty)
  add_library(kitty INTERFACE) # v0.4
  target_include_directories(kitty SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/kitty)
endif()

if (NOT TARGET range)
  add_library(rang INTERFACE)
  target_include_directories(rang SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/rang)
endif()

if (NOT TARGET lorina)
  add_library(lorina INTERFACE) # v0.1
  target_include_directories(lorina SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lorina)
  target_link_libraries(lorina INTERFACE rang fmt)
endif()

if (NOT TARGET json)
  add_library(json INTERFACE) # v3.5.0
  target_include_directories(json SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/json)
endif()

if (NOT TARGET percy)
  add_library(percy INTERFACE) # >v0.1.2
  target_include_directories(percy SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/percy)

  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  target_link_libraries(percy INTERFACE Threads::Threads)

  set(ABC_USE_NAMESPACE "pabc")
  set(STATIC_LIBABC true)
  add_subdirectory(abcsat)
  if (UNIX)
    target_compile_definitions(libabcsat PUBLIC "LIN64" ABC_NAMESPACE=pabc ABC_NO_USE_READLINE)
  elseif(WIN32)
    target_compile_definitions(libabcsat PUBLIC ABC_NAMESPACE=pabc ABC_USE_NO_READLINE NOMINMAX  WIN32_NO_DLL _CRT_SECURE_NO_WARNINGS)
  endif()
  target_link_libraries(percy INTERFACE libabcsat)

  if (ENABLE_NAUTY)
    add_subdirectory(nauty)
    target_link_libraries(percy INTERFACE nauty)
  else()
    target_compile_definitions(percy INTERFACE DISABLE_NAUTY)
  endif()
endif()

if (NOT TARGET bill)
  add_library(bill INTERFACE)
  target_include_directories(bill SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/bill)

  if(BILL_Z3)
    target_compile_definitions(bill INTERFACE BILL_HAS_Z3)
    set(BILL_Z3_INCLUDE_PATH "" CACHE PATH "Path to Z3 includes, e.g., z3++.h")
    set(BILL_Z3_LIBRARY_PATH "" CACHE PATH "Path to Z3 library, e.g., libz3.a")
    if(NOT "${BILL_Z3_INCLUDE_PATH}" STREQUAL "")
      target_include_directories(bill SYSTEM INTERFACE ${BILL_Z3_INCLUDE_PATH})
    endif()
    if(NOT "${BILL_Z3_LIBRARY_PATH}" STREQUAL "")
      target_link_directories(bill INTERFACE ${BILL_Z3_LIBRARY_PATH})
    endif()
    if (WIN32)
      target_link_libraries(bill INTERFACE libz3)
    else()
      target_link_libraries(bill INTERFACE z3)
    endif()
  endif()
endif()

if (NOT TARGET abcresub)
  add_library(abcresub INTERFACE)
  target_include_directories(abcresub SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/abcresub)
endif()

if (NOT TARGET libabcesop)
  set(STATIC_LIBABC true)
  add_subdirectory(abcesop)
endif()

if (ENABLE_MATPLOTLIB AND NOT TARGET matplot)
  find_package(Python3 COMPONENTS Development NumPy)
  add_library(matplot INTERFACE)
  target_include_directories(matplot SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/matplot/ ${Python3_INCLUDE_DIRS} ${Python3_NumPy_INCLUDE_DIRS})
  target_link_libraries(matplot INTERFACE Python3::Python Python3::NumPy)
endif()
