################################################################################
#                                                                              #
#                      Checking common prerequisites                           #
#                                                                              #
################################################################################
AC_PREREQ(2.57)
AC_INIT(PandA-ext, 0.9.6-dev, panda-info@polimi.it)
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE(-Wno-portability)
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
TOPSRCDIR=`cd $srcdir; pwd`
BUILDDIR=`pwd`
AC_DISABLE_OPTION_CHECKING
AC_CONFIG_MACRO_DIR([../etc/macros])

if test -f set_variable.sh; then
   rm set_variable.sh
fi
touch set_variable.sh
echo "#!/bin/bash" >> set_variable.sh




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
echo "export FLIBS=\"$FLIBS\"" >> set_variable.sh

# we need bison instead of yacc
YACC_TRIMMED=`echo ${YACC}| sed 's/-y//;s/ //'`
if test ${YACC_TRIMMED} != bison; then
  AC_MSG_ERROR(bison is required)
fi
#and flex instead of lex
if test "$LEX" != flex; then
    AC_MSG_ERROR(flex is required)
fi
#now we take care of the different versions of flex
LEX_VERSION=`${LEX} -V`
AS_VERSION_COMPARE($LEX_VERSION, "2.5.34", flex_2_5_34=no, flex_2_5_34=yex, flex_2_5_34=yes)
if test "$flex_2_5_34" = yes; then
  # This is flex >= 2.5.34
  AC_DEFINE(HAVE_FLEX_2_5_34_OR_GREATER, 1, "Define flex greater or equal than 2.5.35")
fi
AS_VERSION_COMPARE($LEX_VERSION, "2.5.35", flex_2_5_35=no, flex_2_5_35=yex, flex_2_5_35=yes)
if test "$flex_2_5_35" = yes; then
  # This is flex >= 2.5.35
  AC_DEFINE(HAVE_FLEX_2_5_35_OR_GREATER, 1, "Define flex greater or equal than 2.5.35")
fi
# Checks for libraries.
AC_CHECK_LIB([m], [pow])
#AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([z], [gzopen])


# Checks for header files.
AC_FUNC_ALLOCA
AC_HEADER_STDC
AC_CHECK_HEADERS([float.h limits.h malloc.h sys/timeb.h termio.h zlib.h libintl.h stddef.h getopt.h ])
AC_HEADER_SYS_WAIT

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_C_VOLATILE
AC_CHECK_SIZEOF(unsigned long)
AC_CHECK_TYPES([ptrdiff_t])
AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([long])
AC_CHECK_SIZEOF([void *])
AC_CHECK_SIZEOF([long double])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_PROG_GCC_TRADITIONAL
AC_FUNC_STRTOD
AC_CHECK_FUNCS([floor memset modf pow sqrt strchr strcasecmp ])


AC_LANG([C++])
AC_TYPE_SIGNAL
AC_STRUCT_TM

AC_LIBTOOL_DLOPEN
AC_DISABLE_FAST_INSTALL
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

#check doxygen
test -z "$DOXYGEN" && AC_PATH_PROG(DOXYGEN, doxygen, doxygen,  ) 
AC_CHECK_PROG(DOX_HAVE_DOT,dot,YES,NO)

#check svnversion
AC_CHECK_PROG(HAVE_SVNVERSION, svnversion, yes, no)
AM_CONDITIONAL(SVNVERSION, test "x$HAVE_SVNVERSION" = xyes)

AC_SUBST(DOX_HAVE_DOT)
DOCUMENTATION_OUTPUTDIR=doc
AC_SUBST(DOCUMENTATION_OUTPUTDIR)



################################################################################
#                                                                              #
#                          Configure options                                   #
#                                                                              #
################################################################################
AC_EXT_ARGS
AC_ARG_ENABLE(32bits,              [  --enable-32bits              create 32bits binaries; ],                             [panda_32BITS="$enableval"])



################################### 32bits #####################################
if test "x$panda_32BITS" != x; then
   CXXFLAGS+=" -m32 "
   CFLAGS+=" -m32 "
   LDFLAGS+=" -m32 "
fi



################################################################################
#                                                                              #
#                       External libraries and tool                            #
#                                                                              #
################################################################################



