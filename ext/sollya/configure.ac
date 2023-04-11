#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([sollya],[3.0],[christoph.lauter@ens-lyon.org, sylvain.chevillard@ens-lyon.org, mioara.joldes@ens-lyon.fr])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.6 no-define dist-zip dist-bzip2])

 
# Checks for programs.
AC_PROG_YACC
AC_PROG_CC
AC_PROG_CXX 
AM_PROG_CC_C_O
AC_LANG_PUSH([C++])
AC_MSG_CHECKING([whether C++ compiler works])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdlib.h>]]
[[]] 
[[]]
, [[exit(0);]])],
[AC_MSG_RESULT(yes) 
],[
AC_MSG_ERROR(no)		
])
AC_LANG_POP([C++])	
AC_PROG_LEX
if test "$LEX" != flex; then
  LEX="$SHELL $missing_dir/missing flex"
  AC_SUBST(LEX_OUTPUT_ROOT, lex.yy)
  AC_SUBST(LEXLIB, '')
fi
#AC_PROG_RANLIB
_LT_SET_OPTION([LT_INIT],[dlopen])
m4_warn([obsolete],[AC_LIBTOOL_DLOPEN: Remove this warning and the call to _LT_SET_OPTION when you
put the 'dlopen' option into LT_INIT's first parameter.])

#AC_DISABLE_SHARED
LT_INIT
AC_SUBST(LIBTOOL_DEPS)
AC_CONFIG_MACRO_DIR([m4])

#Check whether flex and bison support interactive and pure parsers

AC_ARG_ENABLE(strict-lexer-parser-check, [  --enable-strict-lexer-parser-check enables strict compatibility checking for building the lexer and parser files], [STRICTFLEXBISONTESTING="1"], [STRICTFLEXBISONTESTING=""])

AC_MSG_CHECKING([flex supports interactive and pure lexers])
if test `echo -e "%{\n%}\n\n%option always-interactive\n%option reentrant\n%option prefix=\"coucou\"\n%option bison-bridge\n%option nounput\n\n%%\n"test" { }\n%%\n" > conftest.l ; $LEX conftest.l > /dev/null 2> /dev/null ; echo $?; rm -f lex.coucou.c` -eq 0 ; then 
AC_MSG_RESULT(yes)
else
if test "x$STRICTFLEXBISONTESTING" = "x"; then
   AC_MSG_RESULT(flex version unable to compile the lexer from the grammar files)
else
   AC_MSG_ERROR(no)	
fi
fi

AC_MSG_CHECKING([bison supports interactive and pure parsers])
if test `echo -e "%{\n#define YYERROR_VERBOSE 1\n#define YYPARSE_PARAM scanner\n#define YYLEX_PARAM scanner\n%}\n\n%defines\n%pure_parser\n\n%%\nstart: \".\"\n{ }\n;\n\n" > conftest.y ; $YACC conftest.y > /dev/null 2> /dev/null ; echo $?; rm -f y.tab.*` -eq 0 ; then 
AC_MSG_RESULT(yes)
else
if test "x$STRICTFLEXBISONTESTING" = "x"; then
   AC_MSG_RESULT(bison version unable to compile the parser from the grammar files)
else
   AC_MSG_ERROR(no)	
fi
fi

# withs...
FPLLLVERSIONSTRING=""

WITH_LIB_GIVEN="no"
AC_ARG_WITH(gmp-include,
   [  --with-gmp-include=DIR  GMP include directory ],
   AM_CPPFLAGS="$AM_CPPFLAGS -I$withval")
AC_ARG_WITH(gmp-lib,
   [  --with-gmp-lib=DIR      GMP lib directory ], [
   WITH_LIB_GIVEN="yes"
   AM_LDFLAGS="$AM_LDFLAGS -L$withval"
  ])
AC_ARG_WITH(gmp,
   [  --with-gmp=DIR          GMP install directory ], [
   AM_CPPFLAGS="$AM_CPPFLAGS -I$withval/include"
   AM_LDFLAGS="$AM_LDFLAGS -L$withval/lib"
   WITH_LIB_GIVEN="yes"	   
  ])

AC_ARG_WITH(mpfr-include,
   [  --with-mpfr-include=DIR  MPFR include directory ],
   AM_CPPFLAGS="$AM_CPPFLAGS -I$withval")
AC_ARG_WITH(mpfr-lib,
   [  --with-mpfr-lib=DIR      MPFR lib directory ], [
   AM_LDFLAGS="$AM_LDFLAGS -L$withval"
   WITH_LIB_GIVEN="yes"
  ])
AC_ARG_WITH(mpfr,
   [  --with-mpfr=DIR          MPFR install directory ], [
   AM_CPPFLAGS="$AM_CPPFLAGS -I$withval/include"
   AM_LDFLAGS="$AM_LDFLAGS -L$withval/lib"
   WITH_LIB_GIVEN="yes"	   
  ])

AC_ARG_WITH(fplll-include,
   [  --with-fplll-include=DIR  fplll include directory ],
   [AM_CPPFLAGS="$AM_CPPFLAGS -I$withval"
    FPLLLVERSIONSTRING=", --with-fplll-include given as '$withval'"
   ])
AC_ARG_WITH(fplll-lib,
   [  --with-fplll-lib=DIR      fplll lib directory ], [
   AM_LDFLAGS="$AM_LDFLAGS -L$withval"
   FPLLLVERSIONSTRING=", --with-fplll-lib given as '$withval'"
   WITH_LIB_GIVEN="yes"
  ])
AC_ARG_WITH(fplll,
   [  --with-fplll=DIR          fplll install directory ], [
   AM_CPPFLAGS="$AM_CPPFLAGS -I$withval/include"
   AM_LDFLAGS="$AM_LDFLAGS -L$withval/lib"
   FPLLLVERSIONSTRING=", --with-fplll given as '$withval'"
   WITH_LIB_GIVEN="yes"
  ])

AC_ARG_WITH(z-include,
   [  --with-z-include=DIR  z include directory ],
   AM_CPPFLAGS="$AM_CPPFLAGS -I$withval")
AC_ARG_WITH(z-lib,
   [  --with-z-lib=DIR      z lib directory ], [
   AM_LDFLAGS="$AM_LDFLAGS -L$withval"
   WITH_LIB_GIVEN="yes"
  ])
AC_ARG_WITH(z,
   [  --with-z=DIR          z install directory ], [
   AM_CPPFLAGS="$AM_CPPFLAGS -I$withval/include"
   AM_LDFLAGS="$AM_LDFLAGS -L$withval/lib"
   WITH_LIB_GIVEN="yes"
  ])

AC_ARG_WITH(iconv-include,
   [  --with-iconv-include=DIR  iconv include directory ],
   AM_CPPFLAGS="$AM_CPPFLAGS -I$withval")
AC_ARG_WITH(iconv-lib,
   [  --with-iconv-lib=DIR      iconv lib directory ], [
   AM_LDFLAGS="$AM_LDFLAGS -L$withval"
   WITH_LIB_GIVEN="yes"
  ])
AC_ARG_WITH(iconv,
   [  --with-iconv=DIR          iconv install directory ], [
   AM_CPPFLAGS="$AM_CPPFLAGS -I$withval/include"
   AM_LDFLAGS="$AM_LDFLAGS -L$withval/lib"
   WITH_LIB_GIVEN="yes"
  ])


XML2INCLUDEGIVEN=""
XML2LIBGIVEN=""
AC_ARG_WITH(xml2-include,
   [  --with-xml2-include=DIR  XML2 include directory ],[
   AM_CPPFLAGS="$AM_CPPFLAGS -I$withval -I$withval/libxml2"
   XML2INCLUDEGIVEN=1
   ])
AC_ARG_WITH(xml2-lib,
   [  --with-xml2-lib=DIR      XML2 lib directory ], [
   AM_LDFLAGS="$AM_LDFLAGS -L$withval"
   XML2LIBGIVEN=1
   WITH_LIB_GIVEN="yes"
  ])
AC_ARG_WITH(xml2,
   [  --with-xml2=DIR          XML2 install directory ], [
   AM_CPPFLAGS="$AM_CPPFLAGS -I$withval/include -I$withval/include/libxml2"
   AM_LDFLAGS="$AM_LDFLAGS -L$withval/lib"
   XML2INCLUDEGIVEN=1
   XML2LIBGIVEN=1
   WITH_LIB_GIVEN="yes"
  ])

AC_ARG_WITH(mpfi-include,
   [  --with-mpfi-include=DIR  MPFI include directory ],
   AM_CPPFLAGS="$AM_CPPFLAGS -I$withval")
AC_ARG_WITH(mpfi-lib,
   [  --with-mpfi-lib=DIR      MPFI lib directory ], [
   AM_LDFLAGS="$AM_LDFLAGS -L$withval"
   WITH_LIB_GIVEN="yes"
  ])
AC_ARG_WITH(mpfi,
   [  --with-mpfi=DIR          MPFI install directory ], [
   AM_CPPFLAGS="$AM_CPPFLAGS -I$withval/include"
   AM_LDFLAGS="$AM_LDFLAGS -L$withval/lib"
   WITH_LIB_GIVEN="yes"
  ])


# The xml2 include and lib can be found either by xml2-config or pkg-config
# depending on what is available or provided by the user.
XML2CONFIGGIVEN=""
XML2CONFIG=""
AC_ARG_WITH(xml2-config,
            [  --with-xml2-config=executable XML2 config tool ],
            [if test "x$withval" != "xyes" -a "x$withval" != "x"
             then
               XML2CONFIG="$withval"
               XML2CONFIGGIVEN=1
	     fi
            ])

PKGCONFIGGIVEN=""
PKGCONFIG=""
AC_ARG_WITH(pkg-config,
            [  --with-pkg-config=executable pkg-config tool ],
            [if test "x$withval" != "xyes" -a "x$withval" != "x"
             then
               PKGCONFIG="$withval"
               PKGCONFIGGIVEN=1
	     fi
            ])

# If none of xml2-config and pkg-config is provided by the user, we pick one
if test "x$XML2CONFIGGIVEN" = "x" -a "x$PKGCONFIGGIVEN" = "x"
then
  AC_CHECK_PROG(PKGCONFIG, pkg-config, [pkg-config])
  if test "x$PKGCONFIG" = "x"
  then
    AC_CHECK_PROG(XML2CONFIG, xml2-config, [xml2-config])
  fi
fi

XML2CFLAGS=""
XML2LIBS=""
if test "x$PKGCONFIG" != "x"
then
  XML2CFLAGS=`$PKGCONFIG --cflags libxml-2.0 2> /dev/null`
  XML2LIBS=`$PKGCONFIG --libs libxml-2.0 --static 2> /dev/null`
else
  if test "x$XML2CONFIG" != "x"
  then
    XML2CFLAGS=`$XML2CONFIG --cflags`
    XML2LIBS=`$XML2CONFIG --libs`
  fi
fi
if test "x$XML2INCLUDEGIVEN" = "x" -a "x$XML2CFLAGS" != "x"
then
  AM_CPPFLAGS="$AM_CPPFLAGS $XML2CFLAGS"
fi

if test "x$XML2LIBGIVEN" = "x" -a "x$XML2LIBS" != "x"
then
  AC_SEARCH_LIBS([xmlTextReaderIsEmptyElement], [xml2], [LIBS="$LIBS $XML2LIBS"], [], [])
fi


# Save user flags

OLDCFLAGS="$CFLAGS"
OLDCPPFLAGS="$CPPFLAGS"
OLDLDFLAGS="$LDFLAGS"

# Checks for libraries.

CPPFLAGS="$AM_CPPFLAGS $CPPFLAGS"
LDFLAGS="$AM_LDFLAGS $LDFLAGS"

dlopen_ok="yes"
AC_CHECK_LIB([c], [dlopen], [], [dlopen_ok="no"])
if [[ $dlopen_ok == "no" ]] ; then
  AC_CHECK_LIB([dl], [dlopen], [], AC_MSG_ERROR([libdl unusable]))
fi
AC_CHECK_LIB([gmp], [__gmpz_init], [], AC_MSG_ERROR([libgmp unusable]))
AC_CHECK_LIB([gmp], [__gmpz_fac_ui], [], AC_MSG_ERROR([libgmp version too old]))
AC_CHECK_LIB([gmp], [__gmpz_bin_uiui], [], AC_MSG_ERROR([libgmp version too old]))
AC_CHECK_LIB([gmp], [__gmpz_export], [], AC_MSG_ERROR([libgmp version too old]))
AC_CHECK_LIB([mpfr], [mpfr_init], [], AC_MSG_ERROR([libmpfr unusable]))
AC_CHECK_LIB([mpfr], [mpfr_erfc], [], AC_MSG_ERROR([libmpfr version too old]))
AC_CHECK_LIB([z], [gzdopen], [], [])
AC_CHECK_LIB([iconv], [libiconv_open], [], [])
AC_CHECK_LIB([m],[cos])
AC_CHECK_LIB([lzma], [lzma_code], [], AC_MSG_ERROR([libxml2 unusable because of missing liblzma]))

#Special check for fplll
FPLLLUSABLE="yes"
FPLLLSPECIALINCLUDE="no"
FPLLLISREALLIB="yes"
AC_LANG_PUSH([C++])
AC_MSG_CHECKING([if fplll is a real library to be linked in])
FPLLLOLDCFLAGS="$CFLAGS"
FPLLLOLDCPPFLAGS="$CPPFLAGS"
FPLLLOLDLDFLAGS="$LDFLAGS"
LIBS="-lfplll $LIBS"	
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdlib.h>]]
[[#include <fplll.h>]] 
[[#include <gmp.h>]]
, [[ZZ_mat<mpz_t> M(3, 3); M.gen_uniform(3); lllReduction(M, 0.99, 0.51, LM_WRAPPER); return 0;]])],
[AC_MSG_RESULT(yes) 
],[FPLLLISREALLIB="no"
AC_MSG_RESULT(no) ])
CFLAGS="$FPLLLOLDCFLAGS"
CPPFLAGS="$FPLLLOLDCPPFLAGS"
LDFLAGS="$FPLLLOLDLDFLAGS"
if [[ $FPLLLISREALLIB == "no" ]] ; then
AC_MSG_CHECKING([for fplll support, fplll being just a header file])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdlib.h>]]
[[#include <fplll.h>]] 
[[#include <gmp.h>]]
, [[ZZ_mat<mpz_t> * M; M = new ZZ_mat<mpz_t>(3,3); if(M!=NULL) exit(0); else exit(1);]])],
[AC_MSG_RESULT(yes) 
FPLLLVERSIONSTRING="FPLLL: just a header file, directly in include directory$FPLLLVERSIONSTRING"
],[FPLLLUSABLE="no"
AC_MSG_RESULT(no) 
])
else
# Add -lfplll to LFLAGS
AM_LDFLAGS="-lfplll $AM_LDFLAGS"	
FPLLLVERSIONSTRING="FPLLL: a real library, linked with -lfplll$FPLLLVERSIONSTRING"
fi
if [[ $FPLLLUSABLE == "no" ]] ; then
FPLLLSPECIALINCLUDE="yes"
AC_MSG_CHECKING([for Debian-like fplll support, fplll being just a header file])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdlib.h>]]
[[#include <fplll/fplll.h>]] 
[[#include <gmp.h>]]
, [[ZZ_mat<mpz_t> * M; M = new ZZ_mat<mpz_t>(3,3); if(M!=NULL) exit(0); else exit(1);]])],
[AC_MSG_RESULT(yes) 
FPLLLSPECIALINCLUDE="yes"
FPLLLVERSIONSTRING="FPLLL: just a header file, in a subdirectory of the include directory (Debian-like)$FPLLLVERSIONSTRING"
],[
AC_MSG_ERROR([fplll unusable (or g++ too new -- see README)])
FPLLLVERSIONSTRING="FPLLL: unusable or g++ too new, $FPLLLVERSIONSTRING"
])
fi
if [[ $FPLLLSPECIALINCLUDE == "yes" ]] ; then 
AC_DEFINE([HAVE_SPECIAL_FPLLL_INCLUDE],[1],[Define for determining a special version of FPLLL])
else
AC_DEFINE([HAVE_SPECIAL_FPLLL_INCLUDE],[0],[Define for determining a special version of FPLLL])
fi
AC_DEFINE_UNQUOTED([HAVE_FPLLL_VERSION_STRING],["$FPLLLVERSIONSTRING"],[Define a kind of a version string for FPLLL])

AC_CHECK_LIB([mpfi], [mpfi_init], [], AC_MSG_ERROR([libmpfi unusable]))
AC_LANG_POP([C++])	

#Check for -Wl,-export-dynamic etc.

CPPFLAGS="$OLDCPPFLAGS"
CFLAGS="$OLDCFLAGS -fPIC"
LDFLAGS="$OLDLDFLAGS"


AC_MSG_CHECKING([for -fPIC support])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
[AC_MSG_RESULT(yes) 
AM_CFLAGS="$AM_CFLAGS -fPIC"
],[AC_MSG_RESULT(no)
])

