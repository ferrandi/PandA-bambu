dnl
dnl check for Icarus Verilog Compiler
dnl
AC_DEFUN([AC_CHECK_ICARUS],[

dnl 
dnl Check for Icarus Verilog Compiler
dnl
icarus_executable=`which iverilog`
if test -n "$icarus_executable"; then
  echo "checking Icarus Verilog Compiler... yes: $icarus_executable"
fi
if test "x$icarus_executable" = x; then
   panda_USE_ICARUS=no;
   AC_MSG_ERROR(Icarus Verilog Compiler NOT correctly configured! Please install it)
else
   panda_USE_ICARUS=yes;
fi

AC_PROVIDE([$0])dnl
])

