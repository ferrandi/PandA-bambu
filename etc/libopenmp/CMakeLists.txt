cmake_minimum_required(VERSION 3.16)

# Install payload matching Automake layout
set(LIBOPENMP_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(LIBOPENMP_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/${PANDA_INSTALL_NAMESPACE}/libopenmp")

set(LIBOPENMP_FILES
   kmp_bambu_names.h
   kmp_bambu.h
   kmp_barrier.cpp
   kmp_csupport.cpp
   kmp_dispatch.cpp
   kmp_dispatch.h
   kmp_os.h
   kmp_runtime.cpp
   kmp_sched.cpp
   kmp_single_file.cpp
   kmp.h
   LICENSE.txt
   omp.cpp
   omp.h
)

install(FILES ${LIBOPENMP_FILES}
        DESTINATION "${LIBOPENMP_INSTALL_DIR}")
install(FILES "${LIBOPENMP_SRC_DIR}/LICENSE_INSTALL"
        DESTINATION "${LIBOPENMP_INSTALL_DIR}"
        RENAME LICENSE)
