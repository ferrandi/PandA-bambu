################################################################################
#                                                                              #
#                      Checking common prerequisites                           #
#                                                                              #
################################################################################
AC_PREREQ(2.57)
AC_INIT(PandA, 0.9.7-dev, panda-info@polimi.it)
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([-Wno-portability 1.9 tar-pax subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_SRCDIR([configure.ac])

AC_DISABLE_SHARED
#AC_DISABLE_STATIC
AC_CONFIG_MACRO_DIR(etc/macros)

#Flags have to be emptied since AC_PROG_CXX and AC_PROG_CC set them to "-g -O2"
: ${CFLAGS=""}
: ${CXXFLAGS=""}
CFLAGS_EXT=-O2
CXXFLAGS_EXT=-O2
CXXFLAGS_EXTFPLL=-O2
CXXFLAGS_STD=-O2

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

AC_ARG_ENABLE(clanglibcpp,         [  --enable-clanglibcpp         Clang is going to use libc++ ],                                                          [panda_clanglibcpp="$enableval"], [panda_clanglibcpp="no"])
AC_ARG_ENABLE(oldclangabi,         [  --enable-oldclangabi         Clang plugins are compiled with the old abi ],                                           [panda_clangoldabi="$enableval"], [panda_clangoldabi="no"])
###################### libc++ support #######################
if test "x$panda_clanglibcpp" = xyes; then
  CXXFLAGS+=" -stdlib=libc++ "
  if test "x$panda_clangoldabi" = xyes; then
    CXXFLAGS+=-D_GLIBCXX_USE_CXX11_ABI=0
  fi
  CXXFLAGS_EXT+=" -stdlib=libc++ "
  if test "x$panda_clangoldabi" = xyes; then
    CXXFLAGS_EXT+=-D_GLIBCXX_USE_CXX11_ABI=0
  fi
  CXXFLAGS_STD+=" -stdlib=libc++ "
  if test "x$panda_clangoldabi" = xyes; then
    CXXFLAGS_STD+=-D_GLIBCXX_USE_CXX11_ABI=0
  fi
fi


# Check if compiler support c++17
AC_COMPILE_STDCXX_17
# Check if compiler support c++11
AC_COMPILE_STDCXX_11
# Check if compiler support c++0x
AC_COMPILE_STDCXX_0X
# Check if compiler support std::hexfloat
AC_HEXFLOAT


############################### c++0x/11 checking ##############################
if test "x$ac_cv_cxx_compile_cxx17_native" = xno; then
   if test "x$ac_cv_cxx_compile_cxx17_cxx" = xyes; then
      CXXFLAGS+=" --std=c++17 "
      CXXFLAGS_EXT+=" --std=c++17 "
      CXXFLAGS_STD+=" --std=c++17 "
   elif test "x$ac_cv_cxx_compile_cxx17_gxx" = xyes; then
      CXXFLAGS+=" --std=g++17 "
      CXXFLAGS_EXT+=" --std=g++17 "
      CXXFLAGS_STD+=" --std=g++17 "
   elif test "x$ac_cv_cxx_compile_cxx17_cxx1z" = xyes; then
      CXXFLAGS+=" --std=c++1z "
      CXXFLAGS_EXT+=" --std=c++1z "
      CXXFLAGS_STD+=" --std=c++1z "
   elif test "x$ac_cv_cxx_compile_cxx11_native" = xyes; then
      CXXFLAGS+=" "
      CXXFLAGS_EXT+=" "
   elif test "x$ac_cv_cxx_compile_cxx11_cxx" = xyes; then
      CXXFLAGS+=" --std=c++11 "
      CXXFLAGS_EXT+=" --std=c++11 "
   elif test "x$ac_cv_cxx_compile_cxx11_gxx" = xyes; then
      CXXFLAGS+=" --std=g++11 "
      CXXFLAGS_EXT+=" --std=g++11 "
   elif test "x$ac_cv_cxx_compile_cxx11_cxx1x" = xyes; then
      CXXFLAGS+=" --std=c++1x "
      CXXFLAGS_EXT+=" --std=c++1x "
   elif test "x$ac_cv_cxx_compile_cxx0x_cxx" = xyes; then
      CXXFLAGS+=" --std=c++0x "
      CXXFLAGS_EXT+=" --std=c++0x "
   else
      AC_MSG_ERROR("Compiler does not support C++17 nor C++11 nor C++0X")
   fi
fi
AC_SUBST(CXXFLAGS_STD)
AM_CONDITIONAL(BUILD_MOCKTURTLE, test "x$ac_cv_cxx_compile_cxx17_native" = xyes || test "x$ac_cv_cxx_compile_cxx17_cxx" = xyes || test "x$ac_cv_cxx_compile_cxx17_gxx" = xyes || test "x$ac_cv_cxx_compile_cxx17_cxx1z" = xyes)


AM_PO_SUBDIRS

# we need bison instead of yacc
YACC_TRIMMED=$(echo ${YACC}| sed 's/-y//;s/ //')
if test ${YACC_TRIMMED} != bison; then
  AC_MSG_ERROR(bison is required)
fi
#and flex instead of lex
if test "$LEX" != flex; then
    AC_MSG_ERROR(flex is required)
fi
#now we take care of the different versions of bison
bison_27=no
BISON_VERSION=$(${YACC} -V | head -n1 | awk '{print $4}')
AS_VERSION_COMPARE(${BISON_VERSION}, "2.7", bison_27=no, bison_27=yes, bison_27=yes)
if test "$bison_27" = yes; then
  AC_DEFINE(HAVE_BISON_2_7_OR_GREATER, 1, "Define yacc greater or equal than 2.7")
fi
AM_CONDITIONAL(BISON_2_7_OR_GREATER, test "x$bison_27" = xyes)

#now we take care of the different versions of flex
LEX_VERSION=$(${LEX} -V)
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
AC_CHECK_HEADERS([float.h limits.h sys/timeb.h zlib.h stddef.h getopt.h ], [],[AC_MSG_ERROR("$ac_header not found")])
AC_HEADER_SYS_WAIT

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_C_VOLATILE
AC_CHECK_SIZEOF(unsigned long)

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

#check MinGW compilation
MINGW_AC_WIN32_NATIVE_HOST
AM_CONDITIONAL([MINGW64], [test x$mingw_cv_win32_host = xyes])
if test "x$mingw_cv_win32_host" = xyes; then
  CXXFLAGS+=" -idirafter /usr/include "
fi

#check doxygen
test -z "$DOXYGEN" && AC_PATH_PROG(DOXYGEN, doxygen, doxygen,  ) 
AC_CHECK_PROG(DOX_HAVE_DOT,dot,YES,NO)

#check svnversion
AC_CHECK_PROG(HAVE_SVNVERSION, svnversion, yes, no)

AC_SUBST(DOX_HAVE_DOT)
DOCUMENTATION_OUTPUTDIR=doc
AC_SUBST(DOCUMENTATION_OUTPUTDIR)

#check BOOST
AX_BOOST_BASE([1.48.0])
AX_BOOST_SYSTEM
AX_BOOST_DATE_TIME
AX_BOOST_FILESYSTEM
AX_BOOST_REGEX

AC_DEFINE_UNQUOTED(BOOST_INCLUDE_DIR, "${BOOST_DIR}", "Define the boost include dir")

PANDA_CHECK_BOOST_SCOPED_ENUMS
if test "x$panda_boost_no_scoped_enums" = xyes; then
   CXXFLAGS+=" -DBOOST_NO_SCOPED_ENUMS -DBOOST_NO_CXX11_SCOPED_ENUMS "
fi
PANDA_CHECK_RPATH_FLAVOR

AC_LINUX_DISTRO

TOPSRCDIR=$(cd $srcdir; pwd)
BUILDDIR=$(pwd)
LAST_DIRECTORY=`echo "$TOPSRCDIR" | awk -F/ '{print $NF}'`


GIT_WORKTREE_PATH=$(echo $TOPSRCDIR/.git)
GIT_PRIVATE_PANDA=no
AC_MSG_CHECKING([if directory is a git repository])
if test -d $GIT_WORKTREE_PATH && test -f $GIT_WORKTREE_PATH/HEAD && test -f $GIT_WORKTREE_PATH/index; then
   AC_MSG_RESULT([yes])
   GIT_REMOTE_URL=$(git --git-dir $GIT_WORKTREE_PATH config --get remote.origin.url)
   AC_MSG_CHECKING([if this is clone of ferrandi/private-PandA-bambu.git])
   if [[[ $GIT_REMOTE_URL == *"private-PandA-bambu.git"* ]]]; then
      GIT_PRIVATE_PANDA=yes
      AC_MSG_RESULT([yes])
   else
      AC_MSG_RESULT([no: $GIT_REMOTE_URL])
   fi
else
   AC_MSG_RESULT([no])
   AC_MSG_CHECKING([if directory is a git worktree])
   if test -f $TOPSRCDIR/.git; then
      GIT_WORKTREE_PATH=$(head -1 $TOPSRCDIR/.git | sed -e 's/^gitdir://g')
      if test -d $GIT_WORKTREE_PATH && test -f $GIT_WORKTREE_PATH/HEAD && test -f $GIT_WORKTREE_PATH/index; then
         AC_MSG_RESULT([yes])
      fi
   else
      AC_MSG_RESULT([no])
   fi
fi

AM_CONDITIONAL(INSIDE_GIT, test -d $GIT_WORKTREE_PATH && test -f $GIT_WORKTREE_PATH/HEAD && test -f $GIT_WORKTREE_PATH/index)
AC_SUBST(GIT_WORKTREE_PATH)

if test $TOPSRCDIR = $PWD; then
   echo "";
   echo "     ERROR: Build and srcs dir must be different";
   echo "";
   exit 1;
fi

if test "$prefix" = "NONE"; then
   PREFIX=$ac_default_prefix
else
   PREFIX=$prefix
fi
AC_DEFINE_UNQUOTED(PANDA_INCLUDE_INSTALLDIR, "${prefix}/include", "Define where panda includes are installed")
AC_DEFINE_UNQUOTED(PANDA_LIB_INSTALLDIR, "${prefix}/lib", "Define where panda libraries are installed")
AC_DEFINE_UNQUOTED(PANDA_DATA_INSTALLDIR, "${prefix}/share", "Define where panda data are installed")

################################################################################
#                                                                              #
#                       Architecture options                                   #
#                                                                              #
################################################################################
############### x86_64 checking ############################
if test "$target_cpu" = x86_64; then
   AC_DEFINE(X86_64, 1, "Define if host system is 64 bits")
fi
AM_CONDITIONAL(TARGET_X86_64, test "$target_cpu" = x86_64)



################################################################################
#                                                                              #
#                          Configure options                                   #
#                                                                              #
################################################################################
#To build a NEW_PROJECT:
#check the configure parameters to see if the --enable-NEW_PROJECT has been activated 
#and set the panda_USE_NEW_PROJECTD variable to $enableval which is the empty string or yes
AC_ARG_ENABLE(all,                 [  --enable-all                 compile all tools],                                    [panda_USE_ALL="$enableval"])

#tools
AC_ARG_ENABLE(spider,                [  --enable-spider              compile spider tool for parse profiling analysis data], [panda_USE_SPIDER="$enableval"])
AC_ARG_ENABLE(bambu,                 [  --enable-bambu               compile bambu tool for High-Level Synthesis],           [panda_USE_BAMBU="$enableval"], [panda_USE_BAMBU="yes"])
AC_ARG_ENABLE(eucalyptus,            [  --enable-eucalyptus          compile eucalyptus tool for RTL characterization],      [panda_USE_EUCALYPTUS="$enableval"])
AC_ARG_ENABLE(treegcc,               [  --enable-treegcc             compile tree-panda-gcc tool for gimple compilation],    [panda_USE_TREE_PANDA_GCC="$enableval"])

#Ext libraries tools
AC_ARG_ENABLE(modelsim,               [  --enable-modelsim            enable modelsim simulator],                               [panda_USE_MODELSIM="$enableval"])

#synthesis wrappers
AC_ARG_ENABLE(xilinx,                 [  --enable-xilinx              compile the wrapper for interfacing with Xilinx tools],[panda_USE_WRAPPER_XILINX="$enableval"])
AC_ARG_ENABLE(altera,                 [  --enable-altera              compile the wrapper for interfacing with Altera tools],[panda_USE_WRAPPER_ALTERA="$enableval"])
AC_ARG_ENABLE(lattice,                [  --enable-lattice             compile the wrapper for interfacing with Lattice tools],[panda_USE_WRAPPER_LATTICE="$enableval"])
AC_ARG_ENABLE(nanoxplore,             [  --enable-nanoxplore          compile the wrapper for interfacing with NanoXplore tools],[panda_USE_WRAPPER_NANOXPLORE="$enableval"])
AC_ARG_ENABLE(verilator,              [  --disable-verilator          disable the/ wrapper to Verilator Verilog compiler],   [panda_USE_VERILATOR="$enableval"], [panda_USE_VERILATOR="yes"])


#aid stuff
AC_ARG_ENABLE(release,             [  --enable-release             compile the source code for release (default enabled)],                                  [panda_RELEASE="$enableval"], [panda_RELEASE="yes"])
AC_ARG_ENABLE(unordered,           [  --enable-unordered           compile the source code with unordered data structure (default disabled)],               [panda_UNORDERED="$enableval"], [panda_UNORDERED="no"])
AC_ARG_ENABLE(Werror,              [  --enable-Werror              compile the source code with Werror option; a subset of possible warning is considered], [panda_WERROR="$enableval"])
AC_ARG_ENABLE(asserts,             [  --disable-asserts            disable most of the runtime check (default enabled)],                                    [panda_ASSERTS="$enableval"], [panda_ASSERTS="yes"])
AC_ARG_ENABLE(debug,               [  --enable-debug               compile the source code with debug symbols (default disabled)],                          [panda_DEBUG="$enableval"], [panda_DEBUG="no"])
AC_ARG_ENABLE(opt,                 [  --enable-opt                 compile the source code with -O2 (default enabled) ],                                    [panda_OPT="$enableval"], [panda_OPT="yes"])
AC_ARG_ENABLE(allstatic,           [  --enable-allstatic           create static binaries ],                                                                [panda_ALLSTATIC="$enableval"], [panda_ALLSTATIC="yes"])
AC_ARG_ENABLE(32bits,              [  --enable-32bits              create 32bits binaries ],                                                                [panda_32BITS="$enableval"])


#etc/ext stuff
AC_ARG_ENABLE(flopoco,             [  --enable-flopoco             compile FloPoCo external library],                     [panda_USE_FLOPOCO="$enableval"])
AC_ARG_ENABLE(icarus,              [  --enable-icarus              compile Icarus Verilog compiler],                      [panda_USE_ICARUS="$enableval"])
AC_ARG_ENABLE(verilator,           [  --enable-verilator           compile Verilator Verilog compiler],                   [panda_USE_VERILATOR="$enableval"])

################################################################################
#                                                                              #
#                          Variable initialization                             #
#                                                                              #
################################################################################
panda_USE_FRACTIONS=no;
panda_USE_GENERIC_BASHBACKEND=no
panda_USE_I386_GCC=no;
panda_USE_LIB_AIG=no;
panda_USE_LIB_ALGORITHMS=no;
panda_USE_LIB_ARCH=no;
panda_USE_LIB_DESIGN_ANALYSIS=no;
panda_USE_LIB_BACKEND=no;
panda_USE_LIB_BDD=no;
panda_USE_LIB_BEHAVIOR=no;
panda_USE_LIB_BOOLEAN=no;
panda_USE_LIB_BRIDGE=no;
panda_USE_LIB_CIRCUIT=no;
panda_USE_LIB_CLIQUECOVER=no;
panda_USE_LIB_CLUSTERING=no;
panda_USE_LIB_CMOS=no;
panda_USE_LIB_CODE_ESTIMATION=no;
panda_USE_LIB_CODESIGN=no;
panda_USE_LIB_CUDA_FAULT_SIMULATION=no;
panda_USE_LIB_DOMINANCE=no;
panda_USE_LIB_DYNAMIC_METRICS=no;
panda_USE_LIB_EDIF=no;
panda_USE_LIB_EVALUATION=no;
panda_USE_LIB_FROM_AIGER=no;
panda_USE_LIB_FROM_ARCH=no;
panda_USE_LIB_FROM_BLIF=no;
panda_USE_LIB_FROM_BOOLEAN=no;
panda_USE_LIB_FROM_C=no;
panda_USE_LIB_FROM_CSV=no;
panda_USE_LIB_FROM_DEEP_PROFILING=no;
panda_USE_LIB_FROM_IPXACT=no;
panda_USE_LIB_FROM_PSPLIB=no;
panda_USE_LIB_FROM_RTL=no;
panda_USE_LIB_FROM_DATA_XML=no;
panda_USE_LIB_FROM_STRUCTURAL=no;
panda_USE_LIB_FROM_SYSTEMC=no;
panda_USE_LIB_FROM_TGFF=no;
panda_USE_LIB_GRAPH=no;
panda_USE_LIB_GRAPH_PARTITIONING=no;
panda_USE_LIB_GRAPHHELPER=no;
panda_USE_LIB_GSL=no;
panda_USE_LIB_HLS=no;
panda_USE_LIB_HOST_PROFILING=no;
panda_USE_LIB_LIBRARY_CHARACTERIZATION=no;
panda_USE_LIBBAMBU=no;
panda_USE_LIB_LOGIC_SYNTHESIS=no;
panda_USE_LIB_LOGIC_SYNTHESIS_FLOW=no;
panda_USE_LIB_LOOPS=no;
panda_USE_LIB_MAPPING=no;
panda_USE_LIB_METRIC_PARTITIONING=no;
panda_USE_LIB_METRICS=no;
panda_USE_LIB_OBJECT_CODE_WRAPPER=no;
panda_USE_LIB_PARSER=no;
panda_USE_LIB_PARTITIONING=no;
panda_USE_LIB_PLACEMENT=no;
panda_USE_LIB_POLIXML=no;
panda_USE_LIB_PRAGMA=no;
panda_USE_LIB_PROBABILITY_DISTRIBUTION=no;
panda_USE_LIB_REGRESSORS=no;
panda_USE_LIB_REGULARITY=no;
panda_USE_LIB_RTL=no;
panda_USE_LIB_RTL_METRICS=no;
panda_USE_LIB_RTL_CHARACTERIZATION=no;
panda_USE_LIB_SDF_GRAPHS=no;
panda_USE_LIB_SETHELPER=no;
panda_USE_LIB_SIMULATION_WRAPPER=no;
panda_USE_LIB_STATIC_METRICS=no;
panda_USE_LIB_TARGET=no;
panda_USE_LIB_TASK_GRAPHS=no;
panda_USE_LIB_TECHNOLOGY=no;
panda_USE_LIB_TECHNOLOGY_MAPPING=no;
panda_USE_LIB_TECHNOLOGY_CHARACTERIZATION=no;
panda_USE_LIB_TARGET_PROFILING=no;
panda_USE_LIB_TO_C=no;
panda_USE_LIB_TO_DATAFILE=no;
panda_USE_LIB_TREE=no;
panda_USE_LIB_TREE_MANIPULATION=no;
panda_USE_LIB_UTILITY=no;
panda_USE_LIB_WRAPPER=no;

panda_USE_LIB_VCD_PARSER=no;
panda_USE_LIB_VCD=no;

boost_USE_FILESYSTEM=no;
panda_USE_LIB_2LEV=no;
panda_USE_SPARC_ELF_GCC=no;

panda_USE_LIB_SYNTHESIS_WRAPPER=no;

panda_USE_LIB_ILP=no;
panda_USE_LIBBDD=no

MIN_GCC_VERSION=0.0.0
MAX_GCC_VERSION=9.9.9

MIN_CLANG_VERSION=0.0.0
MAX_CLANG_VERSION=100.9.9


################################################################################
#                                                                              #
#                Implicated by configure options                               #
#                                                                              #
################################################################################
if test "x$panda_USE_ALL" = xyes; then
  panda_USE_BAMBU=yes;
  panda_USE_EUCALYPTUS=yes;
  panda_USE_GECCO=yes;
  panda_USE_GRIZZLY=yes;
  panda_USE_KOALA=yes;
  panda_USE_LEMUR=yes;
  panda_USE_MONGOOSE=yes;
  panda_USE_SPIDER=yes;
  panda_USE_TUCANO=yes;
  panda_USE_TURTLE=yes;
  panda_USE_ZEBU=yes;
fi

if test "x$panda_USE_TUCANO" != xyes && test "x$panda_USE_GECCO" != xyes && test "x$panda_USE_ZEBU" != xyes && test "x$panda_USE_LEMUR" != xyes && test "x$panda_USE_BAMBU" != xyes && test "x$panda_USE_KOALA" != xyes && test "x$panda_USE_GRIZZLY" != xyes && test "x$panda_USE_SPIDER" != xyes && test "x$panda_USE_MONGOOSE" != xyes && test "x$panda_USE_EUCALYPTUS" != xyes; then
  echo "";
  echo "      ERROR: No tools selected. Please configure with --enable-<tool> or --enable-all"; 
  echo "";
  exit 1;
fi

if test "x$panda_WERROR" != xno && test "x$panda_USE_TUCANO" != xyes && test "x$panda_USE_KOALA" != xyes && test "x$panda_USE_LEMUR" != xyes && test "x$panda_USE_GRIZZLY" != xyes && test "x$GIT_PRIVATE_PANDA" == xyes; then
  panda_WERROR=yes;
fi




################################################################################
#                                                                              #
#                            Tool implications                                 #
#                                                                              #
################################################################################

if test "x$panda_USE_EXPERIMENTAL" = xyes; then
    panda_RELEASE=no;
fi

################ Bambu checking ##############################
if test "x$panda_USE_BAMBU" = xyes; then
  panda_USE_BAMBU_RESULTS_XML=yes;
  panda_USE_GLPK=yes;
  panda_USE_LIB_GRAPH=yes;
  panda_USE_LIB_FROM_C=yes;
  panda_USE_I386_GCC=yes;
  AS_VERSION_COMPARE($MIN_GCC_VERSION, [4.5.0], [MIN_GCC_VERSION=4.5.0],,)
  AS_VERSION_COMPARE($MAX_GCC_VERSION, [9.0.0],,,[MAX_GCC_VERSION=9.0.0])
  AS_VERSION_COMPARE($MIN_CLANG_VERSION, [4.0.0], [MIN_CLANG_VERSION=4.0.0],,)
  AS_VERSION_COMPARE($MAX_CLANG_VERSION, [12.0.0],,,[MAX_CLANG_VERSION=12.0.0])
  panda_USE_LIB_HLS=yes;
  panda_USE_LIB_HOST_PROFILING=yes;
  panda_USE_LIB_TECHNOLOGY=yes;
  panda_USE_LIB_TO_C=yes;
  panda_USE_LIB_TO_HDL=yes;
  panda_USE_PARAMETER=yes;
  panda_USE_LIB_PRAGMA=yes;
  panda_USE_LIB_FROM_PRAGMA=yes;
  panda_USE_LIB_GRAPHHELPER=yes;
  panda_USE_LIB_CMOS=yes;
  panda_USE_LIB_SIMULATION_WRAPPER=yes;
  panda_USE_LIB_SYNTHESIS_WRAPPER=yes;
  panda_USE_LIB_VCD_PARSER=yes;
  panda_USE_LIB_VCD=yes;
  panda_USE_LIBBAMBU=yes;
  panda_USE_TREE_PANDA_GCC=yes;
  panda_USE_SPIDER=yes;
  panda_USE_TASTE=yes;
  panda_USE_ABSEIL=yes;
  panda_USE_GENERIC_BASHBACKEND=yes;

  #characterization requires to configure at least one synthesis toolchain
  if test "x$panda_USE_GENERIC_BASHBACKEND" = xyes || test "x$panda_USE_DESIGN_COMPILER" = xyes || test "x$panda_USE_WRAPPER_XILINX" = xyes || test "x$panda_USE_WRAPPER_ALTERA" = xyes || test "x$panda_USE_WRAPPER_LATTICE" = xyes || test "x$panda_USE_WRAPPER_NANOXPLORE" = xyes; then
    AC_APPEND(ENABLE_DOC,"EUCALYPTUS")
    panda_USE_EUCALYPTUS=yes;
  fi

AC_CHECK_HEADERS([bdd.h fdd.h bvec.h ], [panda_USE_LIBBDD=yes],[ echo "$ac_header not found: Clang/LLVM Point-to analysis will not be enabled"])
  if test "x$panda_USE_LIBBDD" = xyes; then
  AC_DEFINE(HAVE_LIBBDD, 1, "Define if bambu is built")
  fi

###  bambu experimental packages ###
  if test "x$panda_USE_EXPERIMENTAL" = xyes; then
    panda_USE_LIB_CLUSTERING=yes;
    panda_USE_LIB_CODESIGN=yes;
    panda_USE_LIB_TO_DATAFILE=yes;
    panda_USE_LIB_2LEV=yes;
    panda_USE_LIB_FROM_ESTIMATION=yes;
    ### panda_USE_METSLIB=yes;
    panda_USE_TRNG=yes;
    #in experimental mode, the FloPoCo library is activated by default
    if test "x$panda_USE_FLOPOCO" != xno; then
       panda_USE_FLOPOCO=yes;
    fi
  else
    panda_USE_COIN_OR=no;
    panda_USE_CUDD=no;
  fi

  AC_APPEND(ENABLE_DOC,"BAMBU")
  AC_DEFINE(HAVE_BAMBU_BUILT, 1, "Define if bambu is built")

fi
AM_CONDITIONAL(BUILD_BAMBU,       test "x$panda_USE_BAMBU" = xyes)
AM_CONDITIONAL(BUILD_PTAA,        test "x$panda_USE_LIBBDD" = xyes)

################ Eucalyptus checking ##############################
if test "x$panda_USE_EUCALYPTUS" = xyes; then
  panda_USE_ABSEIL=yes;
  panda_USE_LIB_CIRCUIT=yes;
  panda_USE_LIB_DESIGN_FLOWS=yes;
  panda_USE_LIB_TO_HDL=yes;
  panda_USE_LIB_TECHNOLOGY=yes;
  panda_USE_LIB_RTL_CHARACTERIZATION=yes;
  panda_USE_LIB_SYNTHESIS_WRAPPER=yes;
  panda_USE_LIB_UTILITY=yes;
  if test "x$panda_USE_FLOPOCO" != xno; then
     panda_USE_FLOPOCO=yes;
  fi
  AC_DEFINE(HAVE_EUCALYPTUS_BUILT, 1, "Define if eucalyptus is built")
fi
AM_CONDITIONAL(BUILD_EUCALYPTUS, test "x$panda_USE_EUCALYPTUS" = xyes)

################ Gecco checking ##############################
if test "x$panda_USE_GECCO" = xyes; then
  panda_USE_ABSEIL=yes;
  panda_USE_LIB_ARCH=yes;
  panda_USE_LIB_FROM_SDF3=yes;
  panda_USE_LIB_FROM_PSPLIB=yes;
  panda_USE_LIB_FROM_TGFF=yes;
  panda_USE_LIB_GRAPH=yes;
  panda_USE_LIB_GRAPH_PARTITIONING=yes;
  panda_USE_LIB_MAPPING=yes;
  panda_USE_PARAMETER=yes;

  AC_APPEND(ENABLE_DOC,"GECCO")
  AC_DEFINE(HAVE_GECCO_BUILT, 1, "Define if gecco is built")
fi
AM_CONDITIONAL(BUILD_GECCO,    test "x$panda_USE_GECCO" = xyes)


################ Grizzly checking ##############################
if test "x$panda_USE_GRIZZLY" = xyes; then

  panda_USE_ABSEIL=yes;
  panda_USE_LIB_GRAPH=yes;
  panda_USE_LIB_FROM_C=yes;
  panda_USE_LIB_TO_C=yes;
  panda_USE_PARAMETER=yes;

  AC_APPEND(ENABLE_DOC,"GRIZZLY")
  AC_DEFINE(HAVE_GRIZZLY_BUILT, 1, "Define if grizzly is built")
fi
AM_CONDITIONAL(BUILD_GRIZZLY,    test "x$panda_USE_GRIZZLY" = xyes)



################ Koala checking ##############################
if test "x$panda_USE_KOALA" = xyes; then

  panda_USE_ABSEIL=yes;
  panda_USE_LIB_BOOLEAN=yes;
  panda_USE_LIB_CIRCUIT=yes;
  panda_USE_LIB_CMOS=yes;
  if test "x$panda_USE_CUDA" = xyes; then
     panda_USE_LIB_CUDA_FAULT_SIMULATION=yes;
  fi
  panda_USE_LIB_EDIF=yes;
  panda_USE_LIB_FROM_AIGER=yes;
  panda_USE_LIB_FROM_BENCH=yes;
  panda_USE_LIB_FROM_BLIF=yes;
  panda_USE_LIB_TECHNOLOGY_MAPPING=yes;
  panda_USE_LIB_TECHNOLOGY_CHARACTERIZATION=yes;
  panda_USE_LIB_GRAPH=yes;
  panda_USE_LIB_LOGIC_FACTORING=yes;
  panda_USE_LIB_LOGIC_SYNTHESIS_FLOW=yes;
  panda_USE_LIB_PLACEMENT=yes;
  panda_USE_LIB_REGULARITY=yes;
  panda_USE_LIB_SIMULATION_WRAPPER=yes;
  panda_USE_LIB_TARGET=yes;
  panda_USE_LIB_TECHNOLOGY=yes;
  panda_USE_LIB_TO_HDL=yes;
  panda_USE_PARAMETER=yes;

  if test "x$panda_USE_ICARUS" != "xno"; then
    panda_USE_ICARUS=yes;
  fi
  if test "x$panda_USE_VERILATOR" != "xno"; then
    panda_USE_VERILATOR=yes;
  fi
  if test "x$panda_USE_CUDD" != "xno"; then
    panda_USE_CUDD=yes;
  fi
  if test "x$panda_USE_LIB_FROM_LIBERTY" != "xno"; then
    panda_USE_LIB_FROM_LIBERTY=yes;
  fi

  if test -f $TOPSRCDIR/ext/switchcraft/libfactor2.so; then
     panda_USE_LIB_BRIDGE=yes;

     BRIDGE_CPPFLAGS="-I$TOPSRCDIR/ext/switchcraft";
     AC_SUBST(BRIDGE_CPPFLAGS)

     BRIDGE_LIBS="-lfactor2";
     AC_SUBST(BRIDGE_LIBS)

     BRIDGE_LDFLAGS="-L$TOPSRCDIR/ext/switchcraft";
     AC_SUBST(BRIDGE_LDFLAGS)

     AC_DEFINE(HAVE_BRIDGE_BUILT, 1, "Define if switchcraft is integrated")
  else
     echo "";
     echo "      WARNING: libfactor library is missing";
     echo "";
  fi

  if test "x$panda_USE_SYNTHESIS_TOOL" != "xno"; then
    panda_USE_SYNTHESIS_TOOL=yes;
  fi

  AC_APPEND(ENABLE_DOC,"KOALA")
  AC_DEFINE(HAVE_KOALA_BUILT, 1, "Define if koala is built")
fi
AM_CONDITIONAL(BUILD_BRIDGE,     test "x$panda_USE_KOALA" = xyes && test -f $TOPSRCDIR/ext/switchcraft/libfactor2.so)
AM_CONDITIONAL(BUILD_KOALA,      test "x$panda_USE_KOALA" = xyes)
AM_CONDITIONAL(BUILD_CUDA,       test "x$panda_USE_CUDA" = xyes)



################ Lemur checking ##############################
if test "x$panda_USE_LEMUR" = xyes; then
  AC_APPEND(ENABLE_DOC,"LEMUR")
  AC_DEFINE(HAVE_LEMUR_BUILT, 1, "Define if lemur is built")
  panda_USE_ABSEIL=yes;
  panda_USE_LIB_GSL=yes;
fi
AM_CONDITIONAL(BUILD_LEMUR,        test "x$panda_USE_LEMUR" = xyes)



################ Mongoose checking ##############################
if test "x$panda_USE_MONGOOSE" = xyes; then
  AC_APPEND(ENABLE_DOC,"MONGOOSE")
  AC_DEFINE(HAVE_MONGOOSE_BUILT, 1, "Define if mongoose is built")
  panda_USE_ABSEIL=yes;
  panda_USE_LIB_CIRCUIT=yes;
  panda_USE_LIB_CMOS=yes;
  panda_USE_LIB_DESIGN_ANALYSIS=yes;
  panda_USE_LIB_FROM_IPXACT=yes;
  panda_USE_LIB_FROM_LIBERTY=yes;
  panda_USE_LIB_GRAPH=yes;
  panda_USE_LIB_POLIXML=yes;
  panda_USE_LIB_TECHNOLOGY=yes;
  panda_USE_PARAMETER=yes;
  if test "x$panda_USE_SYNTHESIS_TOOL" != "xno" && test "x$panda_USE_DESIGN_COMPILER" != "xno"; then
     panda_USE_DESIGN_COMPILER=yes;
  fi
  if test "x$panda_USE_SYNTHESIS_TOOL" != "xno" && test "x$panda_USE_CORE_TOOLS" != "xno"; then
     panda_USE_CORE_TOOLS=yes;
  fi
fi
AM_CONDITIONAL(BUILD_MONGOOSE, test "x$panda_USE_MONGOOSE" = xyes)



################ Spider checking ##############################
if test "x$panda_USE_SPIDER" = xyes; then
  panda_USE_ABSEIL=yes;
  panda_USE_BAMBU_RESULTS_XML=yes;
  panda_USE_LIB_FROM_DATA_XML=yes;
  panda_USE_LIB_TO_DATAFILE=yes;
  panda_USE_PARAMETER=yes;
  panda_USE_LIB_FROM_DISCREPANCY=yes;
  AC_APPEND(ENABLE_DOC,"SPIDER")
  AC_DEFINE(HAVE_SPIDER_BUILT, 1, "Define if spider is built")
  if test "x$panda_USE_EXPERIMENTAL" = xyes; then
     panda_USE_LIB_EVALUATION=yes;
     panda_USE_LIB_FROM_CSV=yes;
     panda_USE_LIB_FROM_DEEP_PROFILING=yes;
     panda_USE_LIB_RTL=yes;
     panda_USE_LIB_GSL=yes;
     panda_USE_LIB_REGRESSORS=yes;
     panda_USE_SOURCE_CODE_STATISTICS_XML=yes;
     panda_USE_LIB_TREE=yes;
     if test "x$panda_USE_LIB_FROM_LIBERTY" = xyes; then
        panda_USE_LIB_FROM_BOOLEAN=yes;
        panda_USE_LIB_TECHNOLOGY=yes;
     fi
  fi
fi
AM_CONDITIONAL(BUILD_SPIDER,      test "x$panda_USE_SPIDER" = xyes)



################ Tucano checking ##############################
if test "x$panda_USE_TUCANO" = xyes; then

  panda_USE_ABSEIL=yes;
  panda_USE_LIB_GRAPH=yes;
  panda_USE_LIB_FROM_SYSTEMC=yes;
  panda_USE_I386_GCC=yes;
  AS_VERSION_COMPARE($MIN_GCC_VERSION, [4.5.0], [MIN_GCC_VERSION=4.5.0],,)
  AS_VERSION_COMPARE($MAX_GCC_VERSION, [4.6.0],,,[MAX_GCC_VERSION=4.6.0])
  panda_USE_LIB_FROM_C=yes;
  panda_USE_LIB_METRIC_PARTITIONING=yes;
  panda_USE_LIB_RTL_METRICS=yes;
  panda_USE_LIB_TLM=yes;
  panda_USE_PARAMETER=yes;

  AC_APPEND(ENABLE_DOC,"TUCANO")
  AC_DEFINE(HAVE_TUCANO_BUILT, 1, "Define if tucano is built")
fi
AM_CONDITIONAL(BUILD_TUCANO,      test "x$panda_USE_TUCANO" = xyes)



####################### Zebu checking ##############################
if test "x$panda_USE_ZEBU" = xyes; then
  panda_USE_ABSEIL=yes;
  panda_USE_LIB_ARCH=yes;
  panda_USE_LIB_CODE_ESTIMATION=yes;
  panda_USE_LIB_COMPARE_MODELS=yes;
  panda_USE_LIB_DYNAMIC_METRICS=yes;
  panda_USE_LIB_FROM_C=yes;
  panda_USE_I386_GCC=yes;
  AS_VERSION_COMPARE($MIN_GCC_VERSION, [4.5.0], [MIN_GCC_VERSION=4.5.0],,)
  AS_VERSION_COMPARE($MAX_GCC_VERSION, [4.9.0],,,[MAX_GCC_VERSION=4.9.0])
  panda_USE_LIB_FROM_PRAGMA=yes;
  panda_USE_LIB_GRAPH=yes;
  panda_USE_LIB_GRAPH_PARTITIONING=yes;
  panda_USE_LIB_MAPPING=yes;
  panda_USE_LIB_OBJECT_CODE_WRAPPER=yes;
  panda_USE_LIB_PRAGMA=yes;
  panda_USE_LIB_TO_C=yes;
  panda_USE_PARAMETER=yes;
  panda_USE_SOURCE_CODE_STATISTICS_XML=yes;
  panda_USE_TREE_PANDA_GCC=yes;

  AC_APPEND(ENABLE_DOC,"ZEBU")
  AC_DEFINE(HAVE_ZEBU_BUILT, 1, "Define if zebu is built")
fi
AM_CONDITIONAL(BUILD_ZEBU,        test "x$panda_USE_ZEBU" = xyes)



#################### PandA tree gcc checking #######################
if test "x$panda_USE_TREE_PANDA_GCC" = xyes; then
  AC_DEFINE(HAVE_TREE_PANDA_GCC_BUILT, 1, "Define if tree-panda-gcc is built")
fi
AM_CONDITIONAL(BUILD_TREE_PANDA_GCC, test "x$panda_USE_TREE_PANDA_GCC" = xyes)



################ Synthesis tools implications ######################
if test "x$panda_USE_SYNTHESIS_TOOL" = xyes; then

  #ASIC datastructures
  panda_USE_LIB_CMOS=yes;

  #ASIC tools
  #-Synopsys
  panda_USE_DESIGN_COMPILER=yes;
  if test "x$panda_USE_EXPERIMENTAL" = xyes; then
     panda_USE_LIB_FROM_LIBERTY=yes;
     panda_USE_LIBRARY_COMPILER=yes;
     panda_USE_PRIME_TIME=yes;
     panda_USE_FORMALITY=yes;
     #-Cadence
     panda_USE_SOC_ENCOUNTER=yes;
  fi

  if test "x$panda_USE_LIB_LOGIC_SYNTHESIS_FLOW" = xyes; then
    #-Nangate
    panda_USE_LIBRARY_CREATOR=yes;
    panda_USE_DESIGN_OPTIMIZER=yes;
  fi
fi


##################### Library implications #########################

if test "x$panda_USE_LIB_TECHNOLOGY_MAPPING" = xyes; then
  panda_USE_LIB_AIG=yes;
fi

if test "x$panda_USE_GLPK" = xno && test "x$panda_USE_COIN_OR" = xyes; then
   panda_USE_GLPK=yes;
fi

if test "x$panda_USE_GLPK" = xyes || test "x$panda_USE_COIN_OR" = xyes; then
   panda_USE_LIB_ILP=yes;
fi

if test "x$panda_USE_CUDD" = xyes; then
  panda_USE_LIB_BDD=yes;
fi

if test "x$panda_USE_LIB_2LEV" = xyes; then
  if test "x$panda_USE_CUDD" != xyes; then
    panda_USE_LIB_2LEV=no;
  fi
fi

if test "x$panda_USE_LIB_AIG" = xyes && test "x$panda_USE_LIB_BDD" = xno; then
  panda_USE_LIB_AIG=no;
fi

if test "x$panda_USE_LIB_AIG" = xyes; then
  panda_USE_LIB_FROM_BOOLEAN=yes;
  panda_USE_LIB_VIRTUAL_LIBRARY=yes;
fi

if test "x$panda_USE_LIB_EDIF" = xyes; then
  panda_USE_LIB_FROM_EDIF=yes;
fi

if test "x$panda_USE_SIMIT" = xyes || test "x$panda_USE_DIOPSIS" = xyes; then
   panda_USE_ARM_GCC=yes;
fi

if test "x$panda_USE_A3" = xyes || test "x$panda_USE_TSIM" = xyes || test "x$panda_USE_LEON3" = xyes || test "x$panda_USE_MPPB" = xyes; then
   panda_USE_SPARC_GCC=yes;
fi

if test "x$panda_USE_LIB_FROM_C" = xyes && test "x$panda_USE_I386_GCC" != xyes && test "x$panda_USE_ARM_GCC" != xyes && test "x$panda_USE_SPARC_GCC" != xyes; then
   panda_USE_I386_GCC=yes;
fi

if test "x$panda_USE_ARM_GCC" = xyes || test "x$panda_USE_SPARC_GCC" = xyes; then
   panda_USE_I386_GCC=yes;
   panda_USE_LIB_FROM_RTL=yes;
fi

if test "x$panda_USE_LIB_FROM_RTL" = xyes; then
  panda_USE_LIB_RTL=yes;
fi

if test "x$panda_USE_LIB_FROM_SYSTEMC" = xyes; then
  panda_USE_LIB_FROM_C=yes;
  panda_USE_LIB_CIRCUIT=yes;
fi

if test "x$panda_USE_LIB_GRAPH_PARTITIONING" = xyes || test "x$panda_USE_LIB_METRIC_PARTITIONING" = xyes; then
   panda_USE_LIB_PARTITIONING=yes;
fi

if test "x$panda_USE_LIB_CODE_ESTIMATION" = xyes || test "x$panda_USE_LIB_METRICS" = xyes || test "x$panda_USE_LIB_PARTITIONING" = xyes || test "x$panda_USE_LIB_MAPPING" = xyes; then
   panda_USE_LIB_CODESIGN=yes;
fi

if test "x$panda_USE_LIB_CODESIGN" = xyes; then
   panda_USE_LIB_SDF_GRAPHS=yes;
   panda_USE_LIB_TASK_GRAPHS=yes;
fi

if test "x$panda_USE_LIB_TASK_GRAPHS" = xyes; then
   panda_USE_LIB_BSG_HELPERS=yes;
   panda_USE_LIB_DOMINANCE=yes;
   panda_USE_LIB_GRAPHHELPER=yes;
   panda_USE_LIB_SETHELPER=yes;
   panda_USE_LIB_TRANSITIVE_REDUCTION=yes;
fi

if test "x$panda_USE_LIB_HLS" = xyes; then
  panda_USE_LIB_CIRCUIT=yes;
  panda_USE_LIB_CLIQUECOVER=yes;
  panda_USE_LIB_TECHNOLOGY=yes;
fi

if test "x$panda_USE_LIB_FROM_C" = xyes; then
  panda_USE_LIB_BEHAVIOR=yes;
  panda_USE_LIB_FRONTEND_ANALYSIS=yes;
  panda_USE_LIB_LOOPS=yes;
  panda_USE_LIB_DOMINANCE=yes;
  panda_USE_LIB_TREE=yes;
  panda_USE_LIB_TREE_MANIPULATION=yes;
  panda_USE_LIB_TREE_PARSER=yes;
fi

if test "x$panda_USE_LIB_CODE_ESTIMATION" = xyes; then
   panda_USE_LIB_STATIC_METRICS=yes;
   panda_USE_LIB_PROBABILITY_DISTRIBUTION=yes;
fi

if test "x$panda_USE_LIB_SDF_GRAPHS" = xyes; then
   panda_USE_FRACTIONS=yes;
   panda_USE_LIB_PROBABILITY_DISTRIBUTION=yes;
fi

if test "x$panda_USE_LIB_FRONTEND_ANALYSIS" = xyes; then
   panda_USE_LIB_DESIGN_FLOWS=yes;
   panda_USE_LIB_POLIXML=yes;
fi
if test "x$panda_USE_LIB_TECHNOLOGY" = xyes; then
  panda_USE_LIB_PHYSICAL_LIBRARY=yes;
  panda_USE_LIB_PHYSICAL_LIBRARY_MODELS=yes;
  panda_USE_LIB_POLIXML=yes;
  panda_USE_LIB_TARGET=yes;
fi

if test "x$panda_USE_LIB_ARCH" = xyes; then
  panda_USE_LIB_FROM_ARCH=yes;
fi

if test "x$panda_USE_PARAMETER" = xyes; then
  panda_USE_LIB_UTILITY=yes;
  panda_USE_LIB_POLIXML=yes;
fi

if test "x$panda_USE_LIB_POLIXML" = xyes; then
  panda_USE_LIB_FROM_POLIXML=yes;
fi

if test "x$panda_USE_LIB_PLACEMENT" = xyes; then
  panda_USE_LIB_TARGET=yes;
fi

if test "x$panda_USE_SIMIT" = xyes || test "x$panda_USE_DIOPSIS" = xyes || test "x$panda_USE_LEON3" = xyes || test "x$panda_USE_MPPB" = xyes || test "x$panda_USE_TSIM" = xyes || test "x$panda_USE_A3" = xyes; then
  panda_USE_LIB_TARGET_PROFILING=yes;
fi

if test "x$panda_USE_LIB_TARGET_PROFILING" != xyes && test "x$panda_USE_LIB_COMPARE_MODELS" = xyes; then
   panda_USE_LIB_COMPARE_MODELS=no;
fi

if (test "x$panda_USE_LIB_HLS" = xyes && (test "x$panda_USE_GENERIC_BASHBACKEND" = xyes || test "x$panda_USE_WRAPPER_XILINX" = xyes || test "x$panda_USE_WRAPPER_ALTERA" = xyes || test "x$panda_USE_WRAPPER_LATTICE" = xyes || test "x$panda_USE_WRAPPER_NANOXPLORE" = xyes || test "x$panda_USE_DESIGN_COMPILER" = xyes)) then
   panda_USE_LIB_RTL_CHARACTERIZATION=yes;
fi

if test "x$panda_USE_LIB_2LEV" = xyes || test "x$panda_USE_LIB_LOGIC_SYNTHESIS_FLOW" = xyes; then
   panda_USE_LIB_LOGIC_SYNTHESIS=yes;
fi

if test "x$panda_USE_LEON3" = xyes || test "x$panda_USE_TSIM" = xyes; then
   panda_USE_SPARC_ELF_GCC=yes;
fi

if test "x$panda_USE_LIB_MAPPING" = xyes && test "x$panda_USE_LIB_TO_C" = xyes; then
   panda_USE_LIB_PRAGMA=yes;
fi
if test "x$panda_USE_LIB_CODE_ESTIMATION" = xyes && test "x$panda_USE_LIB_FROM_C" = xyes; then
   panda_USE_LIB_HOST_PROFILING=yes;
fi

if test "x$panda_USE_SYNTHESIS_TOOL" = xyes || test "x$panda_USE_CORE_TOOLS" = xyes || test "x$panda_USE_DESIGN_COMPILER" = xyes || test "x$panda_USE_GENERIC_BASHBACKEND" = xyes; then
   panda_USE_LIB_SYNTHESIS_WRAPPER=yes;
fi

if test "x$panda_USE_LIB_SYNTHESIS_WRAPPER" = xyes; then
   panda_USE_LIB_TO_HDL=yes;
fi

if test "x$panda_USE_LIB_REGRESSORS" = xyes || test "x$panda_USE_LIB_CODE_ESTIMATION" = xyes || test "x$panda_USE_LIB_TARGET_PROFILING" = xyes; then
   panda_USE_PERFORMANCE_METRICS_XML=yes;
fi

if test "x$panda_USE_LIB_TO_DATAFILE" = xyes || test "x$panda_USE_LIB_CODE_ESTIMATION" = xyes; then
   panda_USE_WEIGHT_MODELS_XML=yes;
fi

if test "x$panda_USE_LIB_FROM_SOURCE_CODE_STATISTICS" = xyes || test "x$panda_USE_ZEBU" = xyes; then
   panda_USE_SOURCE_CODE_STASTICS_XML=yes;
fi

if test "x$panda_USE_ZEBU" = xyes || test "x$panda_USE_LIB_FROM_PRAGMA" = xyes || test "x$panda_USE_TASTE" = xyes; then
   panda_USE_SOURCE_CODE_MANIPULATION=yes
fi

if test "x$panda_USE_LIB_RTL_CHARACTERIZATION" = xyes || test "x$panda_USE_LIB_TECHNOLOGY_CHARACTERIZATION" = xyes; then
   panda_USE_LIB_LIBRARY_CHARACTERIZATION=yes;
fi

if test "x$panda_USE_LIB_TO_DATAFILE" = xyes || test "x$panda_USE_LIB_DESIGN_FLOWS" = xyes; then
   panda_USE_LIB_GRAPH=yes;
fi

if test "$DISTRO" = "arch"; then
   panda_ALLSTATIC=no;
fi

if test "x$panda_USE_TASTE" = xyes; then
   panda_USE_LIB_FROM_AADL_ASN=yes;
fi

###################### libc++ support #######################
if test "x$panda_clanglibcpp" = xyes; then
  AC_DEFINE(EXTRA_CLANGPP_COMPILER_OPTION, " -stdlib=libc++ ", "Define extra compiler options")
else
  AC_DEFINE(EXTRA_CLANGPP_COMPILER_OPTION, "", "Define extra compiler options")
fi

###################### Release and experimental checking #######################
if test "x$panda_RELEASE" = xyes; then
  AC_DEFINE(RELEASE, 1, "Define if a release is built")
  AC_DEFINE(NPROFILE, 1, "Define if profile is disabled") 
  CXXFLAGS+=" -DNDEBUG "
  CFLAGS+=" -DNDEBUG "
  CXXFLAGS_EXT+=" -DNDEBUG "
  CFLAGS_EXT+=" -DNDEBUG "
  CXXFLAGS_EXTFPLL+=" -DNDEBUG "
  panda_USE_LIB_METRIC_PARTITIONING=no;
  panda_USE_SIMIT=no;
  panda_USE_DIOPSIS=no;
  panda_USE_LEON3=no;
  panda_USE_TSIM=no;
  panda_USE_A3=no;
fi
AM_CONDITIONAL(BUILD_RELEASE, test "x$panda_RELEASE" = xyes)

if test "x$panda_RELEASE" = xyes && test "x$panda_USE_KOALA" = xyes && test "x$panda_USE_LIB_FROM_LIBERTY" != xyes; then
   AC_MSG_ERROR("Koala release needs from liberty");
fi

if test "x$panda_USE_EXPERIMENTAL" = xyes; then
  AC_DEFINE(HAVE_EXPERIMENTAL, 1, "Define if experimental features are added")
fi
AM_CONDITIONAL(BUILD_LIB_EXPERIMENTAL,  test "x$panda_USE_EXPERIMENTAL" = xyes)

if test "x$panda_UNORDERED" = xyes; then
   AC_DEFINE(HAVE_UNORDERED, 1, "Define if unordered data structure are used")
fi
if test "x$panda_ASSERTS" = xyes; then
   AC_DEFINE(HAVE_ASSERTS, 1, "Define if assertions are enabled")
fi



################################################################################
#                                                                              #
#                      External tools and libraries                            #
#                                                                              #
################################################################################

################################### A3 stuff ###################################
if test "x$panda_USE_A3" = xyes; then
  AC_CHECK_A3
fi



################################ ALtera checks #################################
if test "x$panda_USE_WRAPPER_ALTERA" = xyes; then
  AC_CHECK_ALTERA
fi
AM_CONDITIONAL(BUILD_QUARTUS_13, test "x$QUARTUS_13_SETTINGS" != "x")

################################## bfd checks ##################################
if test "x$panda_USE_LIB_OBJECT_CODE_WRAPPER" = xyes; then
  AC_CHECK_LIB([bfd],[bfd_openr],[HAVE_LIB_BFD=yes],[HAVE_LIB_BFD=no])
  if test "x$HAVE_LIB_BFD" != xyes; then
     AC_MSG_ERROR(bfd library not found)
  fi
  AC_CHECK_LIB([iberty],[objalloc_free_block],[HAVE_LIB_IBERTY=yes],[HAVE_LIB_IBERTY=no])
  if test "x$HAVE_LIB_IBERTY" != xyes; then
     AC_MSG_ERROR(iberty library not found)
  fi
  AC_CHECK_LIB([dl],[dlopen],[HAVE_LIB_DL=yes],[HAVE_LIB_DL=no])
  if test "x$HAVE_LIB_DL" != xyes; then
     AC_MSG_ERROR(dl library not found)
  fi
fi

################################# CUDA setup ###################################
if test "x$panda_USE_CUDA" = xyes; then
#---------------------------------------------------------------------------
# Setup CUDA paths
#---------------------------------------------------------------------------
   CUDA_CFLAGS="-I/usr/local/cuda/include"
   CUDA_LIBS="-L/usr/local/cuda/lib -lcuda -lcudart -L$NVIDIA_CUDA_SDK/lib -lcutil"
   NVCC="/usr/local/cuda/bin/nvcc"
   AC_SUBST(CUDA_CFLAGS)
   AC_SUBST(CUDA_LIBS)
   AC_SUBST(NVCC)

   AC_ARG_ENABLE([emu],
     [  --enable-emu    Turn on device emulation for CUDA],
     [case "${enableval}" in
            yes) EMULATION=true;;
            no)  EMULATION=false;;
            *) AC_MSG_ERROR([bad value ${enableval} for --enable-emu]);;
     esac],
     [EMULATION=false]
   )