################################### ABC stuff ##################################
if test "x$panda_USE_ABC" = xyes; then
  if test ! -f $TOPSRCDIR/abc70930.tar.gz; then
    AC_MSG_ERROR(ABC is missing)
  else
    AC_DEFINE(HAVE_ABC, 1, "Define if ABC is used")
    ABC_EXEC="$BUILDDIR/abc70930/abc"
    AC_DEFINE_UNQUOTED(ABC_EXEC, "${ABC_EXEC}", "Define the ABC executable")
    if test ! -f "$BUILDDIR/abc70930/abc"; then
       AC_MSG_ERROR(ABC executable is missing);
    fi
  fi
fi
AM_CONDITIONAL(BUILD_ABC, test "x$panda_USE_ABC" = xyes)



############################### OpenBeagle stuff ###############################
if test "x$panda_USE_BEAGLE" = xyes; then
   AC_APPEND(ENABLE_DOC,"BEAGLE")
   bash $TOPSRCDIR/../etc/macros/panda_config_subdir.sh beagle $ac_configure_args "'CXXFLAGS=${CXXFLAGS}'" "'CFLAGS=${CFLAGS}'" "'LDFLAGS=$LDFLAGS'"
   if test $? != 0; then
      AC_MSG_ERROR("Error in beagle configuration")
   fi
   AC_DEFINE(HAVE_BEAGLE, 1, "Define if beagle is used")
   echo "export BEAGLE_DIR=\"$TOPSRCDIR/beagle\"" >> set_variable.sh
fi
AM_CONDITIONAL(BUILD_BEAGLE, test "x$panda_USE_BEAGLE" = xyes)



################################# METSlib stuff ################################
if test "x$panda_USE_METSLIB" = xyes; then
   bash $TOPSRCDIR/../etc/macros/panda_config_subdir.sh metslib-0.5.3 $ac_configure_args "'CXXFLAGS=${CXXFLAGS}'" "'CFLAGS=${CFLAGS}'" "'LDFLAGS=$LDFLAGS'"
   if test $? != 0; then
      AC_MSG_ERROR("Error in metslib configuration")
   fi
   AC_DEFINE(HAVE_METSLIB, 1, "Define if metslib is used")
   echo "export METSLIB_DIR=\"$TOPSRCDIR/metslib-0.5.3\"" >> set_variable.sh
   echo "export METSLIB_OBJDIR=\"ext/metslib-0.5.3\"" >> set_variable.sh
fi
AM_CONDITIONAL(BUILD_METSLIB, test "x$panda_USE_METSLIB" = xyes)

################################# TRNG stuff ################################
if test "x$panda_USE_TRNG" = xyes; then
   bash $TOPSRCDIR/../etc/macros/panda_config_subdir.sh trng-4.17 $ac_configure_args "'CXXFLAGS=${CXXFLAGS}'" "'CFLAGS=${CFLAGS}'" "'LDFLAGS=$LDFLAGS'"
   if test $? != 0; then
      AC_MSG_ERROR("Error in trng configuration")
   fi
   AC_DEFINE(HAVE_TRNG, 1, "Define if trng is used")
   echo "export TRNG_DIR=\"$TOPSRCDIR/ext/trng-4.17\"" >> set_variable.sh
   echo "export TRNG_OBJDIR=\"ext/trng-4.17\"" >> set_variable.sh
fi
AM_CONDITIONAL(BUILD_TRNG, test "x$panda_USE_TRNG" = xyes)


################################# Coin-Or stuff ################################
if test "x$panda_USE_COIN_OR" = xyes; then
   # Checks for libraries.
   AX_BLAS()
   echo "export BLAS_LIBS=\"$BLAS_LIBS\"" >> set_variable.sh
   AX_LAPACK()
   echo "export LAPACK_LIBS=\"$LAPACK_LIBS\"" >> set_variable.sh
   AC_CHECK_LIB([bz2],[BZ2_bzReadOpen], [echo "Checking for libbz2... yes"], [echo "Checking for libbz2... no"; AC_MSG_ERROR("libbz2 not found")])
   $TOPSRCDIR/../etc/macros/panda_config_subdir.sh Coin-Cbc $ac_configure_args "'CXXFLAGS=${CXXFLAGS}'" "'CFLAGS=${CFLAGS}'" "'LDFLAGS=$LDFLAGS'"
   if test $? != 0; then
      AC_MSG_ERROR("Error in Coin-Cbc configuration")
   fi
   AC_DEFINE(HAVE_COIN_OR, 1, "Define if coin-or is used")
   echo "export COIN_OR_DIR=\"$TOPSRCDIR/Coin-Cbc\"" >> set_variable.sh
   echo "export COIN_OR_OBJDIR=\"ext/Coin-Cbc\"" >> set_variable.sh
