dnl
dnl check where the ALTERA synthesis tools are
dnl
AC_DEFUN([AC_CHECK_ALTERA],[
    AC_ARG_WITH(altera-dir,
    [  --with-altera-dir=DIR   set where the root of ALTERA tools are installed ],
    [
       ac_altera_dir="$withval"
    ])

if test "x$ac_altera_dir" = x; then
   ac_altera_dirs="/opt/altera/* /opt/intelFPGA/*";
else
   ac_altera_dirs=$ac_altera_dir;
fi

for dirs in $ac_altera_dirs; do
      for dir in $dirs; do
         if test -n "`ls -1 $dir/quartus/bin/quartus_sh 2> /dev/null`"; then
            dnl check for quartus <= 13
            quartus_version=`$dir/quartus/bin/quartus_sh --version | grep Version | awk '{print $[2]}'`
            if test "x$quartus_version" != "x"; then
                temp_quartus_13_settings=""
                AS_VERSION_COMPARE(${quartus_version}, "14.0.0", temp_quartus_13_settings="export PATH=$PATH:$dir/quartus/bin/", ,)
                if test "x$temp_quartus_13_settings" != "x"; then
                   QUARTUS_13_SETTINGS=${temp_quartus_13_settings}
                   size_check=`$dir/quartus/bin/quartus_sh --help | grep "\-\-64bit"`
                   if test "x$size_check" != "x"; then
                      AC_DEFINE(HAVE_QUARTUS_13_64BIT, 1, "define if altera distribution has a working 64bit version")
                      echo "checking if ALTERA quartus 13 64-bit is present in $dir... yes"
                   else
                      AC_DEFINE(HAVE_QUARTUS_13_64BIT, 0, "define if altera distribution has a working 64bit version")
                      echo "checking if ALTERA quartus 13 32-bit is present in $dir... yes"
                   fi
                else
                  QUARTUS_SETTINGS="export PATH=$PATH:$dir/quartus/bin/"
                  echo "checking if ALTERA quartus_sh is present in $dir... yes"
                fi
            else
               QUARTUS_SETTINGS="export PATH=$PATH:$dir/quartus/bin/"
               echo "checking if ALTERA quartus_sh is present in $dir... yes"
            fi
         else
            echo "checking if ALTERA quartus_sh is present in $dir... no"
         fi
      done
   done

if test "x$QUARTUS_SETTINGS" != "x"; then
   AC_DEFINE_UNQUOTED(QUARTUS_SETTINGS, "${QUARTUS_SETTINGS}", "Define the altera settings script")
fi
if test "x$QUARTUS_13_SETTINGS" != "x"; then
   AC_DEFINE_UNQUOTED(QUARTUS_13_SETTINGS, "${QUARTUS_13_SETTINGS}", "Define the altera quartus 13 settings script")
fi
if test "x$QUARTUS_SETTINGS" == "x" && test "x$QUARTUS_13_SETTINGS" == "x"; then
   AC_MSG_ERROR(ALTERA settings script required)
fi

AC_PROVIDE([$0])dnl
])