# ------------------------------------------------------------------------------
# Setup nvcc flags
# ------------------------------------------------------------------------------
   if test x$PANDA_DEBUG = xtrue
   then
     NVCCFLAGS="-g"
   else
     NVCCFLAGS="-O3 -use_fast_math"
   fi
   if test x$EMULATION = xtrue
   then
     NVCCFLAGS+=" -deviceemu"
   fi
   NVCCFLAGS+=" -I$NVIDIA_CUDA_SDK/common/inc"
   AC_SUBST(NVCCFLAGS)

fi



################################## GSL stuff ###################################
if test "x$panda_USE_LIB_GSL" = xyes; then
   AX_PATH_GSL(01.10,,AC_MSG_ERROR(could not find required version of GSL))
fi
AM_CONDITIONAL(BUILD_GSL,  test "x$panda_USE_LIB_GSL" = xyes)


################################ Lattice checks #################################
if test "x$panda_USE_WRAPPER_LATTICE" = xyes; then
  AC_CHECK_LATTICE
fi

################################ NanoXplore checks #################################
if test "x$panda_USE_WRAPPER_NANOXPLORE" = xyes; then
  AC_CHECK_NANOXPLORE
fi


############################# Mentor modelsim stuff ############################
if test "x$panda_USE_MODELSIM" = xyes; then
  AC_CHECK_MENTOR
