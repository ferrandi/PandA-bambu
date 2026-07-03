cmake_minimum_required(VERSION 3.16)

set(LIBMDPI_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/${PANDA_INSTALL_NAMESPACE}/libmdpi")
set(LIBMDPI_INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}/${PANDA_INSTALL_NAMESPACE}/mdpi")

set(LIBMDPI_DATA_FILES
   Makefile.mk
   mdpi_csim.c
   mdpi_driver.cpp
   mdpi_pp.c
   mdpi.c
)

set(LIBMDPI_HEADER_FILES
   include/mdpi/mdpi_bbp.h
   include/mdpi/mdpi_csim.h
   include/mdpi/mdpi_debug.h
   include/mdpi/mdpi_driver.h
   include/mdpi/mdpi_ipc_atomic.h
   include/mdpi/mdpi_ipc_sig.h
   include/mdpi/mdpi_ipc.h
   include/mdpi/mdpi_memmap.h
   include/mdpi/mdpi_pp.h
   include/mdpi/mdpi_stack.h
   include/mdpi/mdpi_types.h
   include/mdpi/mdpi_user.h
   include/mdpi/mdpi_wrapper.h
   include/mdpi/mdpi.h
   include/mdpi/svdpi.h
)

install(FILES ${LIBMDPI_DATA_FILES}
        DESTINATION "${LIBMDPI_INSTALL_DIR}")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE_INSTALL"
        DESTINATION "${LIBMDPI_INSTALL_DIR}"
        RENAME LICENSE)
install(FILES ${LIBMDPI_HEADER_FILES}
        DESTINATION "${LIBMDPI_INCLUDE_INSTALL_DIR}")
