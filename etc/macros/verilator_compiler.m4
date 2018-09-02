dnl
dnl check for Verilator Verilog Compiler
dnl
AC_DEFUN([AC_CHECK_VERILATOR],[
AC_CHECK_PROG(panda_USE_VERILATOR, verilator, yes,AC_MSG_ERROR(Verilog Compiler NOT correctly configured! Please install it))
has_support_l2_name=$(verilator --l2-name v 2>&1 | head -n1 | grep 'Invalid Option')
if test "$has_support_l2_name"x == x; then
AC_DEFINE(HAVE_L2_NAME, 1, "Define if verilator has the --l2-name option")
else
AC_DEFINE(HAVE_L2_NAME, 0, "Define if verilator has the --l2-name option")
fi
AC_PROVIDE([$0])dnl
])

