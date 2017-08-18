dnl
dnl check if a3 is in the path
dnl
AC_DEFUN([AC_CHECK_A3],[

dnl check for a3
ac_a3=""
AC_CHECK_PROG(ac_a3, a3, YES, NO)
if test "x$ac_a3" = x; then
   echo "checking if a3 is present... no"
   AC_MSG_ERROR([a3 not found])
else
   echo "checking if a3 is present... yes"
fi

AC_PROVIDE([$0])
])
