dnl
dnl check where rapidminer is
dnl
AC_DEFUN([AC_CHECK_RAPIDMINER],[
    AC_ARG_WITH(Rapidminer-dir,
    [  --with-rapidminer-dir=DIR  where the root of Rapidminer is installed ],
    [
       ac_rapidminer_dir="$withval"
    ])

AC_PROG_JAVA

if test "x$ac_rapidminer_dir" = x; then
   for dir in /opt/rapidminer/; do
      AC_CHECK_FILE("$dir/lib/rapidminer.jar", ac_rapidminer_jar=$dir/lib/rapidminer.jar,)
   done
fi

if test "x$ac_rapidminer_jar" = x; then
   echo "checking if rapidminer is present... no"
   AC_MSG_ERROR([rapidminer not found])
else
   echo "checking if rapidminer is present... yes $ac_rapidminer_jar"
fi
AC_DEFINE_UNQUOTED(RAPIDMINER_JAR, "${ac_rapidminer_jar}", "Define which is the rapidminer jar")

AC_PROVIDE([$0])
])
