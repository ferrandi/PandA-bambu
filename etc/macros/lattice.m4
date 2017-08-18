dnl
dnl check where the LATTICE synthesis tools are
dnl
AC_DEFUN([AC_CHECK_LATTICE],[
    AC_ARG_WITH(lattice-dir,
    [  --with-lattice-dir=DIR  set where the root of LATTICE tools are installed ],
    [
       ac_lattice_dir="$withval"
    ])

if test "x$ac_lattice_dir" = x; then
   ac_lattice_dirs="/opt/diamond/* /usr/local/diamond/*";
else
   ac_lattice_dirs=$ac_lattice_dir;
fi
enable_lattice_64bit=no;
for dirs in $ac_lattice_dirs; do
      for dir in $dirs; do
         if test -n "`ls -1 $dir/bin/lin/diamondc 2> /dev/null`"; then
            ac_lattice_settings="export TEMP=/tmp;export LSC_INI_PATH=\"\";export LSC_DIAMOND=true;export TCL_LIBRARY=$dir/tcltk/lib/tcl8.5;export FOUNDRY=$dir/ispfpga;export PATH=$FOUNDRY/bin/lin:$dir/bin/lin:$PATH";
            LATTICE_SETTINGS=$ac_lattice_settings
            ac_lattice_dir=$dir
            echo "checking if LATTICE diamondc [32bit] is present in $dir... yes"
         else
            echo "checking if LATTICE diamondc [32bit] is present in $dir... no"
         fi
         if test -n "`ls -1 $dir/bin/lin64/diamondc 2> /dev/null`"; then
            ac_lattice_settings="export TEMP=/tmp;export LSC_INI_PATH=\"\";export LSC_DIAMOND=true;export TCL_LIBRARY=$dir/tcltk/lib/tcl8.5;export FOUNDRY=$dir/ispfpga;export PATH=$FOUNDRY/bin/lin64:$dir/bin/lin64:$PATH";
            LATTICE_SETTINGS=$ac_lattice_settings
            ac_lattice_dir=$dir
            enable_lattice_64bit=yes
            echo "checking if LATTICE diamondc [64bit] is present in $dir... yes"
         else
            echo "checking if LATTICE diamondc [64bit] is present in $dir... no"
         fi
      done
   done

if test "x$ac_lattice_settings" != "x"; then
   AC_DEFINE_UNQUOTED(LATTICE_SETTINGS, "${LATTICE_SETTINGS}", "Define the lattice settings script")
else
   AC_MSG_ERROR(LATTICE settings script required)
fi

if test "x$enable_lattice_64bit" = xyes; then
  AC_DEFINE(HAVE_LATTICE_64bit, 1, "define if lattice distribution has a working 64bit version")
fi

dnl check for the LATTICE PMI DEF verilog package
for dirs in $ac_lattice_dirs; do
   for dir in $dirs; do
      if test -n "`ls -1 $dir/cae_library/synthesis/verilog/pmi_def.v 2> /dev/null`"; then
         ac_lattice_pmi_def="$dir/cae_library/synthesis/verilog/pmi_def.v";
         LATTICE_PMI_DEF=$ac_lattice_pmi_def
         echo "checking if LATTICE pmi_def.v is present in $dir/cae_library/synthesis/verilog ... yes $LATTICE_PMI_DEF"
      else
         echo "checking if LATTICE pmi_def.v is present in $dir/cae_library/synthesis/verilog ... no"
      fi
   done
done

if test "x$ac_lattice_pmi_def" != "x"; then
   AC_DEFINE_UNQUOTED(LATTICE_PMI_DEF, "${LATTICE_PMI_DEF}", "Define the LATTICE PMI DEF verilog package")
else
   AC_MSG_ERROR(LATTICE PMI DEF verilog file is required)
fi

dnl check for the LATTICE PMI TDPBE verilog package
for dirs in $ac_lattice_dirs; do
   for dir in $dirs; do
      if test -n "`ls -1 $dir/cae_library/simulation/verilog/pmi/pmi_ram_dp_true_be.v 2> /dev/null`"; then
         ac_lattice_pmi_tdpbe="$dir/cae_library/simulation/verilog/pmi/pmi_ram_dp_true_be.v";
         LATTICE_PMI_TDPBE=$ac_lattice_pmi_tdpbe
         echo "checking if LATTICE pmi_ram_dp_true_be.v is present in $dir/cae_library/simulation/verilog/pmi ... yes $LATTICE_PMI_TDPBE"
      else
         echo "checking if LATTICE pmi_ram_dp_true_be.v is present in $dir/cae_library/simulation/verilog/pmi ... no"
      fi
   done
done

if test "x$ac_lattice_pmi_tdpbe" != "x"; then
   AC_DEFINE_UNQUOTED(LATTICE_PMI_TDPBE, "${LATTICE_PMI_TDPBE}", "Define the LATTICE PMI TDPBE verilog package")
else
   AC_MSG_ERROR(LATTICE PMI TDPBE verilog file is required)
fi

dnl check for the LATTICE PMI MUL verilog package
for dirs in $ac_lattice_dirs; do
   for dir in $dirs; do
      if test -n "`ls -1 $dir/cae_library/simulation/verilog/pmi/pmi_dsp_mult.v 2> /dev/null`"; then
         ac_lattice_pmi_mul="$dir/cae_library/simulation/verilog/pmi/pmi_dsp_mult.v";
         LATTICE_PMI_MUL=$ac_lattice_pmi_mul
         echo "checking if LATTICE pmi_dsp_mult.v is present in $dir/cae_library/simulation/verilog/pmi ... yes $LATTICE_PMI_MUL"
      else
         echo "checking if LATTICE pmi_dsp_mult.v is present in $dir/cae_library/simulation/verilog/pmi ... no"
      fi
   done
done

if test "x$ac_lattice_pmi_mul" != "x"; then
   AC_DEFINE_UNQUOTED(LATTICE_PMI_MUL, "${LATTICE_PMI_MUL}", "Define the LATTICE PMI MUL verilog package")
else
   AC_MSG_ERROR(LATTICE PMI MUL verilog file is required)
fi

AC_PROVIDE([$0])dnl
])



