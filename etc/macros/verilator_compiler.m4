dnl
dnl check for Verilator Verilog Compiler
dnl
AC_DEFUN([AC_CHECK_VERILATOR],[
AC_CHECK_PROG(panda_USE_VERILATOR, verilator, yes,AC_MSG_ERROR(erilator Verilog Compiler NOT correctly configured! Please install it))
AC_PROVIDE([$0])dnl
])

