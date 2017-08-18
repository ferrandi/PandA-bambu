dnl
dnl check where the x86 treegcc is
dnl
AC_DEFUN([AC_CHECK_TREEGCC],[
    AC_ARG_WITH(treegcc-dir,
    [  --with-treegcc-dir=DIR  where the root of TREEGCC is installed ],
    [
       ac_treegcc_dir="$withval"
    ])
    AC_ARG_WITH(treegcc-cflags,
    [  --with-treegcc-cflags=flags  options passed to the TREEGCC compiler ],
    [
       ac_treegcc_cflags="$withval"
    ])

dnl check for the x86 treegcc compiler
if test "x$ac_treegcc_dir" = x; then
   ac_treegcc_dirs="/opt/treegcc/gcc4.3.0 /opt/treegcc/treegcc-4.3.0* /mingw/opt/gcc4.3.0 $HOME/gcc4.3.0 /opt/treegcc/treegcc-4.3.4a /opt/gcc-4.5.0";
else
   ac_treegcc_dirs=$ac_treegcc_dir;
fi

gcc_4_5_0_suffixes="-4.5.0_O0 -4.5"

for dirs in $ac_treegcc_dirs; do
   for dir in $dirs; do
      if test -n "`ls -1 $dir/bin/treegcc$ac_exeext 2> /dev/null`"; then
         ac_treegcc="$dir/bin/treegcc$ac_exeext";
         TREEGCC=$ac_treegcc
         ac_treegcc_dir=$dir
         echo "checking if TREEGCC is present in $dir... yes $TREEGCC"
      else
         for suff in $gcc_4_5_0_suffixes; do
            if test -n "`ls -1 $dir/bin/gcc$suff$ac_exeext 2> /dev/null`"; then
               ac_treegcc="$dir/bin/gcc$suff$ac_exeext";
               TREEGCC=$ac_treegcc
               ac_treegcc_dir=$dir
               echo "checking if TREEGCC is present in $dir/bin/gcc$suff... yes $TREEGCC"
               echo "It is also a GCC 4.5"
               ac_gcc_45_plugin="$dir/plugin/plugin_dumpGimpleSSA.so"
               if test -f $dir/plugin/plugin_dumpGimpleEmpty.so; then
                 echo "Found dump gimple empty plugin"
                 ac_gcc_45_empty_plugin="$dir/plugin/plugin_dumpGimpleEmpty.so"
               fi
            else
               echo "checking if TREEGCC is present in $dir-$suff... no"
            fi
         done
      fi
      if test -n "`ls -1 $dir/bin/treeg++$ac_exeext 2> /dev/null`"; then
         ac_treegpp="$dir/bin/treeg++$ac_exeext";
         TREEGPP=$ac_treegpp
         echo "checking if TREEGPP is present in $dir... yes $TREEGPP"
      else
         for suff in $gcc_4_5_0_suffixes; do
            if test -n "`ls -1 $dir/bin/g++$suff$ac_exeext 2> /dev/null`"; then
               ac_treegpp="$dir/bin/g++$suff$ac_exeext";
               TREEGPP=$ac_treegpp
               echo "checking if TREEGPP is present in $dir-$suff... yes $TREEGPP"
            else
               echo "checking if TREEGPP is present in $dir-$suff... no"
            fi
         done
      fi
      if test -n "`ls -1 $dir/bin/treecpp$ac_exeext 2> /dev/null`"; then
         ac_treecpp="$dir/bin/treecpp$ac_exeext";
         TREECPP=$ac_treecpp
         echo "checking if TREECPP is present in $dir... yes $TREECPP"
      else
         for suff in $gcc_4_5_0_suffixes; do
            if test -n "`ls -1 $dir/bin/cpp$suff$ac_exeext 2> /dev/null`"; then
               ac_treecpp="$dir/bin/cpp$suff$ac_exeext";
               TREECPP=$ac_treecpp
               echo "checking if TREECPP is present in $dir-$suff... yes $TREECPP"
            else
               echo "checking if TREECPP is present in $dir-$suff... no"
            fi
         done
      fi
   done
done



if test "x$ac_treegcc" != "x"; then
   AC_DEFINE_UNQUOTED(TREEGCC, "${TREEGCC}", "Define the treegcc compiler")
fi
if test "x$ac_treegpp" != "x"; then
   AC_DEFINE_UNQUOTED(TREEGPP, "${TREEGPP}", "Define the treeg++ compiler")
fi
if test "x$ac_treecpp" != "x"; then
   AC_DEFINE_UNQUOTED(TREECPP, "${TREECPP}", "Define the treecpp compiler")
fi
if test "x$ac_treegcc_cflags" != x; then
   AC_DEFINE_UNQUOTED(TREEGCC_CFLGAGS, "${ac_treegcc_cflags}", "Define the options passed to the treegcc compiler")
fi

dnl check for gcc 4.5.x PandA plugin
if test "x$ac_gcc_45_plugin" != x; then
   AC_DEFINE_UNQUOTED(GCC_45_PLUGIN, "${ac_gcc_45_plugin}", "Define the filename of the GCC PandA plugin")
   AC_DEFINE_UNQUOTED(GCC_45_SSA_PLUGIN, "${ac_gcc_45_plugin}", "Define the filename of the GCC PandA SSA plugin")
AC_DEFINE(HAVE_GCC4_5,1,[define if the GCC 4.5.x.a is available])
fi
if test "x$ac_gcc_45_empty_plugin" != x; then
   AC_DEFINE_UNQUOTED(GCC_45_EMPTY_PLUGIN, "${ac_gcc_45_empty_plugin}", "Define the filename of the GCC PandA Empty plugin")
fi

dnl check for gcc 4.3.4
if test "x$ac_treegcc_dir" = x"/opt/treegcc/treegcc-4.3.4a" || test "x$ac_treegcc_dir" = x"/opt/crosstools/treegcc-4.3.4a-glibc-2.3.6-1/arm-unknown-linux-gnu"; then
   AC_DEFINE(HAVE_GCC4_3_4a,1,[define if the GCC 4.3.4.a is available])
fi


AC_PROVIDE([$0])dnl
])



