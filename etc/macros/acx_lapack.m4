dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/acx_lapack.html
dnl
AC_DEFUN([ACX_LAPACK], [
AC_REQUIRE([ACX_BLAS])
acx_lapack_ok=no

AC_ARG_WITH(lapack,
        [--with-lapack=<lib>   Use LAPACK library <lib>],
	[case $withval in
          yes | "") ;;
          no) acx_lapack_ok=disable ;;
          -* | */* | *.a | *.so | *.so.* | *.o) LAPACK_LIBS="$withval" ;;
          *) LAPACK_LIBS="-l$withval" ;;
	esac])

# Get fortran linker name of LAPACK function to check for.
AC_F77_FUNC(cheev)

# We cannot use LAPACK if BLAS is not found
if test "x$acx_blas_ok" != xyes; then
        acx_lapack_ok=noblas
fi

# First, check LAPACK_LIBS environment variable
if test "x$LAPACK_LIBS" != x; then
        save_LIBS="$LIBS"; LIBS="$LAPACK_LIBS $BLAS_LIBS $LIBS $FLIBS"
        AC_MSG_CHECKING([for $cheev in $LAPACK_LIBS])
        AC_TRY_LINK_FUNC($cheev, [acx_lapack_ok=yes], [LAPACK_LIBS=""])
        AC_MSG_RESULT($acx_lapack_ok)
        LIBS="$save_LIBS"
        if test acx_lapack_ok = no; then
                LAPACK_LIBS=""
        fi
fi

# LAPACK linked to by default?  (is sometimes included in BLAS lib)
if test $acx_lapack_ok = no; then
        save_LIBS="$LIBS"; LIBS="$LIBS $BLAS_LIBS $FLIBS"
        AC_CHECK_FUNC($cheev, [acx_lapack_ok=yes])
        LIBS="$save_LIBS"
fi

# Generic LAPACK library?
for lapack in lapack lapack32 lapack_rs6k; do
        if test $acx_lapack_ok = no; then
                save_LIBS="$LIBS"; LIBS="$BLAS_LIBS $LIBS"
                AC_CHECK_LIB($lapack, $cheev,
                    [acx_lapack_ok=yes; LAPACK_LIBS="-l$lapack"], [], [$FLIBS])
                LIBS="$save_LIBS"
        fi
done


dnl ******* generic check ***********
dnl this check is needed in case of static libraries which configure
dnl fails to detect under WIN32
if test $acx_lapack_ok = no; then
  AC_MSG_CHECKING(for any lapack library)
  AC_ARG_WITH(lapack-libs, [  --with-lapack-libs=DIR  adds lapack library path],
    [lapack_search_lib_dirs="$withval"],
    [lapack_search_lib_dirs="/usr/lib \
                         /usr/local/lib \
                         /usr/lib/atlas/lib \
                         /usr/local/atlas/lib \
                         /lib"])
  
  
  lapack_search_lib_names="liblapack.so \
                           liblapack.so.* \
                           liblapack.a"
  
  dnl search for atlas libs
  for d in $lapack_search_lib_dirs; do
    AQ_SEARCH_FILES("$d",$lapack_search_lib_names)
    if test -n "$found_file" ; then
       acx_lapack_ok=yes
       case "$found_file" in 
         *a)
           acx_lapack_ok=yes; 
           LAPACK_LIBS="$d/$found_file"
           ;;
         *)
           lapack_libraries="-L$d"
           LAPACK_LIBS="-l`echo $found_file | sed 's/lib//;s/\.so*//;s/\.a//'`"
           ;;
         esac
       break
    fi
  done
  if test "$acx_lapack_ok" = "yes"; then
    AC_MSG_RESULT($LAPACK_LIBS)
  else
    AC_MSG_RESULT(none found)
  fi
fi


AC_SUBST(LAPACK_LIBS)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_lapack_ok" = xyes; then
        ifelse([$1],,AC_DEFINE(HAVE_LAPACK,1,[Define if you have LAPACK library.]),[$1])
        :
else
        acx_lapack_ok=no
        $2
fi
])dnl ACX_LAPACK
