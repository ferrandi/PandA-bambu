dnl Process this file with autoconf to produce a configure script.

AC_INIT([Abseil-cpp-library],[svn-trunk],[BUG_REPORT_ADDRESS])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])	

dnl Check for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL

dnl Sets C++ as main language
AC_LANG([C++])


dnl Generate output
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