fi



##################################### MPPB #####################################
if test "x$panda_USE_MPPB" = xyes; then
   AC_CHECK_MPPB
   AC_CHECK_MPPB_ELF
   AC_CHECK_XILINX
fi



####################################### R ######################################
if test "x$panda_USE_R" = xyes; then
   AC_CHECK_R
fi
AM_CONDITIONAL(BUILD_R, test "x$panda_USE_R" = xyes)



############################### Sparc elf compiler #############################
if test "x$panda_USE_SPARC_ELF_GCC" = xyes; then
  AC_CHECK_SPARC_ELF
  AC_DEFINE(HAVE_SPARC_ELF_GCC, 1, "Define if sparc elf compiler is present")
fi


################################# Synthesis tools ##############################
if test "x$panda_USE_DESIGN_COMPILER" = xyes; then
  AC_CHECK_DC
fi


################################## TSIM stuff ##################################
if test "x$panda_USE_TSIM" = xyes; then
  AC_CHECK_TSIM
fi

################################ Icarus checks #################################
if test "x$panda_USE_ICARUS" = xyes; then
  AC_CHECK_ICARUS
fi

################################ Verilator checks #################################
if test "x$panda_USE_VERILATOR" = xyes; then
  AC_CHECK_VERILATOR
fi

################################ Xilinx checks #################################
if test "x$panda_USE_WRAPPER_XILINX" = xyes; then
  AC_CHECK_XILINX
fi



################################################################################
#                                                                              #
#                              ext directory                                   #
#                                                                              #
################################################################################

