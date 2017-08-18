dnl Process this file with autoconf to produce a configure script.

AC_INIT([FloPoCo], [svn-trunk], [BUG_REPORT_ADDRESS])
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
AM_PROG_LEX
AC_PROG_YACC

# we need bison instead of yacc
YACC_TRIMMED=`echo ${YACC}| sed 's/-y//;s/ //'`
if test ${YACC_TRIMMED} != bison; then
  AC_MSG_ERROR(bison is required)
fi
#and flex instead of lex
if test "$LEX" != flex; then
    AC_MSG_ERROR(flex is required)
fi

dnl Sets C++ as main language
AC_LANG([C++])

dnl Enable and Disable values
enableval="yes"
disableval="no"

dnl Disable executable compilation
AC_ARG_ENABLE(	[exec],
		[AS_HELP_STRING([--disable-exec], [disable executable generation, building FloPoCo as a dynamic library only])],
		[EXEC=$disableval],
		[EXEC=$enableval])
AM_CONDITIONAL(BUILD_FLOPOCO_EXEC, test "x$EXEC" = xyes)

dnl Check for libraries.
AC_CHECK_LIB([gmpxx], [main], [], [AC_MSG_ERROR(libgmpxx is missing.)])
AC_CHECK_LIB([gmp], [main], [], [AC_MSG_ERROR(libgmp is missing.)])
AC_CHECK_LIB([mpfr], [main], [], [AC_MSG_ERROR(libmpfr is missing.)])
AC_CHECK_LIB([sollya], [main], [], [AC_MSG_ERROR(libsollya is missing.)])
AC_DEFINE(HAVE_SOLLYA, 1, "Found Sollya")

dnl Check for headers.
AC_CHECK_HEADER([gmpxx.h], [], [AC_MSG_ERROR(gmpxx.h is missing.)])
AC_CHECK_HEADER([gmp.h], [], [AC_MSG_ERROR(gmp.h is missing.)])
AC_CHECK_HEADER([mpfr.h], [], [AC_MSG_ERROR(mpfr.h is missing.)])
dnl need to fix the sollya header test
dnl AC_CHECK_HEADER([sollya.h], [], [AC_MSG_ERROR(sollya.h is missing.)])

dnl These defines do not seem to be used anywhere
dnl AC_DEFINE(HAVE_HOTBM, 1, "HOTBM available")
dnl AC_DEFINE(HAVE_LNS, 1, "LNS available")

dnl Generate output
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([src/Makefile Makefile])
AC_OUTPUT