fi
AM_CONDITIONAL(BUILD_COIN_OR, test "x$panda_USE_COIN_OR" = xyes)



################################## ARM Check ###################################
if test "x$panda_USE_ARM_GCC" = xyes; then
   if test "x$panda_MAX_ARM_GCC" = "x"; then
      AC_MSG_ERROR("Oldest not supported version of ARM gcc not set")
   fi
   if test "x$panda_MIN_ARM_GCC" = "x"; then
      AC_MSG_ERROR("Oldest supported version of ARM gcc not set")
   fi
   AC_CHECK_GCC_ARM_VERSION([4.5.0],[4.6.0])
   AC_DEFINE(HAVE_ARM_COMPILER, 1, "Define if ARM-treegcc is used")
   echo "export ARM_GCC_EXE=\"$ARM_GCC_EXE\"" >> set_variable.sh
   echo "export ARM_GCC_SRC_DIR=\"$ARM_GCC_SRC_DIR\"" >> set_variable.sh
   echo "export ARM_GCC_PLUGIN_DIR=\"$ARM_GCC_PLUGIN_DIR\"" >> set_variable.sh
   echo "export ARM_GCC_VERSION=\"$ARM_GCC_VERSION\"" >> set_variable.sh
   echo "export ARM_EMPTY_PLUGIN=\"$ARM_EMPTY_PLUGIN\"" >> set_variable.sh
   echo "export ARM_RTL_PLUGIN=\"$ARM_RTL_PLUGIN\"" >> set_variable.sh
   echo "export ARM_SSA_PLUGIN=\"$ARM_SSA_PLUGIN\"" >> set_variable.sh
   echo "export ARM_GCC_BUILT_DIR=\"$ARM_GCC_BUILT_DIR\"" >> set_variable.sh
   echo "export ARM_HOST_PLUGIN_COMPILER=\"$ARM_HOST_PLUGIN_COMPILER\"" >> set_variable.sh
   echo "export build_ARM_GCC=\"$build_ARM_GCC\"" >> set_variable.sh
   echo "export build_ARM_EMPTY_PLUGIN=\"$build_ARM_EMPTY_PLUGIN\"" >> set_variable.sh
   echo "export build_ARM_RTL_PLUGIN=\"$build_ARM_RTL_PLUGIN\"" >> set_variable.sh
   echo "export build_ARM_SSA_PLUGIN=\"$build_ARM_EMPTY_PLUGIN\"" >> set_variable.sh
fi
AM_CONDITIONAL(build_ARM_GCC, test "x$build_ARM_GCC" = xyes)



################################# SPARC Check ##################################
if test "x$panda_USE_SPARC_GCC" = xyes; then
   if test "x$panda_MAX_SPARC_GCC" = "x"; then
      AC_MSG_ERROR("Oldest not supported version of SPARC gcc not set")
   fi
   if test "x$panda_MIN_SPARC_GCC" = "x"; then
      AC_MSG_ERROR("Oldest supported version of SPARC gcc not set")
   fi
   AC_CHECK_SPARC_ELF
   AC_CHECK_GCC_SPARC_VERSION([4.5.0],[4.6.0])
   AC_DEFINE(HAVE_SPARC_COMPILER, 1, "Define if SPARC-treegcc is used")
   echo "export SPARC_GCC_EXE=\"$SPARC_GCC_EXE\"" >> set_variable.sh
   echo "export SPARC_GCC_SRC_DIR=\"$SPARC_GCC_SRC_DIR\"" >> set_variable.sh
   echo "export SPARC_GCC_PLUGIN_DIR=\"$SPARC_GCC_PLUGIN_DIR\"" >> set_variable.sh
   echo "export SPARC_GCC_VERSION=\"$SPARC_GCC_VERSION\"" >> set_variable.sh
   echo "export SPARC_EMPTY_PLUGIN=\"$SPARC_EMPTY_PLUGIN\"" >> set_variable.sh
   echo "export SPARC_RTL_PLUGIN=\"$SPARC_RTL_PLUGIN\"" >> set_variable.sh
   echo "export SPARC_SSA_PLUGIN=\"$SPARC_SSA_PLUGIN\"" >> set_variable.sh
   echo "export SPARC_GCC_BUILT_DIR=\"$SPARC_GCC_BUILT_DIR\"" >> set_variable.sh
   echo "export SPARC_HOST_PLUGIN_COMPILER=\"$SPARC_HOST_PLUGIN_COMPILER\"" >> set_variable.sh
   echo "export build_SPARC_GCC=\"$build_SPARC_GCC\"" >> set_variable.sh
   echo "export build_SPARC_EMPTY_PLUGIN=\"$build_SPARC_EMPTY_PLUGIN\"" >> set_variable.sh
   echo "export build_SPARC_RTL_PLUGIN=\"$build_SPARC_RTL_PLUGIN\"" >> set_variable.sh
   echo "export build_SPARC_SSA_PLUGIN=\"$build_SPARC_EMPTY_PLUGIN\"" >> set_variable.sh
