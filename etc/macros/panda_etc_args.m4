AC_DEFUN([AC_ETC_ARGS],[
AC_ARG_ENABLE(I386-gcc,            [  --enable-I386-gcc            use host compiler],      [panda_USE_I386_GCC="$enableval"])
AC_ARG_ENABLE(flopoco,             [  --enable-flopoco             compile FloPoCo external library],                     [panda_USE_FLOPOCO="$enableval"])
])

