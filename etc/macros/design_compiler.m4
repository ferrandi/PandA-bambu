dnl
dnl check for Synopsys Design Compiler
dnl
AC_DEFUN([AC_CHECK_DC],[

dnl 
dnl Check for Synopsys Design Compiler
dnl
dc_executable=`which dc_shell`
if test -n "$dc_executable"; then
  echo "checking Synopsys Design Compiler... yes: $dc_executable"
fi
if test "x$dc_executable" = x; then
   panda_USE_DESIGN_COMPILER=no;
   AC_MSG_ERROR(Synopsys Design Compiler NOT correctly configured! Please check the server name and the path)
else
   panda_USE_DESIGN_COMPILER=yes;
fi

AC_PROVIDE([$0])dnl
])