fi
AM_CONDITIONAL(build_SPARC_GCC, test "x$build_SPARC_GCC" = xyes)



################################# crosstool-ng #################################
if test "x$build_ARM_GCC" = xyes || test "x$build_SPARC_GCC" = xyes; then
   panda_USE_CROSSTOOL_NG=yes
   AC_DEFINE(HAVE_CROSSTOOL_NG, 1, "Define if crostool-ng is used")
   AC_CHECK_PROG(have_cvs, cvs, yes, no)
   if test "x$have_cvs" = "xno"; then
      AC_MSG_ERROR(cvs command not found)
   fi
   AC_CHECK_PROG(have_gawk, gawk, yes, no)
   if test "x$have_gawk" = "xno"; then
      AC_MSG_ERROR(gawk command not found)
   fi
   AC_CHECK_PROG(have_makeinfo, makeinfo, yes, no)
   if test "x$have_makeinfo" = "xno"; then
      AC_MSG_ERROR(makeinfo command not found)
   fi
   AC_CHECK_HEADER([ncurses.h], have_ncurses=yes, have_ncurses=no)
   if test "x$have_ncurses" = "xno"; then
      AC_MSG_ERROR(ncurses header not found)
   fi
   panda_USE_CROSSTOOL_NG=yes;
   if test -d /opt/x-tools; then
     echo "checking /opx/x-tools... yes"
   else
     echo "checking /opt/x-tools...no - creating"
     mkdir /opt/x-tools || AC_MSG_ERROR(Can not create /opt/x-tools)
   fi
   if test "x$build_ARM_GCC" = xyes; then
      if test -d /opt/x-tools/build_arm; then
         echo "checking /opx/x-tools/build_arm... yes"
      else
         echo "checking /opx/x-tools/build_arm... no - creating"
         mkdir /opt/x-tools/build_arm || AC_MSG_ERROR(Can not create /opt/x-tools/build_arm)
      fi
      if test -d /opt/x-tools/arm; then
         echo "checking /opx/x-tools/arm... yes"
      else
         echo "checking /opx/x-tools/arm... no - creating"
         mkdir /opt/x-tools/arm || AC_MSG_ERROR(Can not create /opt/x-tools/arm)
      fi
   fi
   if test "x$build_SPARC_GCC" = xyes; then
      if test -d /opt/x-tools/build_sparc; then
         echo "checking /opx/x-tools/build_sparc... yes"
      else
         echo "checking /opx/x-tools/build_sparc... no - creating"
         mkdir /opt/x-tools/build_sparc || AC_MSG_ERROR(Can not create /opt/x-tools/build_sparc)
      fi
      if test -d /opt/x-tools/sparc; then
         echo "checking /opx/x-tools/sparc... yes"
      else
         echo "checking /opx/x-tools/sparc... no - creating"
         mkdir /opt/x-tools/sparc || AC_MSG_ERROR(Can not create /opt/x-tools/sparc)
      fi
   fi
   AC_CONFIG_FILES(
                  )
   echo "export panda_USE_CROSSTOOL_NG=\"$panda_USE_CROSSTOOL_NG\"" >> set_variable.sh
fi
AM_CONDITIONAL(BUILD_CROSSTOOL_NG, test "x$panda_USE_CROSSTOOL_NG" = xyes)