################################### 32bits #####################################
if test "x$panda_32BITS" != x; then
   CXXFLAGS+=" -m32 "
   CFLAGS+=" -m32 "
   CXXFLAGS_EXT+=" -m32 "
   CFLAGS_EXT+=" -m32 "
   CXXFLAGS_EXTFPLL+=" -m32 "
   LDFLAGS+=" -m32 "
fi

if test ! -d ext ; then
   mkdir ext;
fi


############################### OpenBeagle stuff ###############################
if test "x$panda_USE_BEAGLE" = xyes; then
   AC_APPEND(ENABLE_DOC,"BEAGLE")
   $TOPSRCDIR/etc/macros/panda_config_subdir.sh ext/beagle $ac_configure_args "'CXXFLAGS=${CXXFLAGS_EXT}'" "'CFLAGS=${CFLAGS_EXT}'" "'LDFLAGS=$LDFLAGS'"
   if test $? != 0; then
      AC_MSG_ERROR("Error in beagle configuration")
   fi
   AC_DEFINE(HAVE_BEAGLE, 1, "Define if beagle is used")
   BEAGLE_DIR="$TOPSRCDIR/ext/beagle"
fi
AC_SUBST(BEAGLE_DIR)
AM_CONDITIONAL(BUILD_BEAGLE, test "x$panda_USE_BEAGLE" = xyes)

################################## ARM Check ###################################
if test "x$panda_USE_ARM_GCC" = xyes; then
   AC_CHECK_GCC_ARM_VERSION([4.5.0],[4.6.0])
   AC_DEFINE(HAVE_ARM_COMPILER, 1, "Define if ARM-treegcc is used")
fi
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


################################# SPARC Check ##################################
if test "x$panda_USE_SPARC_GCC" = xyes; then
   AC_CHECK_SPARC_ELF
   AC_CHECK_GCC_SPARC_VERSION([4.5.0],[4.6.0])
   AC_DEFINE(HAVE_SPARC_COMPILER, 1, "Define if SPARC-treegcc is used")
fi
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



################################## CUDD stuff ##################################
if test "x$panda_USE_CUDD" = xyes; then
   AC_APPEND(ENABLE_DOC,"CUDD")

   AC_DEFINE(HAVE_CUDD, 1, "Define if CUDD is used")
   $TOPSRCDIR/etc/macros/panda_config_subdir.sh ext/cudd $ac_configure_args "'CXXFLAGS=${CXXFLAGS_EXT}'" "'CFLAGS=${CFLAGS_EXT}'" "'LDFLAGS=$LDFLAGS'"
   CUDD_CPPFLAGS="-I$TOPSRCDIR/ext/cudd/cudd -I$TOPSRCDIR/ext/cudd/nanotrav -I$TOPSRCDIR/ext/cudd/mtr -I$TOPSRCDIR/ext/cudd/epd -I$TOPSRCDIR/ext/cudd/st -I$TOPSRCDIR/ext/cudd/util -I$TOPSRCDIR/ext/cudd/cplusplus"
   CUDD_LIBS="$BUILDDIR/ext/cudd/cplusplus/libobj.la $BUILDDIR/ext/cudd/cudd/libcudd.la"
fi
AC_SUBST(CUDD_CPPFLAGS)
AC_SUBST(CUDD_LIBS)
AM_CONDITIONAL(BUILD_LIB_CUDD, test "x$panda_USE_CUDD" = xyes)


################ FloPoCo stuff #############################
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
   $TOPSRCDIR/etc/macros/panda_config_subdir.sh ext/libfplll-4.0.3 $ac_configure_args "'CXXFLAGS=${CXXFLAGS_EXTFPLL}'" "'CFLAGS=${CFLAGS_EXT}'" "'LDFLAGS=$LDFLAGS'" --prefix=${prefix}
   if test $? != 0; then
      AC_MSG_ERROR("Error in fplll configuration")
   fi
   cd ext/libfplll-4.0.3
   make
   make install
   if test $? != 0; then
      AC_MSG_ERROR("Error in fplll compilation")
   fi
   cd ../..

   AC_DEFINE(HAVE_SOLLYA, 1, "Define if Sollya is used")
   AC_DEFINE(HAVE_HOTBM, 1, "")
   AC_DEFINE(HAVE_LNS, 1, "LNS available")
   $TOPSRCDIR/etc/macros/panda_config_subdir.sh ext/sollya $ac_configure_args "'CFLAGS=${CFLAGS_EXT}'" "'CXXFLAGS=${CXXFLAGS_EXTFPLL} -I${prefix}/include'" "'LDFLAGS=${LDFLAGS} -L${prefix}/lib'";
   if test $? != 0; then
      AC_MSG_ERROR("Error in sollya configuration")
   fi
   cd ext/sollya
   make
   make install
   if test $? != 0; then
      AC_MSG_ERROR("Error in sollya compilation")
   fi
   cd ../..
   $TOPSRCDIR/etc/macros/panda_config_subdir.sh ext/flopoco $ac_configure_args "'CXXFLAGS=${CXXFLAGS_EXT}'" "'CFLAGS=${CFLAGS_EXT}'" "'LDFLAGS=${LDFLAGS} -L$BUILDDIR/ext/sollya/.libs'"
   if test $? != 0; then
      AC_MSG_ERROR("Error in flopoco configuration")
   fi
   AC_DEFINE(HAVE_FLOPOCO, 1, "Define if flopoco is used")
   FLOPOCO_DIR="$TOPSRCDIR/ext/flopoco"
   FLOPOCO_OBJDIR="$BUILDDIR/ext/flopoco"
fi
AM_CONDITIONAL(BUILD_FLOPOCO, test "x$panda_USE_FLOPOCO" = xyes)
AC_SUBST(FLOPOCO_DIR)
AC_SUBST(FLOPOCO_OBJDIR)


################################# ABSEIL stuff ################################
if test "x$panda_USE_ABSEIL" = xyes; then
   $TOPSRCDIR/etc/macros/panda_config_subdir.sh ext/abseil-cpp $ac_configure_args "'CXXFLAGS=${CXXFLAGS_EXT}'" "'CFLAGS=${CFLAGS_EXT}'" "'LDFLAGS=$LDFLAGS'"
   if test $? != 0; then
      AC_MSG_ERROR("Error in abseil configuration")
   fi
   AC_DEFINE(HAVE_ABSEIL, 1, "Define if abseil is used")
   ABSEIL_DIR="$TOPSRCDIR/ext/abseil-cpp"
   ABSEIL_OBJDIR="ext/abseil-cpp"
fi
AM_CONDITIONAL(BUILD_ABSEIL, test "x$panda_USE_ABSEIL" = xyes)
AC_SUBST(ABSEIL_DIR)

################################# METSlib stuff ################################
if test "x$panda_USE_METSLIB" = xyes; then
   $TOPSRCDIR/etc/macros/panda_config_subdir.sh ext/metslib-0.5.3 $ac_configure_args "'CXXFLAGS=${CXXFLAGS_EXT}'" "'CFLAGS=${CFLAGS_EXT}'" "'LDFLAGS=$LDFLAGS'"
   if test $? != 0; then
      AC_MSG_ERROR("Error in metslib configuration")
   fi
   AC_DEFINE(HAVE_METSLIB, 1, "Define if metslib is used")
   METSLIB_DIR="$TOPSRCDIR/metslib-0.5.3"
   METSLIB_OBJDIR="ext/metslib-0.5.3"
fi
AM_CONDITIONAL(BUILD_METSLIB, test "x$panda_USE_METSLIB" = xyes)


################ TRNG stuff #############################
if test "x$panda_USE_TRNG" = xyes; then
   $TOPSRCDIR/etc/macros/panda_config_subdir.sh ext/trng-4.17 $ac_configure_args "'CXXFLAGS=${CXXFLAGS_EXT}'" "'CFLAGS=${CFLAGS_EXT}'" "'LDFLAGS=$LDFLAGS'"
   if test $? != 0; then
      AC_MSG_ERROR("Error in trng configuration")
   fi
   AC_DEFINE(HAVE_TRNG, 1, "Define if trng is used")
   TRNG_DIR="$TOPSRCDIR/ext/trng-4.17"
   TRNG_OBJDIR="ext/trng-4.17"
fi
AM_CONDITIONAL(BUILD_TRNG, test "x$panda_USE_TRNG" = xyes)


################################# Coin-Or stuff ################################
if test "x$panda_USE_COIN_OR" = xyes; then
   # Checks for libraries.
   AX_BLAS()
   BLAS_LIBS="$BLAS_LIBS"
   AX_LAPACK()
   LAPACK_LIBS="$LAPACK_LIBS"
   AC_CHECK_LIB([bz2],[BZ2_bzReadOpen], [echo "Checking for libbz2... yes"], [echo "Checking for libbz2... no"; AC_MSG_ERROR("libbz2 not found")])
   $TOPSRCDIR/etc/macros/panda_config_subdir.sh ext/Coin-Cbc $ac_configure_args "'CXXFLAGS=${CXXFLAGS_EXT}'" "'CFLAGS=${CFLAGS_EXT}'" "'LDFLAGS=$LDFLAGS'"
   if test $? != 0; then
      AC_MSG_ERROR("Error in Coin-Cbc configuration")
   fi
   AC_DEFINE(HAVE_COIN_OR, 1, "Define if coin-or is used")
   COIN_OR_DIR="$TOPSRCDIR/Coin-Cbc"
   COIN_OR_OBJDIR="ext/Coin-Cbc"
fi
AC_SUBST(COIN_OR_DIR)
AC_SUBST(COIN_OR_OBJDIR)
AC_SUBST(BLAS_LIBS)
AC_SUBST(FLIB)
AC_SUBST(LAPACK_LIBS)
AM_CONDITIONAL(BUILD_COIN_OR, test "x$panda_USE_COIN_OR" = xyes)


################################# Liberty stuff ################################
if test "x$panda_USE_LIB_FROM_LIBERTY" = xyes; then
   $TOPSRCDIR/etc/macros/panda_config_subdir.sh ext/liberty_parser $ac_configure_args "'CXXFLAGS=${CXXFLAGS_EXT}'" "'CFLAGS=${CFLAGS_EXT}'" "'LDFLAGS=$LDFLAGS'"
   if test $? != 0; then
      AC_MSG_ERROR("Error in liberty parser configuration")
   fi
   AC_DEFINE(HAVE_FROM_LIBERTY, 1, "Define if liberty parser is used")
fi
AM_CONDITIONAL(BUILD_LIBERTY_PARSER, test "x$panda_USE_LIB_FROM_LIBERTY" = xyes)


################################# TASTE ################################
if test "x$panda_USE_TASTE" = xyes; then
   AC_DEFINE(HAVE_TASTE, 1, "Define if TASTE has to be used")
   AC_DEFINE_UNQUOTED(GRLIB_DIR, "${prefix}/lib/GRLIB", "Define the GRLIB directory")
fi
AM_CONDITIONAL(BUILD_TASTE, test "x$panda_USE_TASTE" = xyes)
AM_CONDITIONAL(USE_GRLIB, test "x$panda_USE_TASTE" = xyes)


##################################### 2lev #####################################
if test "x$panda_USE_LIB_2LEV" = xyes; then
   $TOPSRCDIR/etc/macros/panda_config_subdir.sh src/logic_synthesis/2lev $ac_configure_args "'CXXFLAGS=${CXXFLAGS_EXT}'" "'CFLAGS=${CFLAGS_EXT}'" "'LDFLAGS=$LDFLAGS'" "'CUDD_CPPFLAGS=$CUDD_CPPFLAGS'" "'CUDD_LIBS=$CUDD_LIBS'"
fi
AM_CONDITIONAL(BUILD_LIB_2LEV, test "x$panda_USE_LIB_2LEV" = xyes)


################################### ABC stuff ##################################
AM_CONDITIONAL(BUILD_ABC,  test "x$panda_USE_ABC" = xyes )


################################## GLPK stuff ##################################
LIB_SUITESPARSECONFIG=
AM_CONDITIONAL(BUILD_GLPK, test "x$panda_USE_GLPK" = xyes)
if test "x$panda_USE_GLPK" = xyes; then
   if test "${DISTRO:0:8}" != "centos_6" && test "$DISTRO" != "ubuntu_12.04"; then
      LIB_SUITESPARSECONFIG=-lsuitesparseconfig
      AC_CHECK_LIB([suitesparseconfig],[SuiteSparse_malloc], [echo "Checking for libsuitesparseconfig... yes"], [echo "Checking for libsuitesparseconfig... no"; AC_MSG_ERROR("libsuitesparseconfig not found")])
   fi
   AC_CHECK_LIB([amd],[amd_defaults], [echo "Checking for libamd... yes"], [echo "Checking for libamd... no"; AC_MSG_ERROR("libamd not found")])
   AC_CHECK_LIB([colamd],[symamd], [echo "Checking for libcolamd... yes"], [echo "Checking for libcolamd... no"; AC_MSG_ERROR("libcolamd not found")], [${LIB_SUITESPARSECONFIG}])
   AC_CHECK_LIB([ltdl],[lt_dlerror], [echo "Checking for libltdl... yes"], [echo "Checking for libltd... no"; AC_MSG_ERROR("libltdl not found")])
   AC_CHECK_LIB([glpk],[glp_simplex], [echo "Checking for libglpk... yes"], [echo "Checking for libglpk... no"; AC_MSG_ERROR("libglpk not found")], [-lamd -lcolamd -lltdl -lgmp ${LIB_SUITESPARSECONFIG}])
   AC_CHECK_HEADER([glpk.h],GLPK_NESTED_DIR=0,GLPK_NESTED_DIR=-1)
   if test "x$GLPK_NESTED_DIR" != "x0"; then
      AC_CHECK_HEADER([glpk/glpk.h],GLPK_NESTED_DIR=1, AC_MSG_ERROR("glpk.h not found"))
   fi
   AC_DEFINE_UNQUOTED(HAVE_GLPK_NESTED, ${GLPK_NESTED_DIR}, "")
   AC_DEFINE(HAVE_GLPK, 1, "Define if glpk is used")
fi
AC_SUBST(LIB_SUITESPARSECONFIG)





################################ LP-SOLVE stuff ################################
if test "x$panda_USE_LP_SOLVE" = xyes; then
  USE_BFP_NAME=LUSOL
  #etaPFI
  #etaPFI2
  #GLPK
  #LUSOL
  if test ! -f $TOPSRCDIR/ext/lpsolve5/lp_solve_USE_$USE_BFP_NAME; then
     rm -f $TOPSRCDIR/ext/lpsolve5/lp_solve_USE_*
     touch $TOPSRCDIR/ext/lpsolve5/lp_solve_USE_$USE_BFP_NAME
     if test -d  ext/lpsolve5/bfp; then
       (cd ext/lpsolve5/bfp; make clean;)
     fi
     touch $TOPSRCDIR/ext/lpsolve5/configure.in
   fi
   if test $USE_BFP_NAME == etaPFI2; then
      BFP_NAME="bfp_etaPFI"
   else
      BFP_NAME="bfp_$USE_BFP_NAME"
   fi
   $TOPSRCDIR/etc/macros/panda_config_subdir.sh ext/lpsolve5 $ac_configure_args "'CXXFLAGS=${CXXFLAGS_EXT}'" "'CFLAGS=${CFLAGS_EXT}'" "'LDFLAGS=$LDFLAGS'"
   AC_DEFINE(HAVE_LP_SOLVE, 1, "Define if lp solver is used")
   LP_SOLVE_DIR="$TOPSRCDIR/lpsolve5"
fi
AC_SUBST(BFP_NAME)
AC_SUBST(LP_SOLVE_DIR)
AM_CONDITIONAL(BUILD_LP_SOLVE, test "x$panda_USE_LP_SOLVE" = xyes)



################################# Nodelib stuff ################################
AM_CONDITIONAL(BUILD_NODELIB, test "x$panda_USE_NODELIB" = xyes)


############################### RapidMiner stuff ###############################
AM_CONDITIONAL(BUILD_RAPIDMINER, test "x$panda_USE_RAPIDMINER" = xyes)


################################## SimIt stuff #################################
AM_CONDITIONAL(BUILD_SIMIT, test "x$panda_USE_SIMIT" = xyes)



############################## VFlib library stuff #############################
if test "x$panda_USE_VFLIB" = xyes; then
   $TOPSRCDIR/etc/macros/panda_config_subdir.sh ext/vflib $ac_configure_args "'CXXFLAGS=${CXXFLAGS_EXT}'" "'CFLAGS=${CFLAGS_EXT}'" "'LDFLAGS=$LDFLAGS'"
   VFLIB_DIR="$TOPSRCDIR/ext/vflib"
fi
AM_CONDITIONAL(BUILD_VFLIB, test "x$panda_USE_VFLIB" = xyes)
AC_SUBST(VFLIB_DIR)



#################################### Ext Libraries #############################
if test test "x$ac_cv_cxx_compile_cxx17_native" = xyes || test "x$ac_cv_cxx_compile_cxx17_cxx" = xyes || test "x$ac_cv_cxx_compile_cxx17_gxx" = xyes || test "x$ac_cv_cxx_compile_cxx17_cxx1z" = xyes || "x$panda_USE_ABSEIL" = xyes || test "x$panda_USE_BEAGLE" = xyes || test "x$panda_USE_COIN_OR" = xyes || test "x$panda_USE_CROSSTOOL_NG" = xyes || test "x$panda_USE_CUDD" = xyes || test "x$panda_USE_FLOPOCO" = xyes || test "x$panda_USE_LIB_FROM_LIBERTY" = xyes || test "x$panda_USE_LP_SOLVE" = xyes || test "x$panda_USE_NODELIB" = xyes || test "x$panda_USE_R" = xyes || test "x$panda_USE_RAPIDMINER" = xyes || test "x$panda_USE_SIMIT" = xyes || test "x$panda_USE_VFLIB" = xyes || test "x$build_ARM_GCC" || test "x$build_SPARC_GCC" = xyes || test "x$panda_USE_TRNG" = xyes; then
  panda_USE_LIB_EXT=yes;
fi
AM_CONDITIONAL(BUILD_EXT, test "x$panda_USE_LIB_EXT" = xyes)



################################################################################
#                                                                              #
#                              .                                               #
#                                                                              #
################################################################################

################ Makefile creation #############################################
AC_CONFIG_FILES(
                 Makefile
                 panda_regressions/Makefile
                 panda_regressions/hls/Makefile
                 panda_unit/Makefile
                 examples/Makefile
                 documentation/Makefile
                 documentation/img/Makefile
                 src/Makefile
                 style/Makefile
                 style/stylesheet.css
                 style/img/Makefile
                 style/img/panda.png
                 style/img/logoPoli.gif
                 Appimage/Makefile
               )

################################################################################
#                                                                              #
#                              src/algorithms                                  #
#                                                                              #
################################################################################

if test "x$panda_USE_LIB_BOOLEAN" = xyes; then
  AC_DEFINE(HAVE_BOOLEAN_BUILT, 1, "")
