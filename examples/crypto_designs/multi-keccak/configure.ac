AC_INIT(multi-keccac, 1.0, fabrizio.ferrandi@polimi.it)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR(Makefile.am)
AC_CONFIG_MACRO_DIR(m4)
AM_INIT_AUTOMAKE(-Wno-portability foreign)
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

LT_INIT([disable-shared])
AC_PROG_CC
AC_PROG_INSTALL
CFLAGS=' -O3'
CC=tree-panda-gcc
AC_OUTPUT(Makefile include/Makefile src/Makefile src/chi/Makefile src/iota/Makefile src/pi/Makefile src/rho/Makefile  src/theta/Makefile)
