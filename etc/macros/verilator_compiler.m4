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
VERILATOR_VERSION=$(verilator --version | head -n1 | awk -F' ' '{print $[2]}'| awk -F'.' '{print $[1]}')
AS_VERSION_COMPARE($VERILATOR_VERSION, [4], verilator_4=no, verilator_4=yes, verilator_4=yes)
if test "$verilator_4" = "yes"; then
AC_DEFINE(HAVE_THREADS, 1, "Define if verilator has the --threads option")
echo "checking verilator version>= 4.0... yes."
else
AC_DEFINE(HAVE_THREADS, 0, "Define if verilator has the --threads option")
echo "checking verilator version>= 4.0... no."
fi
AC_PROVIDE([$0])dnl
])