fi
AM_CONDITIONAL(BUILD_LIB_BOOLEAN,  test "x$panda_USE_LIB_BOOLEAN" = xyes)

if test "x$panda_USE_LIB_BSG_HELPERS" = xyes; then
  AC_DEFINE(HAVE_BSG_HELPERS_BUILT, 1, "Define if bsg_helpers library is built")
fi
AM_CONDITIONAL(BUILD_LIB_BSG_HELPERS, test "x$panda_USE_LIB_BSG_HELPERS" = xyes)

AM_CONDITIONAL(BUILD_LIB_CLIQUECOVER,  test "x$panda_USE_LIB_CLIQUECOVER" = xyes)

AM_CONDITIONAL(BUILD_LIB_CLUSTERING,  test "x$panda_USE_LIB_CLUSTERING" = xyes)

AM_CONDITIONAL(BUILD_LIB_DOMINANCE,  test "x$panda_USE_LIB_DOMINANCE" = xyes)

AM_CONDITIONAL(BUILD_LIB_EVALUATION,  test "x$panda_USE_LIB_EVALUATION" = xyes)

############################# transitive reduction #############################
if test "x$panda_USE_LIB_TRANSITIVE_REDUCTION" = xyes; then
  AC_DEFINE(HAVE_TRANSITIVE_REDUCTION_BUILT, 1, "Define if transitive reduction library is built")
fi
AM_CONDITIONAL(BUILD_LIB_TRANSITIVE_REDUCTION,  test "x$panda_USE_LIB_TRANSITIVE_REDUCTION" = xyes)

AM_CONDITIONAL(BUILD_LIB_GRAPHHELPER,  test "x$panda_USE_LIB_GRAPHHELPER" = xyes)

AM_CONDITIONAL(BUILD_LIB_LOOPS,  test "x$panda_USE_LIB_LOOPS" = xyes)

AM_CONDITIONAL(BUILD_LIB_SETHELPER,  test "x$panda_USE_LIB_SETHELPER" = xyes)

if test "x$panda_USE_LIB_REGRESSORS" = xyes; then
  AC_DEFINE(HAVE_REGRESSORS_BUILT, 1, "Define if regressors library is built")
fi
AM_CONDITIONAL(BUILD_LIB_REGRESSORS,  test "x$panda_USE_LIB_REGRESSORS" = xyes)


############################### algorithm stuff ################################
if test "x$panda_USE_LIB_CLIQUECOVER" = xyes || test "x$panda_USE_LIB_CLUSTERING" = xyes || test "x$panda_USE_LIB_DOMINANCE" = xyes || test "x$panda_USE_LIB_SETHELPER" = xyes || test "x$panda_USE_LIB_GRAPHHELPER" = xyes || test "x$panda_USE_LIB_BOOLEAN" = xyes || test "x$panda_USE_LIB_LOOPS" = xyes || test "x$panda_USE_LIB_EVALUATION" = xyes || test "x$panda_USE_VFLIB" = xyes; then
  panda_USE_LIB_ALGORITHMS=yes;
fi
AM_CONDITIONAL(BUILD_LIB_ALGORITHMS,  test "x$panda_USE_LIB_ALGORITHMS" = xyes)


################################################################################
#                                                                              #
#                              src/bdd                                         #
#                                                                              #
################################################################################

################ CUDD stuff ####################################################
if test "x$panda_USE_LIB_BDD" = xyes; then
    AC_DEFINE(HAVE_BDD_BUILT, 1, "Define if BBD library is built")
fi;
AM_CONDITIONAL(BUILD_LIB_BDD, test "x$panda_USE_LIB_BDD" = xyes)



################################################################################
#                                                                              #
#                              src/behavior                                    #
#                                                                              #
################################################################################

################################ behavior stuff ################################
if test "x$panda_USE_LIB_BEHAVIOR" = xyes; then
  AC_DEFINE(HAVE_BEHAVIOR_BUILT, 1, "Define if behavior library is built")
fi
AM_CONDITIONAL(BUILD_LIB_BEHAVIOR, test "x$panda_USE_LIB_BEHAVIOR" = xyes)

############################# host profiling stuff #############################
if test "x$panda_USE_LIB_HOST_PROFILING" = xyes; then
   AC_DEFINE(HAVE_HOST_PROFILING_BUILT, 1, "Define if host profiling library is built")
fi
AM_CONDITIONAL(BUILD_LIB_HOST_PROFILING, test "x$panda_USE_LIB_HOST_PROFILING" = xyes)


################################################################################
#                                                                              #
#                              src/circuit                                     #
#                                                                              #
################################################################################

################ AIG stuff #########################
if test "x$panda_USE_LIB_AIG" = xyes; then
  AC_DEFINE(HAVE_AIG_BUILT, 1, "Define if AIG library is built")
fi
AM_CONDITIONAL(BUILD_LIB_AIG,  test "x$panda_USE_LIB_AIG" = xyes)

################ Edif stuff #########################
AM_CONDITIONAL(BUILD_LIB_EDIF,  test "x$panda_USE_LIB_EDIF" = xyes)

################ Circuit stuff #########################
if test "x$panda_USE_LIB_CIRCUIT" = xyes; then
  AC_DEFINE(HAVE_CIRCUIT_BUILT, 1, "Define if circuit library is built")
fi
AM_CONDITIONAL(BUILD_LIB_FROM_SYSTEMC, test "x$panda_USE_LIB_FROM_SYSTEMC" = xyes)
AM_CONDITIONAL(BUILD_LIB_CIRCUIT,  test "x$panda_USE_LIB_CIRCUIT" = xyes)

################ CudaFaultSim stuff #########################
if test "x$panda_USE_LIB_CUDA_FAULT_SIMULATION" = xyes; then
  AC_DEFINE(HAVE_CUDA_FAULT_SIMULATION_BUILT, 1, "Define if cuda fault simulator library is built")
fi
AM_CONDITIONAL(BUILD_LIB_CUDA_FAULT_SIMULATION,  test "x$panda_USE_LIB_CUDA_FAULT_SIMULATION" = xyes)



################################################################################
#                                                                              #
#                              src/constants                                   #
#                                                                              #
################################################################################
if test "x$panda_USE_BAMBU_RESULTS_XML" = xyes; then
   AC_DEFINE(HAVE_BAMBU_RESULTS_XML, 1, "Define if bambu results xmls are used")
fi

if test "x$panda_USE_PERFORMANCE_METRICS_XML" = xyes; then
   AC_DEFINE(HAVE_PERFORMANCE_METRICS_XML, 1, "Define if performance metrics xmls are used")
fi
AM_CONDITIONAL(USE_PERFORMANCE_METRICS_XML, test "x$panda_USE_PERFORMANCE_METRICS_XML" = xyes)

if test "x$panda_USE_SOURCE_CODE_STATISTICS_XML" = xyes; then
   AC_DEFINE(HAVE_SOURCE_CODE_STATISTICS_XML, 1, "Define if source code statistics xmls are used")
fi
AM_CONDITIONAL(USE_SOURCE_CODE_STATISTICS_XML, test "x$panda_USE_SOURCE_CODE_STATISTICS_XML" = xyes)

if test "x$panda_USE_WEIGHT_MODELS_XML" = xyes; then
   AC_DEFINE(HAVE_WEIGHT_MODELS_XML, 1, "Define if weight models xmls are used")
fi
AM_CONDITIONAL(USE_WEIGHT_MODELS_XML, test "x$panda_USE_WEIGHT_MODELS_XML" = xyes)

AM_CONDITIONAL(USE_CONSTANTS, test "x$panda_USE_CONSTANTS" = xyes)




################################################################################
#                                                                              #
#                          src/design_analysis                                 #
#                                                                              #
################################################################################

################ Design analysis stuff #########################################
if test "x$panda_USE_LIB_DESIGN_ANALYSIS" = xyes; then

  AC_DEFINE(HAVE_DESIGN_ANALYSIS_BUILT, 1, "Define if design analysis library is built")
fi
AM_CONDITIONAL(BUILD_LIB_DESIGN_ANALYSIS, test "x$panda_USE_LIB_DESIGN_ANALYSIS" = xyes)



################################################################################
#                                                                              #
#                            src/design_flows                                  #
#                                                                              #
################################################################################

################################################################################
#                                                                              #
#                        src/design_flows/backend                              #
#                                                                              #
################################################################################

################ Backend to C #########################
if test "x$panda_USE_LIB_TO_C" = xyes; then
  AC_DEFINE(HAVE_TO_C_BUILT, 1, "Define if C backend library is created")
fi
AM_CONDITIONAL(BUILD_LIB_TO_C,  test "x$panda_USE_LIB_TO_C" = xyes)

################ Backend to DataFile ###########################################
if test "x$panda_USE_LIB_TO_DATAFILE" = xyes; then
  AC_DEFINE(HAVE_TO_DATAFILE_BUILT, 1, "Define if backend datafile library is built")
fi
AM_CONDITIONAL(BUILD_LIB_TO_DATAFILE, test "x$panda_USE_LIB_TO_DATAFILE" = xyes)

################ Backend to HDL #########################
if test "x$panda_USE_LIB_TO_HDL" = xyes; then
  AC_DEFINE(HAVE_TO_HDL_BUILT, 1, "Define if HDL backend library is built")
fi
AM_CONDITIONAL(BUILD_LIB_TO_HDL,  test "x$panda_USE_LIB_TO_HDL" = xyes)

################ Backend stuff #########################
if test "x$panda_USE_LIB_TO_C" = xyes || test "x$panda_USE_LIB_TO_HDL" = xyes || test "x$panda_USE_LIB_TO_DATAFILE" = xyes; then
   panda_USE_LIB_BACKEND=yes;
fi
AM_CONDITIONAL(BUILD_LIB_BACKEND,  test "x$panda_USE_LIB_BACKEND" = xyes)



################################################################################
#                                                                              #
#                        src/design_flows/codesign                             #
#                                                                              #
################################################################################

############################# TaskGraph estimation #############################
AM_CONDITIONAL(BUILD_LIB_CODE_ESTIMATION, test "x$panda_USE_LIB_CODE_ESTIMATION" = xyes)

################ Mapping stuff #########################
if test "x$panda_USE_LIB_MAPPING" = xyes; then
  AC_DEFINE(HAVE_MAPPING_BUILT, 1, "Define if mapping library is built")
fi
AM_CONDITIONAL(BUILD_LIB_MAPPING,  test "x$panda_USE_LIB_MAPPING" = xyes)

################ Compare Models Metric stuff ######################
if test "x$panda_USE_LIB_COMPARE_MODELS" = xyes; then
  AC_DEFINE(HAVE_COMPARE_MODELS_BUILT, 1, "Define if compare models library is built")
fi
AM_CONDITIONAL(BUILD_LIB_COMPARE_MODELS,  test "x$panda_USE_LIB_COMPARE_MODELS" = xyes)

if test "x$panda_USE_WEIGHT_MODELS_XML" = xyes; then
   AC_DEFINE(HAVE_WEIGHT_MODELS_XML, 1, "Define if weight models xmls are used")
fi
AM_CONDITIONAL(USE_WEIGHT_MODELS_XML, test "x$panda_USE_WEIGHT_MODELS_XML" = xyes)


################ Dynamic Metric stuff #########################
if test "x$panda_USE_LIB_DYNAMIC_METRICS" = xyes; then
  AC_DEFINE(HAVE_DYN_METRIC_BUILT, 1, "Define if dynamic metric library is built")
fi
AM_CONDITIONAL(BUILD_LIB_DYN_METRIC,  test "x$panda_USE_LIB_DYNAMIC_METRICS" = xyes)


################ RTL Metric stuff #########################
AM_CONDITIONAL(BUILD_LIB_RTL_METRICS,  test "x$panda_USE_LIB_RTL_METRICS" = xyes)

################ Static Metric stuff #########################
AM_CONDITIONAL(BUILD_LIB_STATIC_METRIC,  test "x$panda_USE_LIB_STATIC_METRICS" = xyes)

################ TLM Metric stuff #########################
AM_CONDITIONAL(BUILD_LIB_TLM,  test "x$panda_USE_LIB_TLM" = xyes)

################ Metrics stuff #########################
if test "x$panda_USE_LIB_COMPARE_MODELS" = xyes || test "x$panda_USE_LIB_STATIC_METRICS" = xyes || test "x$panda_USE_LIB_DYNAMIC_METRICS" = xyes || test "x$panda_USE_LIB_RTL_METRICS" = xyes || test "x$panda_USE_LIB_TLM" = xyes; then
   panda_USE_LIB_METRICS=yes;
fi
AM_CONDITIONAL(BUILD_LIB_METRICS,  test "x$panda_USE_LIB_METRICS" = xyes)

################################## Algorithm Partitioning stuff ################

############################### SDF graph stuff ################################

################ Graph Partitioning checking ###########################
if test "x$panda_USE_LIB_GRAPH_PARTITIONING" = xyes; then
   AC_DEFINE(HAVE_GRAPH_PARTITIONING_BUILT, 1, "Define if graph partitioning library is built")
fi
AM_CONDITIONAL(BUILD_LIB_GRAPH_PARTITIONING,    test "x$panda_USE_LIB_GRAPH_PARTITIONING" = xyes)

################ Metric Partitioning checking ###########################
if test "x$panda_USE_LIB_METRIC_PARTITIONING" = xyes; then
  AC_DEFINE(HAVE_METRIC_PARTITIONING_BUILT, 1, "Define if metric partitioning library is built")
fi
AM_CONDITIONAL(BUILD_LIB_METRIC_PARTITIONING,   test "x$panda_USE_LIB_METRIC_PARTITIONING" = xyes)

################ Partitioning checking ###########################
if test "x$panda_USE_LIB_PARTITIONING" = xyes; then
  AC_DEFINE(HAVE_PARTITIONING_BUILT, 1, "Define if partitioning library is built")
fi
AM_CONDITIONAL(BUILD_LIB_PARTITIONING,    test "x$panda_USE_LIB_PARTITIONING" = xyes)

############################# Codesign checking ################################
if test "x$panda_USE_LIB_CODESIGN" = xyes; then
  AC_DEFINE(HAVE_CODESIGN, 1, "Define if design_flows/codesign features are added")
fi
AM_CONDITIONAL(BUILD_LIB_CODESIGN,    test "x$panda_USE_LIB_CODESIGN" = xyes)

######################### Design flows #########################################
if test "x$panda_USE_LIB_CODESIGN" = xyes || test "x$panda_USE_LIB_BACKEND" = xyes || test "x$panda_USE_LIB_TECHNOLOGY_MAPPING" = xyes; then
   panda_USE_LIB_DESIGN_FLOWS=yes;
fi
if test "x$panda_USE_LIB_DESIGN_FLOWS" = xyes; then
  AC_DEFINE(HAVE_DESIGN_FLOWS_BUILT, 1, "Define if design flows library is built")
fi
AM_CONDITIONAL(BUILD_LIB_DESIGN_FLOWS, test "x$panda_USE_LIB_DESIGN_FLOWS" = xyes)



################################################################################
#                                                                              #
#                         src/frontend_analysis                                #
#                                                                              #
################################################################################

if test "x$panda_USE_LIB_FRONTEND_ANALYSIS" = xyes; then
  AC_DEFINE(HAVE_FRONTEND_ANALYSIS_BUILT, 1, "Define if frontend analysis library is built")
fi
AM_CONDITIONAL(BUILD_SOURCE_CODE_MANIPULATION, test "x$panda_USE_SOURCE_CODE_MANIPULATION" = xyes)
AM_CONDITIONAL(BUILD_LIB_FRONTEND_ANALYSIS, test "x$panda_USE_LIB_FRONTEND_ANALYSIS" = xyes)



################################################################################
#                                                                              #
#                         src/graph                                            #
#                                                                              #
################################################################################

################ graph stuff #########################
AM_CONDITIONAL(BUILD_LIB_GRAPH,  test "x$panda_USE_LIB_GRAPH" = xyes)


################################################################################
#                                                                              #
#                         src/HLS                                              #
#                                                                              #
################################################################################

#################################### HLS/backend ###############################

############### Bambu ##########################################################
AM_CONDITIONAL(BUILD_DSE,   test "x$panda_USE_BAMBU" = xyes && test "x$panda_USE_BEAGLE" = xyes)

################ HLS stuff #########################
if test "x$panda_USE_LIB_HLS" = xyes; then
  AC_DEFINE(HAVE_HLS_BUILT, 1, "Define if HLS library is built")
  AC_CONFIG_FILES(
                   src/HLS/Makefile
                   src/HLS/simulation/Makefile
                 )

fi
AM_CONDITIONAL(BUILD_LIB_HLS,  test "x$panda_USE_LIB_HLS" = xyes)


################################################################################
#                                                                              #
#                              src/ilp                                         #
#                                                                              #
################################################################################

################ ilp stuff #########################
if test "x$panda_USE_LIB_ILP" = xyes; then
  AC_DEFINE(HAVE_ILP_BUILT, 1, "Define if ILP library is built")
fi
AM_CONDITIONAL(BUILD_LIB_ILP,  test "x$panda_USE_LIB_ILP" = xyes)


################################################################################
#                                                                              #
#                              src/logic_synthesis                             #
#                                                                              #
################################################################################

################ Regularity Extraction stuff #########################
AM_CONDITIONAL(BUILD_LIB_REGULARITY,  test "x$panda_USE_LIB_REGULARITY" = xyes)

################ Technology mapping stuff #########################
if test "x$panda_USE_LIB_TECHNOLOGY_MAPPING" = xyes; then
  AC_DEFINE(HAVE_TECHNOLOGY_MAPPING_BUILT, 1, "Define if technology mapping library is built")
fi

AM_CONDITIONAL(BUILD_LIB_TECHNOLOGY_MAPPING, test "x$panda_USE_LIB_TECHNOLOGY_MAPPING" = xyes)

############################### testing stuff ##################################
AM_CONDITIONAL(BUILD_LIB_FAULTS,           test "x$panda_USE_LIB_FAULTS" = xyes)

################ Logic factoring #########################
if test "x$panda_USE_LIB_LOGIC_FACTORING" = xyes; then
  AC_DEFINE(HAVE_LOGIC_FACTORING, 1, "Define if the logic factoring package has to be built")
fi
AM_CONDITIONAL(BUILD_LIB_LOGIC_FACTORING, test "x$panda_USE_LIB_LOGIC_FACTORING" = xyes)

################ Logic synthesis stuff #########################
if test "x$panda_USE_LIB_LOGIC_SYNTHESIS_FLOW" = xyes; then
  AC_DEFINE(HAVE_LOGIC_SYNTHESIS_FLOW_BUILT, 1, "Define if logic synthesis flow is built")
fi
AM_CONDITIONAL(BUILD_LIB_LOGIC_SYNTHESIS_FLOW,  test "x$panda_USE_LIB_LOGIC_SYNTHESIS_FLOW" = xyes)