CFLAGS="$OLDCFLAGS"
LDFLAGS="$OLDLDFLAGS -Wl,-export-dynamic"

AC_MSG_CHECKING([for -export-dynamic support])
AC_RUN_IFELSE([AC_LANG_PROGRAM([[#include <stdlib.h>]], [[exit(0);]])],
[AC_MSG_RESULT(yes) 
AM_LDFLAGS="$AM_LDFLAGS -Wl,-export-dynamic"
],[AC_MSG_RESULT(no)
])

LDFLAGS="$OLDLDFLAGS"

CFLAGS="$OLDCFLAGS"
LDFLAGS="$OLDLDFLAGS -Xlinker --allow-multiple-definition"

AC_MSG_CHECKING([for -Xlinker --allow-multiple-definition support])
AC_RUN_IFELSE([AC_LANG_PROGRAM([[#include <stdlib.h>]], [[exit(0);]])],
[AC_MSG_RESULT(yes) 
AM_LDFLAGS="$AM_LDFLAGS -Xlinker --allow-multiple-definition"
],[AC_MSG_RESULT(no)
])

LDFLAGS="$OLDLDFLAGS"



# Checks for header files.
AC_FUNC_ALLOCA
m4_warn([obsolete],
[The preprocessor macro `STDC_HEADERS' is obsolete.
  Except in unusual embedded environments, you can safely include all
  ISO C90 headers unconditionally.])dnl
# Autoupdate added the next two lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP

AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([inttypes.h libintl.h malloc.h stddef.h stdlib.h string.h sys/ioctl.h sys/time.h termios.h unistd.h limits.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_STDBOOL
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
m4_warn([obsolete],
[Update your code to rely only on HAVE_SYS_TIME_H,
then remove this warning and the obsolete code below it.
All current systems provide time.h; it need not be checked for.
Not all systems provide sys/time.h, but those that do, all allow
you to include it and time.h simultaneously.])dnl
AC_CHECK_HEADERS_ONCE([sys/time.h])
# Obsolete code to be removed.
if test $ac_cv_header_sys_time_h = yes; then
  AC_DEFINE([TIME_WITH_SYS_TIME],[1],[Define to 1 if you can safely include both <sys/time.h>
	     and <time.h>.  This macro is obsolete.])
fi
# End of obsolete code.

AC_TYPE_INT16_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
m4_warn([obsolete],
[Update your code to rely only on HAVE_SYS_TIME_H,
then remove this warning and the obsolete code below it.
All current systems provide time.h; it need not be checked for.
Not all systems provide sys/time.h, but those that do, all allow
you to include it and time.h simultaneously.])dnl
AC_CHECK_HEADERS_ONCE([sys/time.h])
# Obsolete code to be removed.
if test $ac_cv_header_sys_time_h = yes; then
  AC_DEFINE([TIME_WITH_SYS_TIME],[1],[Define to 1 if you can safely include both <sys/time.h>
	     and <time.h>.  This macro is obsolete.])
fi
# End of obsolete code.

AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
m4_warn([obsolete],
[your code may safely assume C89 semantics that RETSIGTYPE is void.
Remove this warning and the `AC_CACHE_CHECK' when you adjust the code.])dnl
AC_CACHE_CHECK([return type of signal handlers],[ac_cv_type_signal],[AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM([#include <sys/types.h>
#include <signal.h>
],
		 [return *(signal (0, 0)) (0) == 1;])],
		   [ac_cv_type_signal=int],
		   [ac_cv_type_signal=void])])
AC_DEFINE_UNQUOTED([RETSIGTYPE],[$ac_cv_type_signal],[Define as the return type of signal handlers
		    (`int' or `void').])

AC_FUNC_VPRINTF
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gettimeofday memmove memset strchr strerror strrchr strtol backtrace])

AC_SUBST(AM_CPPFLAGS)
AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_LDFLAGS)
AC_CONFIG_FILES([Makefile doc/Makefile tests/Makefile])
AC_OUTPUT
