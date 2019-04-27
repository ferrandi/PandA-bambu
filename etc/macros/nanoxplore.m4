dnl
dnl check where the NANOXPLORE synthesis tools are
dnl
AC_DEFUN([AC_CHECK_NANOXPLORE],[
    AC_ARG_WITH(nanoxplore-dir,
    [  --with-nanoxplore-dir=DIR  set where the root of NANOXPLORE tools are installed ],
    [
       ac_nanoxplore_dir="$withval"
    ])
    AC_ARG_WITH(nanoxplore-license,
    [  --with-nanoxplore-license=STRING  set the LM_LICENSE_FILE value ],
    [
       ac_nanoxplore_license="$withval"
    ])
    AC_ARG_WITH(nanoxplore-bypass,
    [  --with-nanoxplore-bypass=STRING  set the NANOXPLORE_BYPASS value ],
    [
       ac_nanoxplore_bypass="$withval"
    ])

if test "x$ac_nanoxplore_dir" = x; then
   ac_nanoxplore_dirs="/opt/NanoXplore/NXmap/*";
else
   ac_nanoxplore_dirs=$ac_nanoxplore_dir;
fi

if test "x$ac_nanoxplore_license" = x; then
   echo "NANOXPLORE license not set. User has to define LM_LICENSE_FILE variable before run a synthesis."
fi
AC_DEFINE_UNQUOTED(NANOXPLORE_LICENSE, "${ac_nanoxplore_license}", "Define the nanoxplore LICENSE VALUE")

if test "x$ac_nanoxplore_bypass" = x; then
   echo "NANOXPLORE bypass not set. User has to define NANOXPLORE_BYPASS variable before run a synthesis."
fi
AC_DEFINE_UNQUOTED(NANOXPLORE_BYPASS, "${ac_nanoxplore_bypass}", "Define the nanoxplore BYPASS VALUE")

for dirs in $ac_nanoxplore_dirs; do
   for dir in $dirs; do
      if test -n "`ls -1 $dir/bin/nxpython 2> /dev/null`"; then
         NANOXPLORE_SETTINGS="export PATH=$PATH:$dir/bin/"
         echo "checking if NANOXPLORE bin is present in $dir... yes"
      else
         echo "checking if NANOXPLORE bin is present in $dir... no"
      fi
   done
done

if test "x$NANOXPLORE_SETTINGS" != "x"; then
   AC_DEFINE_UNQUOTED(NANOXPLORE_SETTINGS, "${NANOXPLORE_SETTINGS}", "Define the altera settings script")
else
   AC_MSG_ERROR(NanoXplore not properly configured)
fi

AC_PROVIDE([$0])dnl
])