################################ Logic Synthesis ###############################
if test "x$panda_USE_LIB_2LEV" = xyes || test "x$panda_USE_LIB_LOGIC_SYNTHESIS_FLOW" = xyes; then
   AC_DEFINE(HAVE_LOGIC_SYNTHESIS_BUILT, 1, "Define if logic synthesis package is built")
fi
AM_CONDITIONAL(BUILD_LIB_LOGIC_SYNTHESIS, test "x$panda_USE_LIB_LOGIC_SYNTHESIS" = xyes)



################################################################################
#                                                                              #
#                     src/intermediate_representations                         #
#                                                                              #
################################################################################


################################## sdf_graphs ##################################
if test "x$panda_USE_LIB_SDF_GRAPHS" = xyes; then
   AC_DEFINE(HAVE_SDF_GRAPHS_BUILT, 1, "Define if sdf graph library is built")
fi
AM_CONDITIONAL(BUILD_LIB_SDF_GRAPHS, test "x$panda_USE_LIB_SDF_GRAPHS" = xyes)

################################# task_graphs ##################################
if test "x$panda_USE_LIB_TASK_GRAPHS" = xyes; then
   AC_DEFINE(HAVE_TASK_GRAPHS_BUILT, 1, "Define if taskgraph library is built")
fi
AM_CONDITIONAL(BUILD_LIB_TASK_GRAPHS, test "x$panda_USE_LIB_TASK_GRAPHS" = xyes)

################################# actor graphs ################################
if test "x$panda_USE_LIB_TASK_GRAPHS" = xyes || test "x$panda_USE_LIB_SDF_GRAPHS" = xyes; then
   panda_USE_LIB_ACTOR_GRAPHS=yes;
fi
if test "x$panda_USE_LIB_ACTOR_GRAPHS" = xyes; then
   AC_DEFINE(HAVE_ACTOR_GRAPHS_BUILT, 1, "Define if actor graphs library is built")
fi
AM_CONDITIONAL(BUILD_LIB_ACTOR_GRAPHS, test "x$panda_USE_LIB_ACTOR_GRAPHS" = xyes)

############################## mapping annotations #############################

####################### intermediate_representations  ##########################
if test "x$panda_USE_LIB_FROM_AADL_ASN" = xyes || test "x$panda_USE_LIB_ACTOR_GRAPHS" = xyes || test "x$panda_USE_LIB_MAPPING" = xyes || test "x$panda_USE_LIB_HLS" = xyes; then
   panda_USE_LIB_INTERMEDIATE_REPRESENTATIONS=yes;
fi
if test "x$panda_USE_LIB_INTERMEDIATE_REPRESENTATIONS" = xyes; then
  AC_DEFINE(HAVE_INTERMEDIATE_REPRESENTATIONS_BUILT, 1, "Define if intermediate representation library is built")
fi
AM_CONDITIONAL(BUILD_LIB_INTERMEDIATE_REPRESENTATIONS, test "x$panda_USE_LIB_INTERMEDIATE_REPRESENTATIONS" = xyes)



################################################################################
#                                                                              #
#                         src/ip_xact                                          #
#                                                                              #
################################################################################

############### ip_xact ########################################################
if test "x$panda_USE_LIB_FROM_IPXACT" = xyes; then
  AC_DEFINE(HAVE_IPXACT_BUILT, 1, "Define if ip-xact library is built")
fi



################################################################################
#                                                                              #
#                              src/machine                                     #
#                                                                              #
################################################################################

################ Machine stuff #########################
if test "x$panda_USE_LIB_ARCH" = xyes; then
  AC_DEFINE(HAVE_ARCH_BUILT, 1, "Define if architecture library is built")
fi
AM_CONDITIONAL(BUILD_LIB_ARCH,  test "x$panda_USE_LIB_ARCH" = xyes)



################################################################################
#                                                                              #
#                              src/parser                                      #
#                                                                              #
################################################################################

################################### aadl stuff #################################
if test "x$panda_USE_LIB_FROM_AADL_ASN" = xyes; then
  AC_DEFINE(HAVE_FROM_AADL_ASN_BUILT, 1, "Define if from aadl/asn library is built")
  AC_CONFIG_FILES(
                  src/parser/aadl/Makefile
                  src/parser/asn/Makefile
                 )
fi
AM_CONDITIONAL(BUILD_LIB_FROM_AADL_ASN,  test "x$panda_USE_LIB_FROM_AADL_ASN" = xyes)

################################## AIGER stuff #################################
if test "x$panda_USE_LIB_FROM_AIGER" = xyes; then
  AC_DEFINE(HAVE_FROM_AIGER_BUILT, 1, "Define if from aiger library is built")
fi
AM_CONDITIONAL(BUILD_LIB_FROM_AIGER,  test "x$panda_USE_LIB_FROM_AIGER" = xyes)

################ bench stuff #########################
if test "x$panda_USE_LIB_FROM_BENCH" = xyes; then
  AC_DEFINE(HAVE_FROM_BENCH_BUILT, 1, "Define if from bench library is built")
fi
AM_CONDITIONAL(BUILD_LIB_FROM_BENCH,  test "x$panda_USE_LIB_FROM_BENCH" = xyes)

################ Blif stuff #########################
if test "x$panda_USE_LIB_FROM_BLIF" = xyes; then
   AC_DEFINE(HAVE_FROM_BLIF_BUILT, 1, "Define if from blif library is built")
fi
AM_CONDITIONAL(BUILD_LIB_FROM_BLIF,  test "x$panda_USE_LIB_FROM_BLIF" = xyes)

################ Boolean stuff #########################
if test "x$panda_USE_LIB_FROM_BOOLEAN" = xyes; then
  AC_DEFINE(HAVE_BOOLEAN_PARSER_BUILT, 1, "Define if from boolean library is built")
fi
AM_CONDITIONAL(BUILD_LIB_FROM_BOOLEAN,  test "x$panda_USE_LIB_FROM_BOOLEAN" = xyes)

################################### From CSV ##################################
if test "x$panda_USE_LIB_FROM_CSV" = xyes; then
  AC_DEFINE(HAVE_FROM_CSV_BUILT, 1, "Define if from csv library is built")
fi
AM_CONDITIONAL(BUILD_LIB_FROM_CSV,  test "x$panda_USE_LIB_FROM_CSV" = xyes)

################ Discrepancy stuff #########################
if test "x$panda_USE_LIB_FROM_DISCREPANCY" = xyes; then
  AC_DEFINE(HAVE_FROM_DISCREPANCY_BUILT, 1, "Define from discrepancy library is built")
  AC_CONFIG_FILES(
                  src/parser/discrepancy/Makefile
                  )
fi
AM_CONDITIONAL(BUILD_LIB_FROM_DISCREPANCY,  test "x$panda_USE_LIB_FROM_DISCREPANCY" = xyes)

################ From Edif stuff #########################
if test "x$panda_USE_LIB_FROM_EDIF" = xyes; then
  AC_DEFINE(HAVE_FROM_EDIF_BUILT, 1, "Define from edif library is built")
fi
AM_CONDITIONAL(BUILD_LIB_FROM_EDIF,  test "x$panda_USE_LIB_FROM_EDIF" = xyes)

################ Estimation stuff #########################
if test "x$panda_USE_LIB_FROM_ESTIMATION" = xyes; then
  AC_DEFINE(HAVE_FROM_ESTIMATION_BUILT, 1, "Define from estimation library is built")
fi
AM_CONDITIONAL(BUILD_LIB_FROM_ESTIMATION,  test "x$panda_USE_LIB_FROM_ESTIMATION" = xyes)

############################# From IP-XACT stuff ##############################ù
if test "x$panda_USE_LIB_FROM_IPXACT" = xyes; then
  AC_DEFINE(HAVE_FROM_IPXACT_BUILT, 1, "Define if from ipxact library is built")
fi
AM_CONDITIONAL(BUILD_LIB_FROM_IPXACT,  test "x$panda_USE_LIB_FROM_IPXACT" = xyes)

################ architecture stuff #########################
if test "x$panda_USE_LIB_FROM_ARCH" = xyes; then
  AC_DEFINE(HAVE_FROM_ARCH_BUILT, 1, "Define if from architecture library is built")
fi
AM_CONDITIONAL(BUILD_LIB_FROM_ARCH,  test "x$panda_USE_LIB_FROM_ARCH" = xyes)

################ Polixml stuff #########################
if test "x$panda_USE_LIB_FROM_POLIXML" = xyes; then
  AC_DEFINE(HAVE_FROM_POLIXML_BUILT, 1, "Define if from polixml library is built")
  AC_CONFIG_FILES(
                   src/parser/polixml/Makefile
                 )
fi
AM_CONDITIONAL(BUILD_LIB_FROM_POLIXML,  test "x$panda_USE_LIB_FROM_POLIXML" = xyes)

################ Parsing pragma stuff #########################
if test "x$panda_USE_LIB_FROM_PRAGMA" = xyes; then
  AC_DEFINE(HAVE_FROM_PRAGMA_BUILT, 1, "Define if from pragma library is built")
  AC_CONFIG_FILES(
                   src/parser/pragma/Makefile
                 )
fi
AM_CONDITIONAL(BUILD_LIB_FROM_PRAGMA,  test "x$panda_USE_LIB_FROM_PRAGMA" = xyes)

################ Deep Profiling stuff #########################
if test "x$panda_USE_LIB_FROM_DEEP_PROFILING" = xyes; then
   AC_DEFINE(HAVE_FROM_PROFILING_ANALYIS_BUILT, 1, "Define if from profiling analysis library is built")
fi
AM_CONDITIONAL(BUILD_LIB_FROM_DEEP_PROFILING,  test "x$panda_USE_LIB_FROM_DEEP_PROFILING" = xyes)

################################## PSPLIB stuff ################################
if test "x$panda_USE_LIB_FROM_PSPLIB" = xyes; then
   AC_DEFINE(HAVE_FROM_PSPLIB_BUILT, 1, "Define if psplib parser is built")
fi

AM_CONDITIONAL(BUILD_LIB_FROM_PSPLIB,  test "x$panda_USE_LIB_FROM_PSPLIB" = xyes)

################ From RTL #########################
if test "x$panda_USE_LIB_FROM_RTL" = xyes; then
   AC_DEFINE(HAVE_FROM_RTL_BUILT, 1, "Define if from rtl library is built")
fi
AM_CONDITIONAL(BUILD_LIB_FROM_RTL, test "x$panda_USE_LIB_FROM_RTL" = xyes)

################################## sdf3 stuff ##################################
if test "x$panda_USE_LIB_FROM_SDF3" = xyes; then
   AC_DEFINE(HAVE_FROM_SDF3_BUILT, 1, "Define if sdf3 parser is built")
fi
AM_CONDITIONAL(BUILD_LIB_FROM_SDF3, test "x$panda_USE_LIB_FROM_SDF3" = xyes)

################ source code statistics stuff ##################################
if test "x$panda_USE_LIB_FROM_DATA_XML" = xyes; then
   AC_DEFINE(HAVE_FROM_DATA_XML_BUILT, 1, "Define if from data xml library is built")
   AC_CONFIG_FILES(
                   src/parser/data_xml/Makefile
                 )
fi
AM_CONDITIONAL(BUILD_LIB_FROM_DATA_XML, test "x$panda_USE_LIB_FROM_DATA_XML" = xyes)

################ TGFF stuff #########################
if test "x$panda_USE_LIB_FROM_TGFF" = xyes; then
  AC_DEFINE(HAVE_FROM_TGFF_BUILT, 1, "Define if from tgff library is built")
fi
AM_CONDITIONAL(BUILD_LIB_FROM_TGFF,  test "x$panda_USE_LIB_FROM_TGFF" = xyes)

################ Tree Parser #########################
if test "x$panda_USE_LIB_TREE_PARSER" = xyes; then
  AC_DEFINE(HAVE_TREE_PARSER_BUILT, 1, "Define if from tree parser library is built")
  AC_CONFIG_FILES(
                   src/parser/treegcc/Makefile
                 )
fi
AM_CONDITIONAL(BUILD_LIB_TREE_PARSER, test "x$panda_USE_LIB_TREE_PARSER" = xyes)

################ Vcd Parser #########################
if test "x$panda_USE_LIB_VCD_PARSER" = xyes; then
  AC_DEFINE(HAVE_VCD_PARSER_BUILT, 1, "Define if from vcd parser library is built")
  AC_CONFIG_FILES(
                   src/parser/vcd/Makefile
                 )
fi
AM_CONDITIONAL(BUILD_LIB_VCD_PARSER, test "x$panda_USE_LIB_VCD_PARSER" = xyes)

################ parser stuff #########################
if test "x$panda_USE_FROM_C" = xyes || test "x$panda_USE_LIB_FROM_ESTIMATION" = xyes || test "x$panda_USE_LIB_FROM_IPXACT" = xyes || test "x$panda_USE_LIB_FROM_TGFF" = xyes || test "x$panda_USE_LIB_FROM_PRAGMA" = xyes || test "x$panda_USE_LIB_HLS" = xyes || test "x$panda_USE_LIB_FROM_BENCH" = xyes || test "x$panda_USE_LIB_EDIF" = xyes || test "x$panda_USE_LIB_FROM_ARCH" = xyes || test "x$panda_USE_LIB_POLIXML" = xyes || test "x$panda_USE_LIB_LIBERTY" = xyes || test "x$panda_USE_LIB_FROM_BOOLEAN" = xyes || test "x$panda_USE_LIB_FROM_RTL" = xyes || test "x$panda_USE_LIB_FROM_DEEP_PROFILING" = xyes || test "x$panda_USE_LIB_VCD_PARSER" = xyes; then
   panda_USE_LIB_PARSER=yes;
fi

if test "x$panda_USE_LIB_PARSER" = xyes; then
  AC_CONFIG_FILES(
                   src/parser/Makefile
                 )
fi
AM_CONDITIONAL(BUILD_LIB_PARSER,  test "x$panda_USE_LIB_PARSER" = xyes)



################################################################################
#                                                                              #
#                              src/polixml                                     #
#                                                                              #
################################################################################

################ Polixml stuff #########################
if test "x$panda_USE_LIB_POLIXML" = xyes; then
  AC_DEFINE(HAVE_POLIXML_BUILT, 1, "Define if polixml library is built")
fi
AM_CONDITIONAL(BUILD_LIB_POLIXML,  test "x$panda_USE_LIB_POLIXML" = xyes)



################################################################################
#                                                                              #
#                              src/pragma                                      #
#                                                                              #
################################################################################

################ Pragma stuff #########################
if test "x$panda_USE_LIB_PRAGMA" = xyes; then
  AC_DEFINE(HAVE_PRAGMA_BUILT, 1, "Define if pragma library is built")
fi
AM_CONDITIONAL(BUILD_LIB_PRAGMA,  test "x$panda_USE_LIB_PRAGMA" = xyes)



################################################################################
#                                                                              #
#                              src/rtl                                         #
#                                                                              #
################################################################################

################ RTL #########################
if test "x$panda_USE_LIB_RTL" = xyes; then
  AC_DEFINE(HAVE_RTL_BUILT, 1, "Define if rtl library is built")
fi
AM_CONDITIONAL(BUILD_LIB_RTL,         test "x$panda_USE_LIB_RTL" = xyes)



################################################################################
#                                                                              #
#                              src/technology                                  #
#                                                                              #
################################################################################

if test "x$panda_USE_LIB_RTL_CHARACTERIZATION" = xyes; then
   AC_DEFINE(HAVE_RTL_CHARACTERIZATION_BUILT, 1, "Define if library characterization library is built")
fi
if test "x$panda_USE_LIB_LIBRARY_CHARACTERIZATION" = xyes; then
   AC_DEFINE(HAVE_LIBRARY_CHARACTERIZATION_BUILT, 1, "Define if library characterization library is built")
fi

AM_CONDITIONAL(BUILD_LIB_LIBRARY_CHARACTERIZATION, test "x$panda_USE_LIB_LIBRARY_CHARACTERIZATION" = xyes)
AM_CONDITIONAL(BUILD_LIB_RTL_CHARACTERIZATION, test "x$panda_USE_LIB_RTL_CHARACTERIZATION" = xyes)
AM_CONDITIONAL(BUILD_LIB_TECHNOLOGY_CHARACTERIZATION, test "x$panda_USE_LIB_TECHNOLOGY_CHARACTERIZATION" = xyes)

############################ Physical Library Models ###########################
if test "x$panda_USE_LIB_PHYSICAL_LIBRARY_MODELS" = xyes; then
   AC_DEFINE(HAVE_PHYSICAL_LIBRARY_MODELS_BUILT, 1, "Define if physical library models library is built")
fi
AM_CONDITIONAL(BUILD_LIB_PHYSICAL_LIBRARY_MODELS,  test "x$panda_USE_LIB_PHYSICAL_LIBRARY_MODELS" = xyes)

################################ Physical Library ##############################
if test "x$panda_USE_LIB_PHYSICAL_LIBRARY_MODELS" = xyes; then
  panda_USE_LIB_PHYSICAL_LIBRARY=yes;
fi
if test "x$panda_USE_LIB_PHYSICAL_LIBRARY" = xyes; then
  AC_DEFINE(HAVE_PHYSICAL_LIBRARY_BUILT, 1, "Define if physical library is built")
fi
AM_CONDITIONAL(BUILD_LIB_PHYSICAL_LIBRARY,  test "x$panda_USE_LIB_PHYSICAL_LIBRARY" = xyes)

#################################### CMOS #####################################
if test "x$panda_USE_LIB_CMOS" = xyes; then
   AC_DEFINE(HAVE_CMOS_BUILT, 1, "Define if cmos library is built")
fi
AM_CONDITIONAL(BUILD_LIB_CMOS, test "x$panda_USE_LIB_CMOS" = xyes)

############################## Target Technology ###############################
if test "x$panda_USE_LIB_TARGET" = xyes; then
   AC_DEFINE(HAVE_TARGET_BUILT, 1, "Define if target library is built")
fi
AM_CONDITIONAL(BUILD_LIB_TARGET, test "x$panda_USE_LIB_TARGET" = xyes)

################ AIG stuff #########################
if test "x$panda_USE_LIB_VIRTUAL_LIBRARY" = xyes; then
  AC_DEFINE(HAVE_VIRTUAL_LIBRARY_BUILT, 1, "Define if virtual library is built")
fi
AM_CONDITIONAL(BUILD_LIB_VIRTUAL_LIBRARY,  test "x$panda_USE_LIB_VIRTUAL_LIBRARY" = xyes)

################################## Technology ##################################
if test "x$panda_USE_LIB_PHYSICAL_LIBRARY" = xyes || test "x$panda_USE_LIB_VIRTUAL_LIBRARY" = xyes; then
  panda_USE_LIB_TECHNOLOGY=yes;
fi
if test "x$panda_USE_LIB_TECHNOLOGY" = xyes; then
  AC_DEFINE(HAVE_TECHNOLOGY_BUILT, 1, "Define if technoglogy library is built")
fi
AM_CONDITIONAL(BUILD_LIB_TECHNOLOGY,  test "x$panda_USE_LIB_TECHNOLOGY" = xyes)



