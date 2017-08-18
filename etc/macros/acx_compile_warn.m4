# Compile warning arguments to ./configure
# by Christian Stimming <stimming@tuhh.de> 2003-11-19

dnl ACX_COMPILE_WARN()
dnl Add arguments for compile warnings and debug options to ./configure.
dnl
AC_DEFUN([ACX_COMPILE_WARN],
[
dnl Add compile arguments for debugging and warnings. Macro argument
dnl $1 is the default argument if --enable-debug is not specified.

dnl If no Macro argument is given, enable the debugging code.
if test -z "$1"; then 
  default_debug_arg="-g"; else 
  default_debug_arg="$1"; 
fi

AC_MSG_CHECKING([for compiler arguments])

dnl For enabling of debugging flags/code
AC_ARG_ENABLE(debug,
  [  --enable-debug          enable compile arguments for debugging code],
  [case "${enableval}" in
     yes)   CXXFLAGS="${CXXFLAGS} -g"
	    # Remove -O2
	    CXXFLAGS=`echo "${CXXFLAGS}" | sed -e 's/-O2//'`
	    CFLAGS="${CFLAGS} -g"
	    LDFLAGS="${LDFLAGS} -g"
	    AC_DEFINE(DEBUG,1,[Define if you want debugging code enabled.]) ;;
     no) ;;
     *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
   esac
  ], [
	# Default value if the argument was not given
	CXXFLAGS="${CXXFLAGS} ${default_debug_arg}"
	CFLAGS="${CFLAGS} ${default_debug_arg}"
	LDFLAGS="${LDFLAGS} ${default_debug_arg}"
])

dnl If this is gcc, then ...
if test ${GCC}x = yesx; then

  dnl Enable all warnings
  AC_ARG_ENABLE(warnings,
    [  --enable-warnings       enable compilation warnings, default=yes],
    [case "${enableval}" in
       yes) CXXFLAGS="${CXXFLAGS} -Wall -pedantic -ansi"
	    CFLAGS="${CFLAGS} -Wall -pedantic" ;;
       all) CXXFLAGS="${CXXFLAGS} -Wall -pedantic -ansi"
	    CFLAGS="${CFLAGS} -Wall -pedantic -ansi" ;;
       no) ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-warnings) ;;
     esac
  ], [ 
     # Default value if the argument was not given
     CXXFLAGS="${CXXFLAGS} -Wall" 
     CFLAGS="${CFLAGS} -Wall" 
  ])

  dnl For gcc >= 3.4.x, specifically enable the new warning switch
  dnl -Wdeclaration-after-statement in order to preserve source code
  dnl compatibility to gcc 2.95 and other compilers.
  GCC_VERSION=`${CC} -dumpversion`
  if test `echo ${GCC_VERSION} | cut -d. -f1` -ge 3; then
     # This is gcc >= 3.x.x
     if test `echo ${GCC_VERSION} | cut -d. -f2` -ge 4; then
	# This is gcc >= 3.4.x
	CFLAGS="${CFLAGS} -Wdeclaration-after-statement"
     fi
  fi

  dnl For enabling error on warnings
  AC_ARG_ENABLE(error-on-warning,
    [  --enable-error-on-warning treat all compile warnings as errors, default=no],
    [case "${enableval}" in
       yes) CXXFLAGS="${CXXFLAGS} -Werror" 
	    CFLAGS="${CFLAGS} -Werror" ;;
       no) ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-error-on-warning) ;;
     esac
  ], [ 
     # Default value if the argument was not given
     CXXFLAGS="${CXXFLAGS}" 
     CFLAGS="${CFLAGS}" 
  ])
fi

# Beautify the CXXFLAGS: remove extra spaces, remove double -g
CXXFLAGS=`echo "${CXXFLAGS}" | sed -e 's/   */ /g' | sed -e 's/-g -g/-g/'`

# Print the result
AC_MSG_RESULT($CXXFLAGS)

])