dnl
dnl check where the arm treegcc is
dnl
AC_DEFUN([AC_CHECK_ARM_TREEGCC],[
    AC_ARG_WITH(ARM_treegcc-dir,
    [  --with-arm-treegcc-dir=DIR  where the root of ARM_TREEGCC is installed ],
    [
       ac_arm_treegcc_dir="$withval"
    ])
    AC_ARG_WITH(ARM_treegcc-cflags,
    [  --with-ARM_treegcc-cflags=flags  options passed to the ARM_TREEGCC compiler ],
    [
       ac_arm_treegcc_cflags="$withval"
    ])

dnl check for the ARM_treegcc cross compiler
if test "x$ac_ARM_treegcc_dir" = x; then
   ac_ARM_treegcc_dirs="/mingw/opt/crosstools/treegcc*-glibc-2.3.*/arm-unknown-linux-gnu /opt/crosstools/treegcc*-glibc-2.3.*/arm-unknown-linux-gnu";
else
   ac_ARM_treegcc_dirs=$ac_ARM_treegcc_dir;
fi
for dirs in $ac_ARM_treegcc_dirs; do
   for dir in $dirs; do
      for binary in $dir/bin/*gcc$ac_exeext; do
         if test -n "`ls -1 $binary 2> /dev/null`"; then
            echo "checking if ARM_TREEGCC is present in $dir... yes: $binary"
            ac_ARM_treegcc_dir="$dir";
            ac_ARM_treegcc="$binary";
            ARM_TREEGCC="$ac_ARM_treegcc";
         else
            echo "checking if ARM_TREEGCC is present in $dir... no";
         fi
      done
      for binary in $dir/bin/*g++$ac_exeext; do
         if test -n "`ls -1 $binary 2> /dev/null`"; then
            ac_ARM_treegpp="$binary";
            ARM_TREEGPP=$ac_ARM_treegpp
            echo "checking if ARM_TREEGPP is present in $dir... yes: $binary"
         fi
      done
      for binary in $dir/bin/*cpp$ac_exeext; do
         if test -n "`ls -1 $binary 2> /dev/null`"; then
            ac_ARM_treecpp="$binary";
            ARM_TREECPP=$ac_ARM_treecpp
            echo "checking if ARM_TREECPP is present in $dir... yes: $binary"
         fi
      done
   done
done
if test "x$ac_ARM_treegcc" != "x"; then
   AC_DEFINE_UNQUOTED(ARM_TREEGCC, "${ARM_TREEGCC}", "Define the arm-treegcc compiler")
fi
if test "x$ac_ARM_treegpp" != "x"; then
   AC_DEFINE_UNQUOTED(ARM_TREEGPP, "${ARM_TREEGPP}", "Define the arm-treegpp compiler")
fi
if test "x$ac_ARM_treecpp" != "x"; then
   AC_DEFINE_UNQUOTED(ARM_TREECPP, "${ARM_TREECPP}", "Define the arm-treecpp compiler")
fi

if test "x$ac_ARM_treegcc_cflags" != x; then
   AC_DEFINE_UNQUOTED(ARM_TREEGCC_CFLGAGS, "${ac_ARM_treegcc_cflags}", "Define the options passed to the ARM_treegcc compiler")
fi

AC_PROVIDE([$0])dnl
])

dnl
dnl check where the sparc treegcc is
dnl
AC_DEFUN([AC_CHECK_SPARC_TREEGCC],[
    AC_ARG_WITH(SPARC_treegcc-dir,
    [  --with-sparc-treegcc-dir=DIR  where the root of SPARC_TREEGCC is installed ],
    [
       ac_sparc_treegcc_dir="$withval"
    ])
    AC_ARG_WITH(SPARC_treegcc-cflags,
    [  --with-SPARC_treegcc-cflags=flags  options passed to the SPARC_TREEGCC compiler ],
    [
       ac_sparc_treegcc_cflags="$withval"
    ])

dnl check for the SPARC_treegcc cross compiler
if test "x$ac_SPARC_treegcc_dir" = x; then
   ac_SPARC_treegcc_dirs="/mingw/opt/crosstools/treegcc*-glibc-2.3.*/sparc-unknown-linux-gnu /opt/crosstools/treegcc*-glibc-2.3.*/sparc-unknown-linux-gnu";
else
   ac_SPARC_treegcc_dirs=$ac_SPARC_treegcc_dir;
fi
for dirs in $ac_SPARC_treegcc_dirs; do
   for dir in $dirs; do
      for binary in $dir/bin/*gcc$ac_exeext; do
         if test -n "`ls -1 $binary 2> /dev/null`"; then
            echo "checking if SPARC_TREEGCC is present in $dir... yes: $binary"
            ac_SPARC_treegcc_dir="$dir";
            ac_SPARC_treegcc="$binary";
            SPARC_TREEGCC="$ac_SPARC_treegcc";
         else
            echo "checking if SPARC_TREEGCC is present in $dir... no";
         fi
      done
      for binary in $dir/bin/*g++$ac_exeext; do
         if test -n "`ls -1 $binary 2> /dev/null`"; then
            ac_SPARC_treegpp="$binary";
            SPARC_TREEGPP=$ac_SPARC_treegpp
            echo "checking if SPARC_TREEGPP is present in $dir... yes: $binary"
         fi
      done
      for binary in $dir/bin/*cpp$ac_exeext; do
         if test -n "`ls -1 $binary 2> /dev/null`"; then
            ac_SPARC_treecpp="$binary";
            SPARC_TREECPP=$ac_SPARC_treecpp
            echo "checking if SPARC_TREECPP is present in $dir... yes: $binary"
         fi
      done
   done
done

if test "x$ac_SPARC_treegcc" != "x"; then
   AC_DEFINE_UNQUOTED(SPARC_TREEGCC, "${SPARC_TREEGCC}", "Define the sparc-treegcc compiler")
fi
if test "x$ac_SPARC_treegpp" != "x"; then
   AC_DEFINE_UNQUOTED(SPARC_TREEGPP, "${SPARC_TREEGPP}", "Define the sparc-treegpp compiler")
fi
if test "x$ac_SPARC_treecpp" != "x"; then
   AC_DEFINE_UNQUOTED(SPARC_TREECPP, "${SPARC_TREECPP}", "Define the sparc-treecpp compiler")
fi

if test "x$ac_SPARC_treegcc_cflags" != x; then
   AC_DEFINE_UNQUOTED(SPARC_TREEGCC_CFLGAGS, "${ac_SPARC_treegcc_cflags}", "Define the options passed to the SPARC_treegcc compiler")
fi

AC_PROVIDE([$0])dnl
])