################################################################################
#                                                                              #
#                              src/tree                                        #
#                                                                              #
################################################################################

########################### Code Estimation ####################################
if test "x$panda_USE_LIB_CODE_ESTIMATION" = xyes; then
    AC_DEFINE(HAVE_CODE_ESTIMATION_BUILT, 1, "Define if code estimation library is built")
fi
AM_CONDITIONAL(BUILD_LIB_CODE_ESTIMATION, test "x$panda_USE_LIB_CODE_ESTIMATION" = xyes)

########################## Tree Manipulation ###################################
if test "x$panda_USE_LIB_TREE_MANIPULATION" = xyes; then
    AC_DEFINE(HAVE_TREE_MANIPULATION_BUILT, 1, "Define if tree manipulation library is built")
fi
AM_CONDITIONAL(BUILD_LIB_TREE_MANIPULATION, test "x$panda_USE_LIB_TREE_MANIPULATION" = xyes)

############################## Tree Node #######################################
if test "x$panda_USE_LIB_TREE" = xyes; then
    AC_DEFINE(HAVE_TREE_BUILT, 1, "Define if tree library is built")
fi
AM_CONDITIONAL(BUILD_LIB_TREE, test "x$panda_USE_LIB_TREE" = xyes)



################################################################################
#                                                                              #
#                              src/utility                                     #
#                                                                              #
################################################################################

######################## Probability Distribution Stuff ########################
if test "x$panda_USE_LIB_PROBABILITY_DISTRIBUTION" = xyes; then
   AC_DEFINE(HAVE_PROBABILITY_DISTRIBUTION_BUILT, 1, "Define if probability distribution library is built")
fi
AM_CONDITIONAL(BUILD_LIB_PROBABILITY_DISTRIBUTION,  test "x$panda_USE_LIB_PROBABILITY_DISTRIBUTION" = xyes)

AM_CONDITIONAL(USE_FRACTIONS, test "x$panda_USE_FRACTIONS" = xyes)

if test "x$panda_USE_LIB_PROBABILITY_DISTRIBUTION" = xyes || test "x$panda_USE_FRACTIONS" = xyes; then
   panda_USE_LIB_UTILITY=yes;
fi
################ Utility stuff #########################
if test "x$panda_USE_LIB_UTILITY" = xyes; then
  AC_DEFINE(HAVE_UTILITY_BUILT, 1, "Define if utility library is built")
fi
AM_CONDITIONAL(BUILD_LIB_UTILITY,  test "x$panda_USE_LIB_UTILITY" = xyes)



################################################################################
#                                                                              #
#                              src/wrapper                                     #
#                                                                              #
################################################################################

################ Synthesis stuff #########################
if test "x$panda_USE_LIB_SYNTHESIS_WRAPPER" = xyes; then
   AC_DEFINE(HAVE_SYNTHESIS, 1, "Define if synthesis wrapper library is built")

fi
AM_CONDITIONAL(BUILD_SYNTHESIS, test "x$panda_USE_LIB_SYNTHESIS_WRAPPER" = xyes)
#Synopsys - fundamental tools for the synthesis flow
AM_CONDITIONAL(BUILD_SYNOPSYS, test "x$panda_USE_DESIGN_COMPILER" = xyes && test "x$panda_USE_PRIME_TIME" = xyes)
#Cadence
AM_CONDITIONAL(BUILD_CADENCE, test "x$panda_USE_SOC_ENCOUNTER" = xyes)
#Nangate
AM_CONDITIONAL(BUILD_NANGATE, test "x$panda_USE_DESIGN_OPTIMIZER" = xyes || test "x$panda_USE_LIBRARY_CREATOR" = xyes)


############################# coreAssembler stuff ##############################
if test "x$panda_USE_CORE_TOOLS" = xyes; then
  AC_DEFINE(HAVE_CORE_TOOLS, 1, "Define if the wrappers to Synopsys core tools are built")
fi
AM_CONDITIONAL(BUILD_CORE_TOOLS, test "x$panda_USE_CORE_TOOLS" = xyes)

################ Xilinx stuff ########################
if test "x$panda_USE_WRAPPER_XILINX" = xyes; then
  AC_DEFINE(HAVE_XILINX, 1, "Define if Xilinx wrapper library is built")
fi

################ Altera stuff ########################
if test "x$panda_USE_WRAPPER_ALTERA" = xyes; then
  AC_DEFINE(HAVE_ALTERA, 1, "Define if Altera wrapper library is built")
fi

################ Lattice stuff ########################
if test "x$panda_USE_WRAPPER_LATTICE" = xyes; then
  AC_DEFINE(HAVE_LATTICE, 1, "Define if Lattice wrapper library is built")
fi

################ NanoXplore stuff ########################
if test "x$panda_USE_WRAPPER_NANOXPLORE" = xyes; then
  AC_DEFINE(HAVE_NANOXPLORE, 1, "Define if NanoXplore wrapper library is built")
fi

################ Design Compiler stuff #####################
if test "x$panda_USE_DESIGN_COMPILER" = xyes; then
  AC_DEFINE(HAVE_DESIGN_COMPILER, 1, "Define if Synopsys Design Compiler is correctly configured")
fi

################ Formality stuff #########################
if test "x$panda_USE_FORMALITY" = xyes; then
  AC_DEFINE(HAVE_FORMALITY, 1, "Define if Synopsys Formality is correctly configured")
fi

################ Library Compiler stuff #########################
if test "x$panda_USE_LIBRARY_COMPILER" = xyes; then
  AC_DEFINE(HAVE_LIBRARY_COMPILER, 1, "Define if Synopsys Library Compiler is correctly configured")
fi
AM_CONDITIONAL(BUILD_LIBRARY_COMPILER, test "x$panda_USE_LIBRARY_COMPILER" = xyes)

################ Prime Time stuff #########################
if test "x$panda_USE_PRIME_TIME" = xyes; then
  AC_DEFINE(HAVE_PRIME_TIME, 1, "Define if Synopsys Prime Time is correctly configured")
fi
AM_CONDITIONAL(BUILD_PRIME_TIME, test "x$panda_USE_PRIME_TIME" = xyes)

########################### Library Creator stuff #############################
if test "x$panda_USE_LIBRARY_CREATOR" = xyes; then
  AC_DEFINE(HAVE_LIBRARY_CREATOR, 1, "Define if Nangate Library Creator is correctly configured")
fi
AM_CONDITIONAL(BUILD_LIBRARY_CREATOR, test "x$panda_USE_LIBRARY_CREATOR" = xyes)

########################### Nangate Design Optimizer stuff #############################
if test "x$panda_USE_DESIGN_OPTIMIZER" = xyes; then
  AC_DEFINE(HAVE_DESIGN_OPTIMIZER, 1, "Define if Nangate Design Optimizaer is correctly configured")
fi
AM_CONDITIONAL(BUILD_DESIGN_OPTIMIZER, test "x$panda_USE_DESIGN_OPTIMIZER" = xyes)

################ SoC Encounter stuff #########################
if test "x$panda_USE_SOC_ENCOUNTER" = xyes; then
  AC_DEFINE(HAVE_SOC_ENCOUNTER, 1, "Define if SoC Encounter is correctly configured")
fi
AM_CONDITIONAL(BUILD_SOC_ENCOUNTER, test "x$panda_USE_SOC_ENCOUNTER" = xyes)


################ Check for simulation ########################
if test "x$panda_USE_LIB_SIMULATION_WRAPPER" = xyes; then
  AC_DEFINE(HAVE_SIMULATION_WRAPPER_BUILT, 1, "Define if Simulation Tools wrapper is built")
fi
AM_CONDITIONAL(BUILD_SIMULATION, test "x$panda_USE_LIB_SIMULATION_WRAPPER" = xyes)

################ Icarus Verilog stuff ########################
if test "x$panda_USE_ICARUS" = xyes; then
  AC_DEFINE(HAVE_ICARUS, 1, "Define if icarus wrapper library is built")
fi

################ Verilator Verilog stuff ########################
if test "x$panda_USE_VERILATOR" = xyes; then
  AC_DEFINE(HAVE_VERILATOR, 1, "Define if verilator wrapper library is built")
fi

############################# Mentor modelsim stuff ############################
if test "x$panda_USE_MODELSIM" = xyes; then
  AC_DEFINE(HAVE_MODELSIM, 1, "Define if modelsim wrapper library is built")
fi
AM_CONDITIONAL(BUILD_MODELSIM, test "x$panda_USE_MODELSIM" = xyes)

#################################### A3 stuff ####################################
if test "x$panda_USE_A3" = xyes; then
  AC_DEFINE(HAVE_A3, 1, "Define if a3 wrapper library is built")
fi
AM_CONDITIONAL(BUILD_A3, test "x$panda_USE_A3" = xyes)

################ Diopsis stuff #########################
if test "x$panda_USE_DIOPSIS" = xyes; then

   AC_DEFINE(HAVE_DIOPSIS, 1, "Define if diopsis wrapper library is built")
fi
AM_CONDITIONAL(BUILD_DIOPSIS,       test "x$panda_USE_DIOPSIS" = xyes)

################ LEON3 stuff #########################
if test "x$panda_USE_LEON3" = xyes; then
  AC_DEFINE(HAVE_LEON3, 1, "Define if leon3 wrapper library is built")
fi
AM_CONDITIONAL(BUILD_LEON3,       test "x$panda_USE_LEON3" = xyes)

################################## MPPB stuff ##################################
if test "x$panda_USE_MPPB" = xyes; then
   AC_DEFINE(HAVE_MPPB, 1, "Define if MPPB wrapper library is built")
fi
AM_CONDITIONAL(BUILD_MPPB, test "x$panda_USE_MPPB" = xyes)

################ SimIt stuff #########################

################################## TSIM stuff ##################################
if test "x$panda_USE_TSIM" = xyes; then
  AC_DEFINE(HAVE_TSIM, 1, "Define if tsim wrapper library is built")
fi
AM_CONDITIONAL(BUILD_TSIM,       test "x$panda_USE_TSIM" = xyes)

################################# To Arch stuff ##################################

if test "x$panda_USE_LIB_TARGET_PROFILING" = xyes; then
  AC_DEFINE(HAVE_TARGET_PROFILING, 1, "Define if target profiling library is built")
fi
AM_CONDITIONAL(BUILD_LIB_TARGET_PROFILING, test "x$panda_USE_LIB_TARGET_PROFILING" = xyes)


################ Treegcc wrapper #########################
if test "x$panda_USE_LIB_FROM_C" = xyes; then
  AC_DEFINE(HAVE_FROM_C_BUILT, 1, "Define if gcc wrapper library is built")
fi
AM_CONDITIONAL(BUILD_LIB_FROM_C, test "x$panda_USE_LIB_FROM_C" = xyes)

if test "x$panda_USE_LIB_OBJECT_CODE_WRAPPER" = xyes; then
  AC_DEFINE(HAVE_OBJECT_CODE_WRAPPER_BUILT, 1, "Define if object code wrapper library is built")
fi
AM_CONDITIONAL(BUILD_LIB_OBJECT_CODE_WRAPPER, test "x$panda_USE_LIB_OBJECT_CODE_WRAPPER" = xyes)

################ Wrapper stuff ########################
if test "x$panda_USE_SIMULATION_WRAPPER" = xyes || test "x$panda_USE_LIB_SYNTHESIS_WRAPPER" = xyes || test "x$panda_USE_ICARUS" = xyes || test "x$panda_USE_LIB_FROM_STRUCTURAL" = xyes || test "x$panda_USE_LIB_FROM_C" = xyes || test "x$panda_USE_LIB_TARGET_PROFILING" = xyes || test "x$panda_USE_NODELIB" = xyes || test "x$panda_USE_FLOPOCO" = xyes || test "x$panda_USE_R" = xyes; then
   panda_USE_LIB_WRAPPER=yes;
fi
AM_CONDITIONAL(BUILD_LIB_WRAPPER,  test "x$panda_USE_LIB_WRAPPER" = xyes)

################ Vcd stuff #############################
if test "x$panda_USE_LIB_VCD" = xyes; then
  AC_DEFINE(HAVE_VCD_BUILT, 1, "Define if vcd library is built")
fi
AM_CONDITIONAL(BUILD_LIB_VCD, test "x$panda_USE_LIB_VCD" = xyes)

################################################################################
#                                                                              #
#                              ETC files                                       #
#                                                                              #
################################################################################
  AC_CONFIG_FILES(
                  etc/Makefile
                 )

################################################################################
#                                                                              #
#                                 devices                                      #
#                                                                              #
################################################################################

AC_CONFIG_FILES(
   etc/devices/Makefile
   etc/devices/Altera_devices/Makefile
   etc/devices/Xilinx_devices/Makefile
   etc/devices/ASIC_devices/Makefile
   etc/devices/Lattice_devices/Makefile
   etc/devices/NanoXplore_devices/Makefile
   etc/devices/Generic_devices/Makefile
   )



################################################################################
#                                                                              #
#                           latex_table_format                                 #
#                                                                              #
################################################################################

if test "x$panda_USE_LIB_TO_DATAFILE" = xyes; then
  AC_CONFIG_FILES(
                  etc/latex_table_format/Makefile
                 )
fi
AM_CONDITIONAL(BUILD_LATEX_TABLE_FORMAT, test "x$panda_USE_LIB_TO_DATAFILE" = xyes)

################################################################################
#                                                                              #
#                                gcc_plugin                                    #
#                                                                              #
################################################################################



################################# I386 Check ###################################
if test "x$panda_USE_I386_GCC" = xyes; then
   if test "x$MAX_GCC_VERSION" = "x"; then
      AC_MSG_ERROR("Oldest not supported version of I386 gcc not set")
   fi
   if test "x$MIN_GCC_VERSION" = "x"; then
      AC_MSG_ERROR("Oldest supported version of I386 gcc not set")
   fi
   EXTRA_CLANG_OPTIONS=
   if test "x$panda_clangoldabi" = xyes; then
     EXTRA_CLANG_OPTIONS=-D_GLIBCXX_USE_CXX11_ABI=0
   fi
   AC_CHECK_GCC_PLUGIN_DIR
   AC_CHECK_GCC45_I386_VERSION($MIN_GCC_VERSION,$MAX_GCC_VERSION)
   AC_CHECK_GCC46_I386_VERSION($MIN_GCC_VERSION,$MAX_GCC_VERSION)
   AC_CHECK_GCC47_I386_VERSION($MIN_GCC_VERSION,$MAX_GCC_VERSION)
   AC_CHECK_GCC48_I386_VERSION($MIN_GCC_VERSION,$MAX_GCC_VERSION)
   AC_CHECK_GCC49_I386_VERSION($MIN_GCC_VERSION,$MAX_GCC_VERSION)
   AC_CHECK_GCC5_I386_VERSION($MIN_GCC_VERSION,$MAX_GCC_VERSION)
   AC_CHECK_GCC6_I386_VERSION($MIN_GCC_VERSION,$MAX_GCC_VERSION)
   AC_CHECK_GCC7_I386_VERSION($MIN_GCC_VERSION,$MAX_GCC_VERSION)
   AC_CHECK_GCC8_I386_VERSION($MIN_GCC_VERSION,$MAX_GCC_VERSION)
   AC_CHECK_CLANG_PLUGIN_DIR
   AC_CHECK_CLANG4_I386_VERSION($MIN_CLANG_VERSION,$MAX_CLANG_VERSION,$EXTRA_CLANG_OPTIONS)
   AC_CHECK_CLANG5_I386_VERSION($MIN_CLANG_VERSION,$MAX_CLANG_VERSION,$EXTRA_CLANG_OPTIONS)
   AC_CHECK_CLANG6_I386_VERSION($MIN_CLANG_VERSION,$MAX_CLANG_VERSION,$EXTRA_CLANG_OPTIONS)
   AC_CHECK_CLANG7_I386_VERSION($MIN_CLANG_VERSION,$MAX_CLANG_VERSION,$EXTRA_CLANG_OPTIONS)
   AC_CHECK_CLANG8_I386_VERSION($MIN_CLANG_VERSION,$MAX_CLANG_VERSION,$EXTRA_CLANG_OPTIONS)
   AC_CHECK_CLANG9_I386_VERSION($MIN_CLANG_VERSION,$MAX_CLANG_VERSION,$EXTRA_CLANG_OPTIONS)
   AC_CHECK_CLANG10_I386_VERSION($MIN_CLANG_VERSION,$MAX_CLANG_VERSION,$EXTRA_CLANG_OPTIONS)
   AC_CHECK_CLANG11_I386_VERSION($MIN_CLANG_VERSION,$MAX_CLANG_VERSION,$EXTRA_CLANG_OPTIONS)

   if test "x$I386_GCC45_PLUGIN_COMPILER" = x && test "x$I386_GCC46_PLUGIN_COMPILER" = x && test "x$I386_GCC47_PLUGIN_COMPILER" = x && test "x$I386_GCC48_PLUGIN_COMPILER" = x && test "x$I386_GCC49_PLUGIN_COMPILER" = x && test "x$I386_GCC5_PLUGIN_COMPILER" = x && test "x$I386_GCC6_PLUGIN_COMPILER" = x && test "x$I386_GCC7_PLUGIN_COMPILER" = x && test "x$I386_GCC8_PLUGIN_COMPILER" = x && test "x$I386_CLANG4_PLUGIN_COMPILER" = x && test "x$I386_CLANG5_PLUGIN_COMPILER" = x && test "x$I386_CLANG6_PLUGIN_COMPILER" = x && test "x$I386_CLANG7_PLUGIN_COMPILER" = x && test "x$I386_CLANG8_PLUGIN_COMPILER" = x && test "x$I386_CLANG9_PLUGIN_COMPILER" = x && test "x$I386_CLANG10_PLUGIN_COMPILER" = x && test "x$I386_CLANG11_PLUGIN_COMPILER" = x; then
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

