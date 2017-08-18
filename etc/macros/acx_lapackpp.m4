dnl Written by Christian Stimming <stimming@tuhh.de>
dnl
dnl AQ_WINDOZE must have been called before
dnl

AC_DEFUN([ACX_SEARCH_FOR_PATH],[
dnl searches for a file in a path
dnl $1 = file to search
dnl $2 = paths to search in
dnl $3 = return variable,  the directory where the file is found
$3="NO"
filename=$1
pathlist="$2"
for li in $pathlist; do
    if test -r "$li/$filename"; then
        $3="$li"
        break
    fi
done
]) dnl ACX_SEARCH_FOR_PATH
dnl
dnl
dnl
AC_DEFUN([ACX_LAPACKPP], [
AC_REQUIRE([ACX_LAPACK])
acx_lapackpp_ok=no

# We cannot use LAPACKPP if BLAS is not found
if test "x$acx_blas_ok" != xyes; then
        acx_lapackpp_ok=noblas
fi

# Now check for includes
AC_MSG_CHECKING(for Lapack++ includes)
AC_ARG_WITH(lapackpp-prefix, [  --with-lapackpp-prefix=DIR Prefix where Lapack++ is installed],
  [lapack_search_dirs="$withval"],
  [lapack_search_dirs="$with_simthetic_prefix \
 $prefix \
 /usr \
 /usr/local \
 /usr/local/lapack \
 /usr/local/lapackpp \
 $HOME/lapackpp \
 $HOME/usr \
 $HOME/usr/lapack "])

dnl search for lapack headers
ACX_SEARCH_FOR_PATH([include/lapackpp/lapackpp.h],[$lapack_search_dirs], found_dir)
if test "$found_dir" = "NO"; then
  ACX_SEARCH_FOR_PATH([include/lapackpp.h],[$lapack_search_dirs], found_dir)
  if test "$found_dir" = "NO"; then
    acx_lapackpp_ok=no
    AC_MSG_RESULT("not found")
  else
    LAPACKPP_INCLUDES="-I$found_dir/include"
    AC_MSG_RESULT($found_dir/include)
  fi
else
  LAPACKPP_INCLUDES="-I$found_dir/include/lapackpp -I$found_dir/include"
  AC_MSG_RESULT($found_dir/include/lapackpp)
fi


dnl search for lapack libs
AC_MSG_CHECKING(for Lapack++ libraries)

case "$target" in
    *-mingw32*)
	ACX_SEARCH_FOR_PATH([lib/lapackpp32.dll],[$lapack_search_dirs], found_dir)
        if test "$found_dir" = "NO"; then
	    ACX_SEARCH_FOR_PATH([lapackpp32.dll],
              [$WIN_PATH_WINDOWS_MINGW $WIN_PATH_SYSTEM_MINGW], found_dir)
	else
	    found_dir="$found_dir/lib"
        fi
        ac_lapackpp_libs="-llapackpp32"
        ;;
    *)
	ACX_SEARCH_FOR_PATH([lib/liblapackpp.so],[$lapack_search_dirs], found_dir)
        if test "$found_dir" != "NO"; then
	    found_dir="$found_dir/lib"
            ac_lapackpp_libs="-llapackpp"
	else
	    ACX_SEARCH_FOR_PATH([lib/liblapack++.so],[$lapack_search_dirs], found_dir)
            if test "$found_dir" != "NO"; then
		found_dir="$found_dir/lib"
        	ac_lapackpp_libs="-llapack++ -llamatrix++ -lblas++ $LAPACK_LIBS $BLAS_LIBS $FLIBS"
	    fi
        fi
        ;;
esac

if test "$found_dir" = "NO"; then
  acx_lapackpp_ok=no
  AC_MSG_RESULT("not found")
else
  LAPACKPP_LDFLAGS="-L$found_dir"
  LAPACKPP_LIBS="$ac_lapackpp_libs"
  AC_MSG_RESULT($found_dir)
  acx_lapackpp_ok=yes
fi

AS_SCRUB_INCLUDE(LAPACKPP_INCLUDES)
AC_SUBST(LAPACKPP_INCLUDES)
AC_SUBST(LAPACKPP_LIBS)
AC_SUBST(LAPACKPP_LDFLAGS)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_lapackpp_ok" = xyes; then
        ifelse([$1],,AC_DEFINE(HAVE_LAPACKPP,1,[Define if you have LAPACKPP library.]),[$1])
        :
else
        acx_lapackpp_ok=no
        $2
fi
])dnl ACX_LAPACKPP
