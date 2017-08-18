dnl
dnl check where the XILINX synthesis tools are
dnl
AC_DEFUN([AC_CHECK_XILINX],[
    AC_ARG_WITH(xilinx-root-dir,
    [  --with-xilinx-root-dir=DIR   set where the root of XILINX tools are installed],
    [
       ac_xilinx_root_dir="$withval"
    ])

if test "x$ac_xilinx_root_dir" = x; then
   ac_xilinx_root_dir="/opt/Xilinx";
fi

ac_xilinx_dirs="$ac_xilinx_root_dir $ac_xilinx_root_dir/* $ac_xilinx_root_dir/*/ISE*";
ac_xilinx_vivado_dirs="$ac_xilinx_root_dir $ac_xilinx_root_dir/* $ac_xilinx_root_dir/Vivado/*";

dnl check for the XILINX configuration setup script: settings32.sh/settings64.sh
if test "$target_cpu" = x86_64; then
   for dirs in $ac_xilinx_dirs; do
      for dir in $dirs; do
         if test -n "`ls -1 $dir/ISE 2> /dev/null`" && test -n "`ls -1 $dir/settings64.sh 2> /dev/null`"; then
            ac_xilinx_settings="$dir/settings64.sh";
            XILINX_SETTINGS=$ac_xilinx_settings
            ac_xilinx_dir=$dir
            echo "checking if XILINX settings64.sh is present in $dir... yes $XILINX_SETTINGS"
         else
            echo "checking if XILINX settings64.sh is present in $dir... no"
            for dirs in $ac_xilinx_dirs; do
               for dir in $dirs; do
                  if test -n "`ls -1 $dir/ISE 2> /dev/null`" && test -n "`ls -1 $dir/settings32.sh 2> /dev/null`"; then
                    ac_xilinx_settings="$dir/settings32.sh";
                    XILINX_SETTINGS=$ac_xilinx_settings
                    ac_xilinx_dir=$dir
                    echo "checking if XILINX settings32.sh is present in $dir... yes $XILINX_SETTINGS"
                  else
                    echo "checking if XILINX settings32.sh is present in $dir... no"
                  fi
               done
            done         
         fi
      done
   done
   for dirs in $ac_xilinx_vivado_dirs; do
      for dir in $dirs; do
         if test -n "`ls -1 $dir/ids_lite 2> /dev/null`" && test -n "`ls -1 $dir/settings64.sh 2> /dev/null`" ; then
            ac_xilinx_vivado_settings="$dir/settings64.sh";
            XILINX_VIVADO_SETTINGS=$ac_xilinx_vivado_settings
            ac_xilinx_vivado_dir=$dir
            echo "checking if XILINX VIVADO settings64.sh is present in $dir... yes $XILINX_VIVADO_SETTINGS"
         else
            echo "checking if XILINX VIVADO settings64.sh is present in $dir... no"
            for dirs in $ac_xilinx_vivado_dirs; do
               for dir in $dirs; do
                  if test -n "`ls -1 $dir/ids_lite 2> /dev/null`" && test -n "`ls -1 $dir/settings32.sh 2> /dev/null`"; then
                    ac_xilinx_vivado_settings="$dir/settings32.sh";
                    XILINX_VIVADO_SETTINGS=$ac_xilinx_vivado_settings
                    ac_xilinx_vivado_dir=$dir
                    echo "checking if XILINX VIVADO settings32.sh is present in $dir... yes $XILINX_VIVADO_SETTINGS"
                  else
                    echo "checking if XILINX VIVADO settings32.sh is present in $dir... no"
                  fi
               done
            done         
         fi
      done
   done
else
   for dirs in $ac_xilinx_dirs; do
      for dir in $dirs; do
         if test -n "`ls -1 $dir/settings32.sh 2> /dev/null`"; then
            ac_xilinx_settings="$dir/settings32.sh";
            XILINX_SETTINGS=$ac_xilinx_settings
            ac_xilinx_dir=$dir
            echo "checking if XILINX settings32.sh is present in $dir... yes $XILINX_SETTINGS"
         else
            echo "checking if XILINX settings32.sh is present in $dir... no"
         fi
      done
   done
   for dirs in $ac_xilinx_vivado_dirs; do
      for dir in $dirs; do
         if test -n "`ls -1 $dir/settings32.sh 2> /dev/null`"; then
            ac_xilinx_vivado_settings="$dir/settings32.sh";
            XILINX_VIVADO_SETTINGS=$ac_xilinx_vivado_settings
            ac_xilinx_vivado_dir=$dir
            echo "checking if XILINX VIVADO settings32.sh is present in $dir... yes $XILINX_VIVADO_SETTINGS"
         else
            echo "checking if XILINX VIVADO settings32.sh is present in $dir... no"
         fi
      done
   done
fi

if test "x$ac_xilinx_settings" != "x"; then
   AC_DEFINE_UNQUOTED(XILINX_SETTINGS, "${XILINX_SETTINGS}", "Define the xilinx settings script")
   AC_DEFINE(HAVE_XILINX_ISE, 1, "Define if Xilinx ISE is available")
fi

if test "x$ac_xilinx_vivado_settings" != "x"; then
   AC_DEFINE_UNQUOTED(XILINX_VIVADO_SETTINGS, "${XILINX_VIVADO_SETTINGS}", "Define the xilinx vivado settings script")
   AC_DEFINE(HAVE_XILINX_VIVADO, 1, "Define if Xilinx Vivado is available")
elif test "x$ac_xilinx_settings" == "x"; then
   AC_MSG_ERROR(XILINX settings script required)
fi

dnl check for the XILINX glbl verilog package
for dirs in $ac_xilinx_dirs $ac_xilinx_vivado_dirs; do
   for dir in $dirs; do
      if test -n "`ls -1 $dir/ISE/verilog/src/glbl.v 2> /dev/null`"; then
         ac_xilinx_glbl="$dir/ISE/verilog/src/glbl.v";
         XILINX_GLBL=$ac_xilinx_glbl
         echo "checking if XILINX glbl.v is present in $dir/ISE/verilog/src ... yes $XILINX_GLBL"
      elif test -n "`ls -1 $dir/data/verilog/src/glbl.v 2> /dev/null`"; then
         ac_xilinx_glbl="$dir/data/verilog/src/glbl.v";
         XILINX_GLBL=$ac_xilinx_glbl
         echo "checking if XILINX glbl.v is present in $dir/ISE/verilog/src ... yes $XILINX_GLBL"
      else
         echo "checking if XILINX glbl.v is present in $dir/ISE/verilog/src ... no"
      fi
   done
done

if test "x$ac_xilinx_glbl" != "x"; then
   AC_DEFINE_UNQUOTED(XILINX_GLBL, "${XILINX_GLBL}", "Define the XILINX GLBL verilog package")
else
   AC_MSG_ERROR(XILINX GLBL verilog file is required)
fi

AC_PROVIDE([$0])dnl
])



