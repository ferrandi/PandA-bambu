cmake_minimum_required(VERSION 3.16)

set(SCRIPTS_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/${PANDA_INSTALL_NAMESPACE}")

install(PROGRAMS
   appimage_build_in_container.sh
   appimage_check_glibc.sh
   hawk.py
   mantis.py
   run_minimal_workflow_tests.sh
   spider.py
   "${CMAKE_SOURCE_DIR}/.devcontainer/library-scripts/compiler-download.sh"
   DESTINATION "${SCRIPTS_INSTALL_DIR}"
)
