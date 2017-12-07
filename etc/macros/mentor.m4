dnl
dnl check where the MENTOR synthesis tools are
dnl
AC_DEFUN([AC_CHECK_MENTOR],[
    AC_ARG_WITH(mentor-dir,
    [  --with-mentor-dir=DIR   set where the root of MENTOR tools are installed ],
    [
       ac_mentor_dir="$withval"
    ])
    AC_ARG_WITH(mentor-license,
    [  --with-mentor-license=STRING  set the LM_LICENSE_FILE value ],
    [
       ac_mentor_license="$withval"
    ])
    AC_ARG_ENABLE(mentor-optimizer,
    [  --disable-mentor-optimizer   disable the mentor optimizer],
    [],
    [enable_mentor_optimizer=yes])

if test "x$ac_mentor_dir" = x; then
   ac_mentor_dirs="/opt/mentor /opt/mentor/*";
else
   ac_mentor_dirs=$ac_mentor_dir;
fi

if test "x$ac_mentor_license" = x; then
   echo "MENTOR modelsim license not set. User has to define LM_LICENSE_FILE variable before run a simulation."
fi
AC_DEFINE_UNQUOTED(MENTOR_LICENSE, "${ac_mentor_license}", "Define the mentor LICENSE VALUE")

dnl check for the MENTOR configuration setup script: settings32.sh/settings64.sh
   for dirs in $ac_mentor_dirs; do
      for dir in $dirs; do
         if test -n "`ls -1 $dir/bin/vsim 2> /dev/null`"; then
            ac_mentor_modelsim_bin="$dir/bin";
            echo "checking if MENTOR modelsim bin dir is present in $dir... yes $ac_mentor_modelsim_bin"
         else
            echo "checking if MENTOR modelsim bin dir is present in $dir... no"
         fi
      done
   done

   for dirs in $ac_mentor_dirs; do
      for dir in $dirs; do
         if test -n "`ls -1 $dir/bin/visualizer 2> /dev/null`"; then
            ac_mentor_visualizer_exe="$dir/bin/visualizer";
            echo "checking if MENTOR visualizer bin is present in $dir... yes $ac_mentor_visualizer_exe"
         else
            echo "checking if MENTOR visualizer bin dir is present in $dir... no"
         fi
      done
   done

if test "x$ac_mentor_modelsim_bin" != "x"; then
   AC_DEFINE_UNQUOTED(MENTOR_MODELSIM_BIN, "${ac_mentor_modelsim_bin}", "Define the mentor modelsim bin directory")
else
   AC_MSG_ERROR(MENTOR modelsim not properly configured)
fi

if test "x$ac_mentor_visualizer_exe" != "x"; then
   AC_DEFINE_UNQUOTED(MENTOR_VISUALIZER_EXE, "${ac_mentor_visualizer_exe}", "Define the mentor visualizer executable")
   AC_DEFINE(HAVE_MENTOR_VISUALIZER_EXE,1,[define if the mentor visualizer is available])
fi

if test "x$enable_mentor_optimizer" = "xyes"; then
   AC_DEFINE(MODELSIM_OPTIMIZER_FLAGS, "-O5", "Define the mentor modelsim optimizer flags")
else
   AC_DEFINE(MODELSIM_OPTIMIZER_FLAGS, "", "No mentor modelsim optimizer flags defined")
fi


AC_PROVIDE([$0])dnl
])