################################## CUDD stuff ##################################
if test "x$panda_USE_CUDD" = xyes; then
   AC_APPEND(ENABLE_DOC,"CUDD")

   AC_DEFINE(HAVE_CUDD, 1, "Define if CUDD is used")
   $TOPSRCDIR/../etc/macros/panda_config_subdir.sh cudd $ac_configure_args "'CXXFLAGS=${CXXFLAGS}'" "'CFLAGS=${CFLAGS}'" "'LDFLAGS=$LDFLAGS'"
   echo "export CUDD_CPPFLAGS=\"-I$TOPSRCDIR/cudd/cudd -I$TOPSRCDIR/cudd/nanotrav -I$TOPSRCDIR/cudd/mtr -I$TOPSRCDIR/cudd/epd -I$TOPSRCDIR/cudd/st -I$TOPSRCDIR/cudd/util -I$TOPSRCDIR/cudd/cplusplus\"" >> set_variable.sh
   echo "export CUDD_LIBS=\"$BUILDDIR/cudd/cplusplus/libobj.la $BUILDDIR/cudd/cudd/libcudd.la\"" >> set_variable.sh
fi
AM_CONDITIONAL(BUILD_LIB_CUDD, test "x$panda_USE_CUDD" = xyes)



################################# FloPoCo stuff ################################
if test "x$panda_USE_FLOPOCO" = xyes; then
   AC_CHECK_LIB([gmpxx], [main], [HAVE_LIBGMPXX=yes], [])
   AC_CHECK_LIB([gmp], [main], [HAVE_LIBGMP=yes], [])
   AC_CHECK_LIB([mpfr], [main], [HAVE_LIBMPFR=yes], [])
   if test "x$panda_32BITS" != x; then
      LIBS="-L/usr/lib32/ $LIBS";
   fi
   AC_CHECK_HEADERS([gmpxx.h gmp.h mpfr.h])
   if test -z $HAVE_LIBMPFR || test -z $HAVE_LIBGMP || test -z $HAVE_LIBGMPXX; then
      AC_MSG_ERROR(either libmpfr, libgmp or libgmpxx is missing)
   fi
   $TOPSRCDIR/../etc/macros/panda_config_subdir.sh libfplll-4.0.3 $ac_configure_args "'CXXFLAGS=${CXXFLAGS}'" "'CFLAGS=${CFLAGS}'" "'LDFLAGS=$LDFLAGS'" --prefix=${prefix}
   if test $? != 0; then
      AC_MSG_ERROR("Error in fplll configuration")
   fi
   cd libfplll-4.0.3
   make
   make install
   if test $? != 0; then
      AC_MSG_ERROR("Error in fplll compilation")
   fi
   cd ..

   AC_DEFINE(HAVE_SOLLYA, 1, "Define if Sollya is used")
   AC_DEFINE(HAVE_HOTBM, 1, "")
   AC_DEFINE(HAVE_LNS, 1, "LNS available")
   $TOPSRCDIR/../etc/macros/panda_config_subdir.sh sollya $ac_configure_args "'CFLAGS=${CFLAGS}'" "'CXXFLAGS=${CXXFLAGS} -I${prefix}/include'" "'LDFLAGS=${LDFLAGS} -L${prefix}/lib'";
   if test $? != 0; then
      AC_MSG_ERROR("Error in sollya configuration")
   fi
   cd sollya
   make
   if test $? != 0; then
      AC_MSG_ERROR("Error in sollya compilation")
   fi
   cd ..
   $TOPSRCDIR/../etc/macros/panda_config_subdir.sh flopoco $ac_configure_args "'CXXFLAGS=${CXXFLAGS}'" "'CFLAGS=${CFLAGS}'" "'LDFLAGS=${LDFLAGS} -L$BUILDDIR/sollya/.libs'"
   if test $? != 0; then
      AC_MSG_ERROR("Error in flopoco configuration")
   fi
   AC_DEFINE(HAVE_FLOPOCO, 1, "Define if flopoco is used")
   echo "export FLOPOCO_DIR=\"$TOPSRCDIR/flopoco\"" >> set_variable.sh
fi
AM_CONDITIONAL(BUILD_FLOPOCO, test "x$panda_USE_FLOPOCO" = xyes)



############################### GRLIB ##########################################
if test "x$panda_USE_GRLIB" = xyes; then
    AC_DEFINE_UNQUOTED(GRLIB_DIR, "${prefix}/lib/GRLIB", "Define the GRLIB directory")
fi
AM_CONDITIONAL(USE_GRLIB, test "x$panda_USE_GRLIB" = xyes)