AM_CONDITIONAL(BUILD_I386_GCC8, test "x$build_I386_GCC8" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC8_EMPTY_PLUGIN, test "x$build_I386_GCC8_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC8_SSA_PLUGIN, test "x$build_I386_GCC8_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC8_SSA_PLUGINCPP, test "x$build_I386_GCC8_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_GCC8_TOPFNAME_PLUGIN, test "x$build_I386_GCC8_TOPFNAME_PLUGIN" = xyes)

AM_CONDITIONAL(BUILD_I386_CLANG4, test "x$build_I386_CLANG4" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG4_EMPTY_PLUGIN, test "x$build_I386_CLANG4_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG4_SSA_PLUGIN, test "x$build_I386_CLANG4_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG4_SSA_PLUGINCPP, test "x$build_I386_CLANG4_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG4_EXPANDMEMOPS_PLUGIN, test "x$build_I386_CLANG4_EXPANDMEMOPS_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG4_GEPICANON_PLUGIN, test "x$build_I386_CLANG4_GEPICANON_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG4_CSROA_PLUGIN, test "x$build_I386_CLANG4_CSROA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG4_TOPFNAME_PLUGIN, test "x$build_I386_CLANG4_TOPFNAME_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG4_ASTANALYZER_PLUGIN, test "x$build_I386_CLANG4_ASTANALYZER_PLUGIN" = xyes)

AM_CONDITIONAL(BUILD_I386_CLANG5, test "x$build_I386_CLANG5" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG5_EMPTY_PLUGIN, test "x$build_I386_CLANG5_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG5_SSA_PLUGIN, test "x$build_I386_CLANG5_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG5_SSA_PLUGINCPP, test "x$build_I386_CLANG5_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG5_EXPANDMEMOPS_PLUGIN, test "x$build_I386_CLANG5_EXPANDMEMOPS_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG5_GEPICANON_PLUGIN, test "x$build_I386_CLANG5_GEPICANON_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG5_CSROA_PLUGIN, test "x$build_I386_CLANG5_CSROA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG5_TOPFNAME_PLUGIN, test "x$build_I386_CLANG5_TOPFNAME_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG5_ASTANALYZER_PLUGIN, test "x$build_I386_CLANG5_ASTANALYZER_PLUGIN" = xyes)

AM_CONDITIONAL(BUILD_I386_CLANG6, test "x$build_I386_CLANG6" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG6_EMPTY_PLUGIN, test "x$build_I386_CLANG6_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG6_SSA_PLUGIN, test "x$build_I386_CLANG6_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG6_SSA_PLUGINCPP, test "x$build_I386_CLANG6_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG6_EXPANDMEMOPS_PLUGIN, test "x$build_I386_CLANG6_EXPANDMEMOPS_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG6_GEPICANON_PLUGIN, test "x$build_I386_CLANG6_GEPICANON_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG6_CSROA_PLUGIN, test "x$build_I386_CLANG6_CSROA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG6_TOPFNAME_PLUGIN, test "x$build_I386_CLANG6_TOPFNAME_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG6_ASTANALYZER_PLUGIN, test "x$build_I386_CLANG6_ASTANALYZER_PLUGIN" = xyes)

AM_CONDITIONAL(BUILD_I386_CLANG7, test "x$build_I386_CLANG7" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG7_EMPTY_PLUGIN, test "x$build_I386_CLANG7_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG7_SSA_PLUGIN, test "x$build_I386_CLANG7_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG7_SSA_PLUGINCPP, test "x$build_I386_CLANG7_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG7_EXPANDMEMOPS_PLUGIN, test "x$build_I386_CLANG7_EXPANDMEMOPS_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG7_GEPICANON_PLUGIN, test "x$build_I386_CLANG7_GEPICANON_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG7_CSROA_PLUGIN, test "x$build_I386_CLANG7_CSROA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG7_TOPFNAME_PLUGIN, test "x$build_I386_CLANG7_TOPFNAME_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG7_ASTANALYZER_PLUGIN, test "x$build_I386_CLANG7_ASTANALYZER_PLUGIN" = xyes)

AM_CONDITIONAL(BUILD_I386_CLANG8, test "x$build_I386_CLANG8" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG8_EMPTY_PLUGIN, test "x$build_I386_CLANG8_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG8_SSA_PLUGIN, test "x$build_I386_CLANG8_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG8_SSA_PLUGINCPP, test "x$build_I386_CLANG8_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG8_EXPANDMEMOPS_PLUGIN, test "x$build_I386_CLANG8_EXPANDMEMOPS_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG8_GEPICANON_PLUGIN, test "x$build_I386_CLANG8_GEPICANON_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG8_CSROA_PLUGIN, test "x$build_I386_CLANG8_CSROA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG8_TOPFNAME_PLUGIN, test "x$build_I386_CLANG8_TOPFNAME_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG8_ASTANALYZER_PLUGIN, test "x$build_I386_CLANG8_ASTANALYZER_PLUGIN" = xyes)

AM_CONDITIONAL(BUILD_I386_CLANG9, test "x$build_I386_CLANG9" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG9_EMPTY_PLUGIN, test "x$build_I386_CLANG9_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG9_SSA_PLUGIN, test "x$build_I386_CLANG9_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG9_SSA_PLUGINCPP, test "x$build_I386_CLANG9_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG9_EXPANDMEMOPS_PLUGIN, test "x$build_I386_CLANG9_EXPANDMEMOPS_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG9_GEPICANON_PLUGIN, test "x$build_I386_CLANG9_GEPICANON_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG9_CSROA_PLUGIN, test "x$build_I386_CLANG9_CSROA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG9_TOPFNAME_PLUGIN, test "x$build_I386_CLANG9_TOPFNAME_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG9_ASTANALYZER_PLUGIN, test "x$build_I386_CLANG9_ASTANALYZER_PLUGIN" = xyes)

AM_CONDITIONAL(BUILD_I386_CLANG10, test "x$build_I386_CLANG10" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG10_EMPTY_PLUGIN, test "x$build_I386_CLANG10_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG10_SSA_PLUGIN, test "x$build_I386_CLANG10_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG10_SSA_PLUGINCPP, test "x$build_I386_CLANG10_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG10_EXPANDMEMOPS_PLUGIN, test "x$build_I386_CLANG10_EXPANDMEMOPS_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG10_GEPICANON_PLUGIN, test "x$build_I386_CLANG10_GEPICANON_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG10_CSROA_PLUGIN, test "x$build_I386_CLANG10_CSROA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG10_TOPFNAME_PLUGIN, test "x$build_I386_CLANG10_TOPFNAME_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG10_ASTANALYZER_PLUGIN, test "x$build_I386_CLANG10_ASTANALYZER_PLUGIN" = xyes)

AM_CONDITIONAL(BUILD_I386_CLANG11, test "x$build_I386_CLANG11" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG11_EMPTY_PLUGIN, test "x$build_I386_CLANG11_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG11_SSA_PLUGIN, test "x$build_I386_CLANG11_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG11_SSA_PLUGINCPP, test "x$build_I386_CLANG11_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG11_EXPANDMEMOPS_PLUGIN, test "x$build_I386_CLANG11_EXPANDMEMOPS_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG11_GEPICANON_PLUGIN, test "x$build_I386_CLANG11_GEPICANON_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG11_CSROA_PLUGIN, test "x$build_I386_CLANG11_CSROA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG11_TOPFNAME_PLUGIN, test "x$build_I386_CLANG11_TOPFNAME_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG11_ASTANALYZER_PLUGIN, test "x$build_I386_CLANG11_ASTANALYZER_PLUGIN" = xyes)


if test "x$panda_USE_ARM_GCC" = xyes || test "x$panda_USE_I386_GCC" = xyes || test "x$panda_USE_SPARC_GCC" = xyes; then
   AC_CONFIG_FILES(
      etc/gcc_plugin/Makefile
      etc/clang_plugin/Makefile
   )
fi
AM_CONDITIONAL(BUILD_GCC_PLUGIN, test "x$panda_USE_ARM_GCC" = xyes || test "x$panda_USE_I386_GCC" = xyes || test "x$panda_USE_SPARC_GCC" = xyes)
AM_CONDITIONAL(ENABLE_WERROR, test "x$panda_WERROR" = xyes)

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
   #AC_CONFIG_FILES(
   #                crosstool-ng/Makefile
   #               )
fi
AM_CONDITIONAL(BUILD_CROSSTOOL_NG, test "x$panda_USE_CROSSTOOL_NG" = xyes)


################################################################################
#                                                                              #
#                                   libbambu                                   #
#                                                                              #
################################################################################
if test "x$panda_USE_LIBBAMBU" = xyes; then
   LIBBAMBU_SRCDIR="$TOPSRCDIR/libbambu"
   AC_DEFINE_UNQUOTED(LIBBAMBU_SRCDIR, "${LIBBAMBU_SRCDIR}", "Define the directory where the libbambu files are")

   AC_CONFIG_FILES(
      etc/lib/Makefile
      etc/lib/technology/Makefile
      etc/lib/technology/dynamic_generators/Makefile
      etc/libbambu/Makefile
      etc/libbambu/libm/Makefile
      etc/libbambu/pthread/Makefile
      etc/libbambu/softfloat/Makefile
      etc/libbambu/soft-fp/Makefile
      etc/libbambu/ac_types/Makefile
      etc/libbambu/ac_math/Makefile
   )
fi
AM_CONDITIONAL(BUILD_LIBBAMBU, test "x$panda_USE_LIBBAMBU" = "xyes")

################################################################################
#                                                                              #
#                                  macros                                      #
#                                                                              #
################################################################################

AC_CONFIG_FILES(etc/macros/Makefile)



################################################################################
#                                                                              #
#                                 scripts                                      #
#                                                                              #
################################################################################

################################### MPPB #######################################
AM_CONDITIONAL(BUILD_MPPB, test "x$panda_USE_MPPB" = xyes)

AC_CONFIG_FILES(etc/scripts/Makefile)

################################################################################
#                                                                              #
#                              EXT files                                       #
#                                                                              #
################################################################################
AC_CONFIG_FILES(ext/Makefile)


################################################################################
#                                                                              #
#                              FLAGS checking                                  #
#                                                                              #
################################################################################

############### Final check for static linking #############################################
if test "x$panda_ALLSTATIC" = xyes ;then
  AM_LDFLAGS="-all-static -pthread";
  LDFLAGS+=" -static";
  AC_SUBST(AM_LDFLAGS)
else
  AM_LDFLAGS="-pthread";
fi

case "${host_os}" in
  darwin*)
      PTHREAD_HACK=" -XCClinker '-framework CoreFoundation' "
      ;;
  mingw*)
      PTHREAD_HACK=" -limagehlp -lshlwapi -XCClinker '-Wl,--allow-multiple-definition -Wl,-Bstatic -lpthread -Wl,-Bdynamic' "
      ;;
  *)
      PTHREAD_HACK=" -XCClinker '-Wl,--whole-archive -lpthread -Wl,--no-whole-archive' "
      ;;
esac
AC_SUBST(PTHREAD_HACK)


openmp_ok=no
m4_ifdef([AC_OPENMP], [AC_OPENMP], [AX_OPENMP([openmp_ok=yes],[openmp_ok=no])])
if test "x$ac_cv_prog_cxx_openmp" != "xunsupported" && test "x$ac_cv_prog_cxx_openmp" != "x"; then
   openmp_ok=yes
   AC_DEFINE(HAVE_OPENMP, 1, "Define if OpenMP is supported")
fi
if test "x$openmp_ok" = "xyes"; then
   dnl We have it, now set up the flags
   CXXFLAGS+=" $OPENMP_CXXFLAGS "
fi

################ Debug checking ##############################
if test "x$panda_DEBUG" = xyes; then
  AC_DEFINE(PANDA_DEBUG, 1, "Define if a debug version is built")
  CXXFLAGS+=" -g3 "
  AC_DEFINE(HAVE_PRINT_STACK,1, "Define if call stack is printed on error")
fi

################ Werror checking ##############################
if test "x$panda_WERROR" = xyes; then
   AC_DEFINE(WERROR, 1, "Define if panda is compiled with -Werror")
   AC_DEFINE(SKIP_WARNING_SECTIONS, 1, "Define if warnings in non-cleaned sections are allowed")
fi

################################################################################
#                                                                              #
#                                Autoheader                                    #
#                                                                              #
################################################################################

cd $TOPSRCDIR && autoheader configure.ac && cp lconfig.h.in $BUILDDIR && cd $BUILDDIR
AC_CONFIG_HEADERS(lconfig.h:lconfig.h.in)
if test ! -d config_headers ; then
   mkdir config_headers
fi
for file in `find $BUILDDIR/config_headers -name *hpp`; do
   keyword=$(grep "#define" $file | awk '{print $2}')
   found=$(grep $keyword $BUILDDIR/lconfig.h.in )
   if test "x$found" = x; then
      echo "removing $file"
      rm $file
      if test -f $file.in; then
         echo "removing $file.in"
         rm $file.in
      fi
   fi
done;

for keyword in `grep "#undef" $BUILDDIR/lconfig.h | awk '{print $3}'`; do
   if test ! -f config_headers/config_"$keyword".hpp; then
      echo Creating config_headers/config_"$keyword".hpp
      echo "#define "$keyword 0 > config_headers/config_"$keyword".hpp
   else
      echo "#define "$keyword 0 > configtest.hpp
      if diff configtest.hpp config_headers/config_"$keyword".hpp >/dev/null 2>&1; then
      { echo config_headers/config_"$keyword".hpp is unchanged >&5
echo config_headers/config_"$keyword".hpp is unchanged >&6;}
    else
      echo config_headers/config_"$keyword".hpp is changed
      rm -f config_headers/config_"$keyword".hpp
      mv configtest.hpp config_headers/config_"$keyword".hpp
    fi
   fi
done;

for keyword in `grep "#define" $BUILDDIR/lconfig.h | awk '{print $2}'`; do
   if test ! -f config_headers/config_"$keyword".hpp; then
      echo Creating config_headers/config_"$keyword".hpp
      grep $keyword $BUILDDIR/lconfig.h > config_headers/config_"$keyword".hpp
   else
      grep $keyword $BUILDDIR/lconfig.h > configtest.hpp
      if diff configtest.hpp config_headers/config_"$keyword".hpp >/dev/null 2>&1; then
      { echo config_headers/config_"$keyword".hpp is unchanged >&5
echo config_headers/config_"$keyword".hpp is unchanged >&6;}
    else
      echo config_headers/config_"$keyword".hpp is changed
      rm -f config_headers/config_"$keyword".hpp
      mv configtest.hpp config_headers/config_"$keyword".hpp
    fi
   fi
done;

all_includes=" -I$BUILDDIR/config_headers"


################ Werror checking ##############################
#Checking should performed at the end since macros for checking boost library does not work with -WError
if test "x$panda_WERROR" = xyes; then
   CXXFLAGS+=" \
   -Wall \
   -Werror \
   -Wextra \
   -Wconversion \
   -Wno-deprecated \
   -Winit-self \
   -Wsign-conversion \
   -Wredundant-decls \
   -Wvla -Wshadow -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual \
   -Winit-self \
   -Wpointer-arith \
   -Wcast-qual \
   -Wcast-align \
   -Wdouble-promotion \
   -Wold-style-cast -Wno-error=old-style-cast \
   -Wsign-promo \
   -Wswitch-enum \
   -Wswitch-default \
   -Wundef \
   -Wno-error=unknown-pragmas"
   AX_CHECK_COMPILE_FLAG("-Wmissing-include-dirs", CXXFLAGS+=" -Wmissing-include-dirs ",,"-Werror") #it is not doing any real test
   AX_CHECK_COMPILE_FLAG("-Wlogical-op", CXXFLAGS+=" -Wlogical-op ",,"-Werror")
   AX_CHECK_COMPILE_FLAG("-Wunused-local-typedefs", CXXFLAGS+=" -Wunused-local-typedefs ",,"-Werror")
   #Disabled as error because of BOOST_FOREACH
   AX_CXXFLAGS_GCC_OPTION(-Wzero-as-null-pointer-constant,, CXXFLAGS+=" -Wzero-as-null-pointer-constant -Wno-error=zero-as-null-pointer-constant ",)

   AC_COMPILE_WPEDANTIC
   if test "x$panda_WPEDANTIC" = xyes; then
      CXXFLAGS+=" -Wpedantic "
   fi
   AX_CHECK_COMPILE_FLAG("-Wmismatched-tags", CXXFLAGS+=" -Wno-mismatched-tags ",,"-Werror")
   AX_CHECK_COMPILE_FLAG("-Woverlength-strings", CXXFLAGS+=" -Wno-overlength-strings ",,"-Werror")
   AX_CXXFLAGS_GCC_OPTION(-Wdeprecated-copy,, CXXFLAGS+=" -Wno-deprecated-copy -Wno-error=deprecated-copy ",)
   AX_CXXFLAGS_GCC_OPTION(-Wunused-but-set-parameter,, CXXFLAGS+=" -Wno-unused-but-set-parameter -Wno-error=unused-but-set-parameter ",)
else
   CXXFLAGS+=" -w "
fi

AX_CHECK_COMPILE_FLAG("-fdiagnostics-show-option", CXXFLAGS+=" -fdiagnostics-show-option ",,"-Werror")

AM_CONDITIONAL(ENABLE_WERROR, test "x$panda_WERROR" = xyes)


############################# Optimization setup ##############################
optlevel=""
AC_ARG_WITH(opt-level, [  --with-opt-level=<level> set the optimization level with which panda is compiled], [optlevel="$withval" ])

if test "x$optlevel" != x; then
  CXXFLAGS+=" -O$optlevel "
  CFLAGS+=" -O$optlevel "
else
  if test "x$panda_OPT" = xyes; then
    CXXFLAGS+=" -Ofast -march=native -mtune=native "
    CFLAGS+=" -Ofast -march=native -mtune=native "
  else
    if test "x$panda_WERROR" = xyes; then
      CXXFLAGS+=" -O1 "
    else
      CXXFLAGS+=" -O0 "
    fi
  fi
fi




################################################################################
#                                                                              #
#                             Final generation                                 #
#                                                                              #
################################################################################

AC_SUBST(CXXFLAGS)
AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(ENABLE_DOC)
AC_SUBST(all_includes)


AC_OUTPUT()

if test "$no_create" = yes; then
/bin/bash ./config.status lconfig.h
fi

for keyword in `grep "#undef" $BUILDDIR/lconfig.h | awk '{print $3}'`; do
   if test ! -f config_headers/config_"$keyword".hpp; then
      echo Creating config_headers/config_"$keyword".hpp
      echo "#define "$keyword 0 > config_headers/config_"$keyword".hpp
   else
      echo "#define "$keyword 0 > configtest.hpp
      if diff configtest.hpp config_headers/config_"$keyword".hpp >/dev/null 2>&1; then
      { echo config_headers/config_"$keyword".hpp is unchanged >&5
echo config_headers/config_"$keyword".hpp is unchanged >&6;}
    else
      echo config_headers/config_"$keyword".hpp is changed
      rm -f config_headers/config_"$keyword".hpp
      mv configtest.hpp config_headers/config_"$keyword".hpp
    fi
   fi
done;

for keyword in `grep "#define" $BUILDDIR/lconfig.h | awk '{print $2}'`; do
   if test ! -f config_headers/config_"$keyword".hpp; then
      echo Creating config_headers/config_"$keyword".hpp
      grep $keyword $BUILDDIR/lconfig.h > config_headers/config_"$keyword".hpp
   else
      grep $keyword $BUILDDIR/lconfig.h > configtest.hpp
      if diff configtest.hpp config_headers/config_"$keyword".hpp >/dev/null 2>&1; then
      { echo config_headers/config_"$keyword".hpp is unchanged >&5
echo config_headers/config_"$keyword".hpp is unchanged >&6;}
    else
      echo config_headers/config_"$keyword".hpp is changed
      rm -f config_headers/config_"$keyword".hpp
      mv configtest.hpp config_headers/config_"$keyword".hpp
    fi
   fi
done;


