# $Id: atlas.m4,v 1.2 2004/04/29 08:36:16 cstim Exp $
# (c) 2004 Martin Preuss<martin@libchipcard.de>
# This function checks if ATLAS is wanted and locates it

AC_DEFUN([AQ_CHECK_ATLAS],[
dnl PREREQUISITES:
dnl   nothing
dnl IN: 
dnl   nothing
dnl OUT:
dnl   Variables:
dnl     atlas_libraries: Path to the ATLAS libraries (subst)
dnl     atlas_lib: ATLAS libraries to link against (subst)
dnl     atlas_available: "yes" if ATLAS is available
dnl   Defines:

dnl check if atlas is desired
AC_MSG_CHECKING(if ATLAS should be used)
AC_ARG_ENABLE(atlas,
  [  --enable-atlas             enable ATLAS (default=yes)],
  enable_atlas="$enableval",
  enable_atlas="yes")
AC_MSG_RESULT($enable_atlas)

atlas_bad=no

if test "$enable_atlas" != "no"; then

AC_ARG_WITH(atlas-libs, [  --with-atlas-libs=DIR  adds ATLAS library path],
  [atlas_search_lib_dirs="$withval"],
  [atlas_search_lib_dirs="/usr/lib \
		       /usr/local/lib \
		       /usr/lib/atlas/lib \
		       /usr/local/atlas/lib \
		       /lib"])


dnl ******* atlas lib ***********
AC_MSG_CHECKING(for ATLAS library)
atlas_search_lib_names="libatlas.so \
                        libatlas.so.* \
                        libatlas.a"

dnl search for atlas libs
for d in $atlas_search_lib_dirs; do
  AQ_SEARCH_FILES("$d",$atlas_search_lib_names)
  if test -n "$found_file" ; then
     case "$found_file" in 
       *a)
         atlas_lib="$d/$found_file"
         ;;
       *)
         atlas_libraries="-L$d"
         atlas_lib="-l`echo $found_file | sed 's/lib//;s/\.so*//;s/\.a//'`"
         ;;
       esac
     AC_MSG_RESULT($d ($found_file))
     break
  fi
done
if test -z "atlas_lib"; then
  atlas_bad=yes
  AC_MSG_WARN(not found)
fi

dnl *******  lib ***********
AC_MSG_CHECKING(for f77blas library)
atlas_search_lib_names="libf77blas.so \
                        libf77blas.so.* \
                        libf77blas.a"

dnl search for atlas libs
AQ_SEARCH_FILES("$d",$atlas_search_lib_names)
if test -n "$found_file" ; then
   case "$found_file" in 
     *a)
       atlas_lib="$d/$found_file $atlas_lib "
       ;;
     *)
       atlas_lib="-l`echo $found_file | sed 's/lib//;s/\.so*//;s/\.a//'` $atlas_lib"
       ;;
   esac
   AC_MSG_RESULT($d ($found_file))
else
  atlas_bad=yes
  AC_MSG_WARN(not found)
fi


dnl *******  lib ***********
AC_MSG_CHECKING(for cblas library)
atlas_search_lib_names="libcblas.so \
                        libcblas.so.* \
                        libcblas.a"

dnl search for atlas libs
AQ_SEARCH_FILES("$d",$atlas_search_lib_names)
if test -n "$found_file" ; then
   case "$found_file" in 
     *a)
       atlas_lib="$d/$found_file $atlas_lib"
       ;;
     *)
       atlas_lib="-l`echo $found_file | sed 's/lib//;s/\.so*//;s/\.a//'` $atlas_lib"
       ;;
   esac
   AC_MSG_RESULT($d ($found_file))
else
  atlas_bad=yes
  AC_MSG_WARN(not found)
fi



AC_MSG_CHECKING(whether ATLAS is usable)
if test -z "$atlas_lib" -o "$atlas_bad" = "yes"; then
    atlas_available="no"
    AC_MSG_WARN(no)
else
    atlas_available="yes"
    AC_MSG_RESULT(yes)
fi


# end of "if enable-atlas"
fi
AC_SUBST(atlas_libraries)
AC_SUBST(atlas_lib)
])