############################### Liberty checking ###############################
if test "x$panda_USE_LIB_FROM_LIBERTY" = xyes; then
   $TOPSRCDIR/../etc/macros/panda_config_subdir.sh liberty_parser $ac_configure_args "'CXXFLAGS=${CXXFLAGS}'" "'CFLAGS=${CFLAGS}'" "'LDFLAGS=$LDFLAGS'"
   if test $? != 0; then
      AC_MSG_ERROR("Error in liberty parser configuration")
   fi
   AC_DEFINE(HAVE_FROM_LIBERTY, 1, "Define if liberty parser is used")
fi
AM_CONDITIONAL(BUILD_LIBERTY_PARSER, test "x$panda_USE_LIB_FROM_LIBERTY" = xyes)



################################ LP-SOLVE stuff ################################
if test "x$panda_USE_LP_SOLVE" = xyes; then
  USE_BFP_NAME=LUSOL
  #etaPFI
  #etaPFI2
  #GLPK
  #LUSOL
  if test ! -f $TOPSRCDIR/lpsolve5/lp_solve_USE_$USE_BFP_NAME; then
     rm -f $TOPSRCDIR/lpsolve5/lp_solve_USE_*
     touch $TOPSRCDIR/lpsolve5/lp_solve_USE_$USE_BFP_NAME
     if test -d  lpsolve5/bfp; then
       (cd lpsolve5/bfp; make clean;)
     fi
     touch $TOPSRCDIR/lpsolve5/configure.in
   fi
   if test $USE_BFP_NAME == etaPFI2; then
      echo "export BFP_NAME=\"bfp_etaPFI\"" >> set_variable.sh;
   else
      echo "export BFP_NAME=\"bfp_$USE_BFP_NAME\"" >> set_variable.sh;
   fi
   $TOPSRCDIR/../etc/macros/panda_config_subdir.sh lpsolve5 $ac_configure_args "'CXXFLAGS=${CXXFLAGS}'" "'CFLAGS=${CFLAGS}'" "'LDFLAGS=$LDFLAGS'"
   AC_DEFINE(HAVE_LP_SOLVE, 1, "Define if lp solver is used")
   echo "export LP_SOLVE_DIR=\"$TOPSRCDIR/lpsolve5\"" >> set_variable.sh
fi
AM_CONDITIONAL(BUILD_LP_SOLVE, test "x$panda_USE_LP_SOLVE" = xyes)



################################# NODELIB stuff ################################
if test "x$panda_USE_NODELIB" = xyes; then
   AC_DEFINE(HAVE_NODELIB, 1, "Define if nodelib is used")
   $TOPSRCDIR/../etc/macros/panda_config_subdir.sh nodelib $ac_configure_args "'CXXFLAGS=${CXXFLAGS}'" "'CFLAGS=${CFLAGS}'" "'LDFLAGS=$LDFLAGS'"
fi
AM_CONDITIONAL(BUILD_NODELIB, test "x$panda_NODELIB" = xyes)



############################### RapidMiner stuff ###############################
if test "x$panda_USE_RAPIDMINER" = xyes; then
   AC_CHECK_RAPIDMINER
   AC_DEFINE(HAVE_RAPIDMINER, 1, "Define if rapidminer is used")
fi
AM_CONDITIONAL(BUILD_RAPIDMINER, test "x$panda_USE_RAPIDMINER" = xyes)



################################## SimIt stuff #################################
if test "x$panda_USE_SIMIT" = xyes; then
   AC_DEFINE(HAVE_SIMIT, 1, "Define if simit is used")
   $TOPSRCDIR/../etc/macros/panda_config_subdir.sh SimIt-PoliMi $ac_configure_args "'CXXFLAGS=${CXXFLAGS}'" "'CFLAGS=${CFLAGS}'" "'LDFLAGS=$LDFLAGS'"
   if test $? != 0; then
      AC_MSG_ERROR("Error in simit configuration")
   fi
fi
AM_CONDITIONAL(BUILD_SIMIT, test "x$panda_USE_SIMIT" = xyes)



############################## VFlib library stuff #############################
if test "x$panda_USE_VFLIB" = xyes; then
   $TOPSRCDIR/../etc/macros/panda_config_subdir.sh vflib $ac_configure_args "'CXXFLAGS=${CXXFLAGS}'" "'CFLAGS=${CFLAGS}'" "'LDFLAGS=$LDFLAGS'"
   echo "export VFLIB_DIR=\"$TOPSRCDIR/vflib\"" >> set_variable.sh
fi
AM_CONDITIONAL(BUILD_VFLIB, test "x$panda_USE_VFLIB" = xyes)




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

AC_CONFIG_FILES(Makefile)

AC_OUTPUT
