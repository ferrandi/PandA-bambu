################################################################################
#                                                                              #
#                      Checking common prerequisites                           #
#                                                                              #
################################################################################
AC_PREREQ(2.57)
AC_INIT(PandA-etc, 0.9.6-dev, panda-info@polimi.it)
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([-Wno-portability subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
TOPSRCDIR=`cd $srcdir; pwd`
BUILDDIR=`pwd`
AC_DISABLE_OPTION_CHECKING

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_DEFINE_UNQUOTED(HOST_GCC, "${CC}", "Define the host compiler")
AC_PROG_CPP
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_LEX
AC_PROG_YACC
AC_PROG_F77
AC_PROG_RANLIB
AX_BOOST_BASE([1.48.0])



################################################################################
#                                                                              #
#                          Configure options                                   #
#                                                                              #
################################################################################
AC_ETC_ARGS
AC_ARG_ENABLE(latex-table-format,  [  --enable-latex-table-format Add latex table formats to the distribution          ],[panda_USE_LATEX_TABLE_FORMAT="$enableval"])
AC_ARG_ENABLE(libbambu,            [  --enable-libbambu          Create libm library in raw format                    ],[panda_BUILD_LIBBAMBU="$enableval"])
AC_ARG_ENABLE(Werror,              [  --enable-Werror            Compile the source code with Werror option; a subset of possible warning is considered],
                                                                                                                          [panda_WERROR="$enableval"])
AC_ARG_WITH(max-I386-gcc,          [  --with-max-I386-gcc        Set the oldest version (larger than 4.5) of not supported I386 gcc ],[panda_MAX_I386_GCC="$withval"])
AC_ARG_WITH(min-I386-gcc,          [  --with-min-I386-gcc        Set the oldest version of supported I386 gcc                       ],[panda_MIN_I386_GCC="$withval"])


################################################################################
#                                                                              #
#                            Build implications                                #
#                                                                              #
################################################################################
AM_CONDITIONAL(BUILD_LIB_EXPERIMENTAL,  test "x$panda_USE_EXPERIMENTAL" = xyes)


################ FloPoCo stuff #############################
AM_CONDITIONAL(BUILD_FLOPOCO, test "x$panda_USE_FLOPOCO" = xyes)

################################################################################
#                                                                              #
#                                      .                                       #
#                                                                              #
################################################################################

AC_CONFIG_FILES(Makefile)



################################################################################
#                                                                              #
#                                 devices                                      #
#                                                                              #
################################################################################

AC_CONFIG_FILES(
   devices/Makefile
   devices/Altera_devices/Makefile
   devices/Xilinx_devices/Makefile
   devices/ASIC_devices/Makefile
   devices/Lattice_devices/Makefile
   )



################################################################################
#                                                                              #
#                           latex_table_format                                 #
#                                                                              #
################################################################################

if test "x$panda_USE_LATEX_TABLE_FORMAT" = xyes; then
  AC_CONFIG_FILES(
                  latex_table_format/Makefile
                 )
fi
AM_CONDITIONAL(BUILD_LATEX_TABLE_FORMAT, test "x$panda_USE_LATEX_TABLE_FORMAT" = xyes)



################################################################################
#                                                                              #
#                                gcc_plugin                                    #
#                                                                              #
################################################################################

################################# ARM Check ####################################
AM_CONDITIONAL(BUILD_ARM_GCC, test "x$build_ARM_GCC" = xyes)
AM_CONDITIONAL(BUILD_ARM_EMPTY_PLUGIN, test "x$build_ARM_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_ARM_RTL_PLUGIN, test "x$build_ARM_RTL_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_ARM_SSA_PLUGIN, test "x$build_ARM_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_ARM_SSA_PLUGINCPP, test "x$build_ARM_SSA_PLUGINCPP" = xyes)
AC_SUBST(ARM_EMPTY_PLUGIN)
AC_SUBST(ARM_SSA_PLUGIN)
AC_SUBST(ARM_RTL_PLUGIN)
AC_SUBST(ARM_GCC_PLUGIN_DIR)
AC_SUBST(ARM_GCC_EXE)
AC_SUBST(ARM_GCC_SRC_DIR)
AC_SUBST(ARM_PLUGIN_DIR)
AC_SUBST(ARM_GCC_VERSION)
AC_SUBST(ARM_GCC_BUILT_DIR)
AC_SUBST(ARM_HOST_PLUGIN_COMPILER)



################################# I386 Check ###################################
if test "x$panda_USE_I386_GCC" = xyes; then
   if test "x$panda_MAX_I386_GCC" = "x"; then
      AC_MSG_ERROR("Oldest not supported version of I386 gcc not set")
   fi
   if test "x$panda_MIN_I386_GCC" = "x"; then
      AC_MSG_ERROR("Oldest supported version of I386 gcc not set")
   fi
   AC_CHECK_PLUGIN_DIR
   AC_CHECK_GCC45_I386_VERSION($panda_MIN_I386_GCC,$panda_MAX_I386_GCC)
   AC_CHECK_GCC46_I386_VERSION($panda_MIN_I386_GCC,$panda_MAX_I386_GCC)
   AC_CHECK_GCC47_I386_VERSION($panda_MIN_I386_GCC,$panda_MAX_I386_GCC)
   AC_CHECK_GCC48_I386_VERSION($panda_MIN_I386_GCC,$panda_MAX_I386_GCC)
   AC_CHECK_GCC49_I386_VERSION($panda_MIN_I386_GCC,$panda_MAX_I386_GCC)
   AC_CHECK_GCC5_I386_VERSION($panda_MIN_I386_GCC,$panda_MAX_I386_GCC)
   AC_CHECK_GCC6_I386_VERSION($panda_MIN_I386_GCC,$panda_MAX_I386_GCC)
   AC_CHECK_GCC7_I386_VERSION($panda_MIN_I386_GCC,$panda_MAX_I386_GCC)
   if test "x$I386_GCC45_PLUGIN_COMPILER" = x && test "x$I386_GCC46_PLUGIN_COMPILER" = x && test "x$I386_GCC47_PLUGIN_COMPILER" = x && test "x$I386_GCC48_PLUGIN_COMPILER" = x && test "x$I386_GCC49_PLUGIN_COMPILER" = x && test "x$I386_GCC5_PLUGIN_COMPILER" = x && test "x$I386_GCC6_PLUGIN_COMPILER" = x && test "x$I386_GCC7_PLUGIN_COMPILER" = x; then
      AC_MSG_ERROR("gcc with support to -m32 and plugin not found")
   fi
fi

AM_CONDITIONAL(BUILD_I386_GCC45, test "x$build_I386_GCC45" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC45_EMPTY_PLUGIN, test "x$build_I386_GCC45_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC45_SSA_PLUGIN, test "x$build_I386_GCC45_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC45_SSA_PLUGINCPP, test "x$build_I386_GCC45_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC45_SSAVRP_PLUGIN, test "x$build_I386_GCC45_SSAVRP_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC45_TOPFNAME_PLUGIN, test "x$build_I386_GCC45_TOPFNAME_PLUGIN" = xyes)

AM_CONDITIONAL(BUILD_I386_GCC46, test "x$build_I386_GCC46" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC46_EMPTY_PLUGIN, test "x$build_I386_GCC46_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC46_SSA_PLUGIN, test "x$build_I386_GCC46_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC46_SSA_PLUGINCPP, test "x$build_I386_GCC46_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC46_SSAVRP_PLUGIN, test "x$build_I386_GCC46_SSAVRP_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC46_TOPFNAME_PLUGIN, test "x$build_I386_GCC46_TOPFNAME_PLUGIN" = xyes)

AM_CONDITIONAL(BUILD_I386_GCC47, test "x$build_I386_GCC47" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC47_EMPTY_PLUGIN, test "x$build_I386_GCC47_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC47_SSA_PLUGIN, test "x$build_I386_GCC47_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC47_SSA_PLUGINCPP, test "x$build_I386_GCC47_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC47_SSAVRP_PLUGIN, test "x$build_I386_GCC47_SSAVRP_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC47_TOPFNAME_PLUGIN, test "x$build_I386_GCC47_TOPFNAME_PLUGIN" = xyes)

AM_CONDITIONAL(BUILD_I386_GCC48, test "x$build_I386_GCC48" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC48_EMPTY_PLUGIN, test "x$build_I386_GCC48_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC48_SSA_PLUGIN, test "x$build_I386_GCC48_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC48_SSA_PLUGINCPP, test "x$build_I386_GCC48_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC48_SSAVRP_PLUGIN, test "x$build_I386_GCC48_SSAVRP_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC48_TOPFNAME_PLUGIN, test "x$build_I386_GCC48_TOPFNAME_PLUGIN" = xyes)

AM_CONDITIONAL(BUILD_I386_GCC49, test "x$build_I386_GCC49" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC49_EMPTY_PLUGIN, test "x$build_I386_GCC49_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC49_SSA_PLUGIN, test "x$build_I386_GCC49_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC49_SSA_PLUGINCPP, test "x$build_I386_GCC49_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC49_TOPFNAME_PLUGIN, test "x$build_I386_GCC49_TOPFNAME_PLUGIN" = xyes)

AM_CONDITIONAL(BUILD_I386_GCC5, test "x$build_I386_GCC5" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC5_EMPTY_PLUGIN, test "x$build_I386_GCC5_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC5_SSA_PLUGIN, test "x$build_I386_GCC5_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC5_SSA_PLUGINCPP, test "x$build_I386_GCC5_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC5_TOPFNAME_PLUGIN, test "x$build_I386_GCC5_TOPFNAME_PLUGIN" = xyes)

AM_CONDITIONAL(BUILD_I386_GCC6, test "x$build_I386_GCC6" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC6_EMPTY_PLUGIN, test "x$build_I386_GCC6_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC6_SSA_PLUGIN, test "x$build_I386_GCC6_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC6_SSA_PLUGINCPP, test "x$build_I386_GCC6_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC6_TOPFNAME_PLUGIN, test "x$build_I386_GCC6_TOPFNAME_PLUGIN" = xyes)

AM_CONDITIONAL(BUILD_I386_GCC7, test "x$build_I386_GCC7" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC7_EMPTY_PLUGIN, test "x$build_I386_GCC7_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC7_SSA_PLUGIN, test "x$build_I386_GCC7_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC7_SSA_PLUGINCPP, test "x$build_I386_GCC7_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC7_TOPFNAME_PLUGIN, test "x$build_I386_GCC7_TOPFNAME_PLUGIN" = xyes)

############################### SPARC Check ####################################
AM_CONDITIONAL(BUILD_SPARC_GCC, test "x$build_SPARC_GCC" = xyes)
AM_CONDITIONAL(BUILD_SPARC_EMPTY_PLUGIN, test "x$build_SPARC_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_SPARC_RTL_PLUGIN, test "x$build_SPARC_RTL_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_SPARC_SSA_PLUGIN, test "x$build_SPARC_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_SPARC_SSA_PLUGINCPP, test "x$build_SPARC_SSA_PLUGINCPP" = xyes)
AC_SUBST(SPARC_EMPTY_PLUGIN)
AC_SUBST(SPARC_SSA_PLUGIN)
AC_SUBST(SPARC_RTL_PLUGIN)
AC_SUBST(SPARC_GCC_PLUGIN_DIR)
AC_SUBST(SPARC_GCC_EXE)
AC_SUBST(SPARC_GCC_SRC_DIR)
AC_SUBST(SPARC_PLUGIN_DIR)
AC_SUBST(SPARC_GCC_VERSION)
AC_SUBST(SPARC_GCC_BUILT_DIR)
AC_SUBST(SPARC_HOST_PLUGIN_COMPILER)




if test "x$panda_USE_ARM_GCC" = xyes || test "x$panda_USE_I386_GCC" = xyes || test "x$panda_USE_SPARC_GCC" = xyes; then
   AC_CONFIG_FILES(
      gcc_plugin/Makefile
   )
fi
AM_CONDITIONAL(BUILD_GCC_PLUGIN, test "x$panda_USE_ARM_GCC" = xyes || test "x$panda_USE_I386_GCC" = xyes || test "x$panda_USE_SPARC_GCC" = xyes)
AM_CONDITIONAL(ENABLE_WERROR, test "x$panda_WERROR" = xyes)



################################################################################
#                                                                              #
#                                   libbambu                                   #
#                                                                              #
################################################################################
if test "x$panda_BUILD_LIBBAMBU" = xyes; then
   LIBBAMBU_SRCDIR="$TOPSRCDIR/libbambu"
   AC_DEFINE_UNQUOTED(LIBBAMBU_SRCDIR, "${LIBBAMBU_SRCDIR}", "Define the directory where the libbambu files are")

   AC_CONFIG_FILES(
      lib/Makefile
      lib/technology/Makefile
      lib/technology/dynamic_generators/Makefile
      libbambu/Makefile
      libbambu/libm/Makefile
      libbambu/pthread/Makefile
      libbambu/softfloat/Makefile
      libbambu/soft-fp/Makefile
   )
fi
AM_CONDITIONAL(BUILD_LIBBAMBU, test "x$panda_BUILD_LIBBAMBU" = "xyes")



################################################################################
#                                                                              #
#                                  macros                                      #
#                                                                              #
################################################################################

AC_CONFIG_FILES(macros/Makefile)



################################################################################
#                                                                              #
#                                 scripts                                      #
#                                                                              #
################################################################################

################################### MPPB #######################################
if test "x$panda_USE_MPPB" = xyes; then
   AC_CONFIG_FILES(
                  )
fi
AM_CONDITIONAL(BUILD_MPPB, test "x$panda_USE_MPPB" = xyes)

AC_CONFIG_FILES(scripts/Makefile)


################################################################################
#                                                                              #
#                                Autoheader                                    #
#                                                                              #
################################################################################

cd $TOPSRCDIR && autoheader configure.ac && cp config.h.in $BUILDDIR && cd $BUILDDIR
AC_CONFIG_HEADERS(config.h:config.h.in)
if test ! -d config_headers ; then
   mkdir config_headers
fi
for file in `find $BUILDDIR/config_headers -name *hpp`; do
   keyword=`grep define $file | awk '{print $2}'`
   found=`grep $keyword $BUILDDIR/config.h.in`
   if test "x$found" = x; then
      echo "removing $file"
      rm $file
      if test -f $file.in; then
         echo "removing $file.in"
         rm $file.in
      fi
   fi
done;
for keyword in `grep undef $BUILDDIR/config.h.in | awk '{print $2}'`; do
   if test ! -f config_headers/config_"$keyword".hpp.in; then
      echo "#define "$keyword 0 > config_headers/config_"$keyword".hpp.in
   fi
   AC_CONFIG_HEADERS([config_headers/config_"$keyword".hpp:config_headers/config_"$keyword".hpp.in])
done;



################################################################################
#                                                                              #
#                             Final generation                                 #
#                                                                              #
################################################################################


AC_OUTPUT

