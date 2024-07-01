################################################################################
#                                                                              #
#                      Checking common prerequisites                           #
#                                                                              #
################################################################################
AC_PREREQ(2.57)
AC_INIT(PandA, 2024.03, panda-info@polimi.it)
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
# Check if compiler support std::hexfloat
AC_HEXFLOAT


############################### c++17 checking ##############################
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
   else
      AC_MSG_ERROR("Compiler does not support C++17")
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

AC_LANG_PUSH([C++])
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

AC_DEFINE_UNQUOTED(BOOST_INCLUDE_DIR, "${BOOST_DIR}", "Define the boost include dir")

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

#synthesis wrappers

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
AC_ARG_ENABLE(flopoco,             [  --enable-flopoco             compile FloPoCo external library],                     [panda_USE_FLOPOCO="$enableval"], [panda_USE_FLOPOCO="no"])



################################################################################
#                                                                              #
#                          Variable initialization                             #
#                                                                              #
################################################################################
panda_USE_GENERIC_BASHBACKEND=no
panda_USE_I386_GCC=no;
panda_USE_LIBBAMBU=no;
panda_USE_LIBBDD=no
panda_USE_LIB_ALGORITHMS=no;
panda_USE_LIB_BACKEND=no;
panda_USE_LIB_BEHAVIOR=no;
panda_USE_LIB_CIRCUIT=no;
panda_USE_LIB_CLIQUECOVER=no;
panda_USE_LIB_DOMINANCE=no;
panda_USE_LIB_FROM_C=no;
panda_USE_LIB_FROM_DATA_XML=no;
panda_USE_LIB_GRAPH=no;
panda_USE_LIB_HLS=no;
panda_USE_LIB_HOST_PROFILING=no;
panda_USE_LIB_ILP=no;
panda_USE_LIB_LIBRARY_CHARACTERIZATION=no;
panda_USE_LIB_LOOPS=no;
panda_USE_LIB_PARSER=no;
panda_USE_LIB_POLIXML=no;
panda_USE_LIB_PRAGMA=no;
panda_USE_LIB_RTL_CHARACTERIZATION=no;
panda_USE_LIB_SIMULATION_WRAPPER=no;
panda_USE_LIB_SYNTHESIS_WRAPPER=no;
panda_USE_LIB_TECHNOLOGY=no;
panda_USE_LIB_TO_C=no;
panda_USE_LIB_TO_DATAFILE=no;
panda_USE_LIB_TREE=no;
panda_USE_LIB_TREE_MANIPULATION=no;
panda_USE_LIB_UTILITY=no;
panda_USE_LIB_VCD=no;
panda_USE_LIB_VCD_PARSER=no;
panda_USE_LIB_WRAPPER=no;

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
  panda_USE_SPIDER=yes;
fi

if test "x$panda_USE_BAMBU" != xyes &&  test "x$panda_USE_SPIDER" != xyes && test "x$panda_USE_EUCALYPTUS" != xyes; then
  echo "";
  echo "      ERROR: No tools selected. Please configure with --enable-<tool> or --enable-all"; 
  echo "";
  exit 1;
fi

if test "x$panda_WERROR" != xno && test "x$GIT_PRIVATE_PANDA" == xyes; then
  panda_WERROR=yes;
fi



################################################################################
#                                                                              #
#                            Tool implications                                 #
#                                                                              #
################################################################################

################ Bambu checking ##############################
if test "x$panda_USE_BAMBU" = xyes; then
  panda_USE_ABSEIL=yes;
  panda_USE_BAMBU_RESULTS_XML=yes;
  panda_USE_GENERIC_BASHBACKEND=yes;
  panda_USE_GLPK=yes;
  panda_USE_I386_GCC=yes;
  panda_USE_LIBBAMBU=yes;
  panda_USE_LIB_FROM_C=yes;
  panda_USE_LIB_FROM_PRAGMA=yes;
  panda_USE_LIB_GRAPH=yes;
  panda_USE_LIB_HLS=yes;
  panda_USE_LIB_HOST_PROFILING=yes;
  panda_USE_LIB_PRAGMA=yes;
  panda_USE_LIB_SIMULATION_WRAPPER=yes;
  panda_USE_LIB_SYNTHESIS_WRAPPER=yes;
  panda_USE_LIB_TECHNOLOGY=yes;
  panda_USE_LIB_TO_C=yes;
  panda_USE_LIB_TO_HDL=yes;
  panda_USE_LIB_VCD=yes;
  panda_USE_LIB_VCD_PARSER=yes;
  panda_USE_PARAMETER=yes;
  panda_USE_SPIDER=yes;
  panda_USE_SYNTHESIS_TOOL=yes;
  panda_USE_TASTE=yes;
  panda_USE_TREE_PANDA_GCC=yes;
  AS_VERSION_COMPARE($MIN_GCC_VERSION, [4.9.0], [MIN_GCC_VERSION=4.9.0],,)
  AS_VERSION_COMPARE($MAX_GCC_VERSION, [9.0.0],,,[MAX_GCC_VERSION=9.0.0])
  AS_VERSION_COMPARE($MIN_CLANG_VERSION, [4.0.0], [MIN_CLANG_VERSION=4.0.0],,)
  AS_VERSION_COMPARE($MAX_CLANG_VERSION, [17.0.0],,,[MAX_CLANG_VERSION=17.0.0])

  #characterization requires to configure at least one synthesis toolchain
  if test "x$panda_USE_GENERIC_BASHBACKEND" = xyes; then
    AC_APPEND(ENABLE_DOC,"EUCALYPTUS")
    panda_USE_EUCALYPTUS=yes;
  fi

  AC_CHECK_HEADERS([bdd.h fdd.h bvec.h ], [panda_USE_LIBBDD=yes],[ echo "$ac_header not found: Clang/LLVM Point-to analysis will not be enabled"])
  if test "x$panda_USE_LIBBDD" = xyes; then
  AC_DEFINE(HAVE_LIBBDD, 1, "Define if bambu is built")
  fi

  panda_USE_COIN_OR=no;

  AC_APPEND(ENABLE_DOC,"BAMBU")
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
fi
AM_CONDITIONAL(BUILD_SPIDER,      test "x$panda_USE_SPIDER" = xyes)


#################### PandA tree gcc checking #######################
if test "x$panda_USE_TREE_PANDA_GCC" = xyes; then
  AC_DEFINE(HAVE_TREE_PANDA_GCC_BUILT, 1, "Define if tree-panda-gcc is built")
fi
AM_CONDITIONAL(BUILD_TREE_PANDA_GCC, test "x$panda_USE_TREE_PANDA_GCC" = xyes)


##################### Library implications #########################
if test "x$panda_USE_GLPK" = xno && test "x$panda_USE_COIN_OR" = xyes; then
   panda_USE_GLPK=yes;
fi

if test "x$panda_USE_GLPK" = xyes || test "x$panda_USE_COIN_OR" = xyes; then
   panda_USE_LIB_ILP=yes;
fi

if test "x$panda_USE_LIB_FROM_C" = xyes && test "x$panda_USE_I386_GCC" != xyes; then
   panda_USE_I386_GCC=yes;
fi

if test "x$panda_USE_LIB_HLS" = xyes; then
  panda_USE_LIB_CIRCUIT=yes;
  panda_USE_LIB_CLIQUECOVER=yes;
  panda_USE_LIB_TECHNOLOGY=yes;
fi

if test "x$panda_USE_LIB_FROM_C" = xyes; then
  panda_USE_LIB_BEHAVIOR=yes;
  panda_USE_LIB_DOMINANCE=yes;
  panda_USE_LIB_FRONTEND_ANALYSIS=yes;
  panda_USE_LIB_LOOPS=yes;
  panda_USE_LIB_TREE=yes;
  panda_USE_LIB_TREE_MANIPULATION=yes;
  panda_USE_LIB_TREE_PARSER=yes;
fi

if test "x$panda_USE_LIB_FRONTEND_ANALYSIS" = xyes; then
   panda_USE_LIB_DESIGN_FLOWS=yes;
   panda_USE_LIB_POLIXML=yes;
fi
if test "x$panda_USE_LIB_TECHNOLOGY" = xyes; then
  panda_USE_LIB_POLIXML=yes;
fi

if test "x$panda_USE_PARAMETER" = xyes; then
  panda_USE_LIB_POLIXML=yes;
  panda_USE_LIB_UTILITY=yes;
fi

if test "x$panda_USE_LIB_POLIXML" = xyes; then
  panda_USE_LIB_FROM_POLIXML=yes;
fi

if (test "x$panda_USE_LIB_HLS" = xyes && (test "x$panda_USE_GENERIC_BASHBACKEND" = xyes)) then
   panda_USE_LIB_RTL_CHARACTERIZATION=yes;
fi

if test "x$panda_USE_SYNTHESIS_TOOL" = xyes || test "x$panda_USE_CORE_TOOLS" = xyes || test "x$panda_USE_GENERIC_BASHBACKEND" = xyes; then
   panda_USE_LIB_SYNTHESIS_WRAPPER=yes;
fi

if test "x$panda_USE_LIB_SYNTHESIS_WRAPPER" = xyes; then
   panda_USE_LIB_TO_HDL=yes;
fi

if test "x$panda_USE_LIB_FROM_SOURCE_CODE_STATISTICS" = xyes;  then
   panda_USE_SOURCE_CODE_STASTICS_XML=yes;
fi

if test "x$panda_USE_LIB_FROM_PRAGMA" = xyes || test "x$panda_USE_TASTE" = xyes; then
   panda_USE_SOURCE_CODE_MANIPULATION=yes
fi

if test "x$panda_USE_LIB_RTL_CHARACTERIZATION" = xyes; then
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
  CFLAGS+=" -DNDEBUG "
  CFLAGS_EXT+=" -DNDEBUG "
  CXXFLAGS+=" -DNDEBUG "
  CXXFLAGS_EXT+=" -DNDEBUG "
  CXXFLAGS_EXTFPLL+=" -DNDEBUG "
fi
AM_CONDITIONAL(BUILD_RELEASE, test "x$panda_RELEASE" = xyes)

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

################################### 32bits #####################################
if test "x$panda_32BITS" != x; then
   CFLAGS+=" -m32 "
   CFLAGS_EXT+=" -m32 "
   CXXFLAGS+=" -m32 "
   CXXFLAGS_EXT+=" -m32 "
   CXXFLAGS_EXTFPLL+=" -m32 "
   LDFLAGS+=" -m32 "
fi

if test ! -d ext ; then
   mkdir ext;
fi


################################# ABSEIL stuff ################################
if test "x$panda_USE_ABSEIL" = xyes; then
   $TOPSRCDIR/etc/macros/panda_config_subdir.sh ext/abseil $ac_configure_args "'CXXFLAGS=${CXXFLAGS_EXT}'" "'CFLAGS=${CFLAGS_EXT}'" "'LDFLAGS=$LDFLAGS'"
   if test $? != 0; then
      AC_MSG_ERROR("Error in abseil configuration")
   fi
   AC_DEFINE(HAVE_ABSEIL, 1, "Define if abseil is used")
   ABSEIL_DIR="$TOPSRCDIR/ext/abseil/abseil-cpp"
   ABSEIL_OBJDIR="ext/abseil"
fi
AM_CONDITIONAL(BUILD_ABSEIL, test "x$panda_USE_ABSEIL" = xyes)
AC_SUBST(ABSEIL_DIR)


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


################################## GLPK stuff ##################################
LIB_SUITESPARSECONFIG=
AM_CONDITIONAL(BUILD_GLPK, test "x$panda_USE_GLPK" = xyes)
if test "x$panda_USE_GLPK" = xyes; then
   AC_LANG_PUSH([C])
   if test "${DISTRO:0:8}" != "centos_6" && test "${DISTRO:0:6}" != "rhel_7" && test "${DISTRO:0:8}" != "oracle_7" && test "$DISTRO" != "ubuntu_12.04"; then
      LIB_SUITESPARSECONFIG=-lsuitesparseconfig
      AC_CHECK_LIB([suitesparseconfig],[SuiteSparse_malloc], [echo "Checking for libsuitesparseconfig... yes"], [echo "Checking for libsuitesparseconfig... no"; AC_MSG_ERROR("libsuitesparseconfig not found")])
   fi
   AC_CHECK_LIB([amd],[amd_defaults], [echo "Checking for libamd... yes"], [echo "Checking for libamd... no"; AC_MSG_ERROR("libamd not found")])
   AC_CHECK_LIB([colamd],[symamd], [echo "Checking for libcolamd... yes"], [echo "Checking for libcolamd... no"; AC_MSG_ERROR("libcolamd not found")], [${LIB_SUITESPARSECONFIG}])
   AC_CHECK_LIB([ltdl],[lt_dlerror], [echo "Checking for libltdl... yes"], [echo "Checking for libltd... no"; AC_MSG_ERROR("libltdl not found")])
   AC_CHECK_LIB([glpk],[glp_simplex], [echo "Checking for libglpk... yes"], [echo "Checking for libglpk... no"; AC_MSG_ERROR("libglpk not found")], [-lamd -lcolamd -lltdl -lgmp ${LIB_SUITESPARSECONFIG}])
   AC_LANG_POP([C])
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


################################# TASTE ################################
if test "x$panda_USE_TASTE" = xyes; then
   AC_DEFINE(HAVE_TASTE, 1, "Define if TASTE has to be used")
   AC_DEFINE_UNQUOTED(GRLIB_DIR, "${prefix}/lib/GRLIB", "Define the GRLIB directory")
fi
AM_CONDITIONAL(BUILD_TASTE, test "x$panda_USE_TASTE" = xyes)
AM_CONDITIONAL(USE_GRLIB, test "x$panda_USE_TASTE" = xyes)


#################################### Ext Libraries #############################
if test "x$ac_cv_cxx_compile_cxx17_native" = xyes || test "x$ac_cv_cxx_compile_cxx17_cxx" = xyes || test "x$ac_cv_cxx_compile_cxx17_gxx" = xyes || test "x$ac_cv_cxx_compile_cxx17_cxx1z" = xyes || test "x$panda_USE_ABSEIL" = xyes || test "x$panda_USE_COIN_OR" = xyes || test "x$panda_USE_FLOPOCO" = xyes || test "x$panda_USE_LP_SOLVE" = xyes; then
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
               )



################################################################################
#                                                                              #
#                              src/algorithms                                  #
#                                                                              #
################################################################################

AM_CONDITIONAL(BUILD_LIB_CLIQUECOVER,  test "x$panda_USE_LIB_CLIQUECOVER" = xyes)
AM_CONDITIONAL(BUILD_LIB_DOMINANCE,  test "x$panda_USE_LIB_DOMINANCE" = xyes)
AM_CONDITIONAL(BUILD_LIB_LOOPS,  test "x$panda_USE_LIB_LOOPS" = xyes)

if test "x$panda_USE_LIB_CLIQUECOVER" = xyes || test "x$panda_USE_LIB_DOMINANCE" = xyes || test "x$panda_USE_LIB_LOOPS" = xyes; then
  panda_USE_LIB_ALGORITHMS=yes;
fi
AM_CONDITIONAL(BUILD_LIB_ALGORITHMS,  test "x$panda_USE_LIB_ALGORITHMS" = xyes)



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
if test "x$panda_USE_LIB_CIRCUIT" = xyes; then
  AC_DEFINE(HAVE_CIRCUIT_BUILT, 1, "Define if circuit library is built")
fi
AM_CONDITIONAL(BUILD_LIB_CIRCUIT,  test "x$panda_USE_LIB_CIRCUIT" = xyes)



################################################################################
#                                                                              #
#                              src/constants                                   #
#                                                                              #
################################################################################
if test "x$panda_USE_BAMBU_RESULTS_XML" = xyes; then
   AC_DEFINE(HAVE_BAMBU_RESULTS_XML, 1, "Define if bambu results xmls are used")
fi
AM_CONDITIONAL(USE_CONSTANTS, test "x$panda_USE_CONSTANTS" = xyes)



################################################################################
#                                                                              #
#                            src/design_flows                                  #
#                                                                              #
################################################################################
if test "x$panda_USE_LIB_BACKEND" = xyes; then
   panda_USE_LIB_DESIGN_FLOWS=yes;
fi
if test "x$panda_USE_LIB_DESIGN_FLOWS" = xyes; then
  AC_DEFINE(HAVE_DESIGN_FLOWS_BUILT, 1, "Define if design flows library is built")
fi
AM_CONDITIONAL(BUILD_LIB_DESIGN_FLOWS, test "x$panda_USE_LIB_DESIGN_FLOWS" = xyes)



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
AM_CONDITIONAL(BUILD_LIB_GRAPH,  test "x$panda_USE_LIB_GRAPH" = xyes)



################################################################################
#                                                                              #
#                         src/HLS                                              #
#                                                                              #
################################################################################
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
if test "x$panda_USE_LIB_ILP" = xyes; then
  AC_DEFINE(HAVE_ILP_BUILT, 1, "Define if ILP library is built")
fi
AM_CONDITIONAL(BUILD_LIB_ILP,  test "x$panda_USE_LIB_ILP" = xyes)



################################################################################
#                                                                              #
#                     src/intermediate_representations                         #
#                                                                              #
################################################################################
if test "x$panda_USE_LIB_FROM_AADL_ASN" = xyes || test "x$panda_USE_LIB_HLS" = xyes; then
   panda_USE_LIB_INTERMEDIATE_REPRESENTATIONS=yes;
fi
if test "x$panda_USE_LIB_INTERMEDIATE_REPRESENTATIONS" = xyes; then
  AC_DEFINE(HAVE_INTERMEDIATE_REPRESENTATIONS_BUILT, 1, "Define if intermediate representation library is built")
fi
AM_CONDITIONAL(BUILD_LIB_INTERMEDIATE_REPRESENTATIONS, test "x$panda_USE_LIB_INTERMEDIATE_REPRESENTATIONS" = xyes)



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

################ Discrepancy stuff #########################
if test "x$panda_USE_LIB_FROM_DISCREPANCY" = xyes; then
  AC_DEFINE(HAVE_FROM_DISCREPANCY_BUILT, 1, "Define from discrepancy library is built")
  AC_CONFIG_FILES(
                  src/parser/discrepancy/Makefile
                  )
fi
AM_CONDITIONAL(BUILD_LIB_FROM_DISCREPANCY,  test "x$panda_USE_LIB_FROM_DISCREPANCY" = xyes)

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

################ source code statistics stuff ##################################
if test "x$panda_USE_LIB_FROM_DATA_XML" = xyes; then
   AC_DEFINE(HAVE_FROM_DATA_XML_BUILT, 1, "Define if from data xml library is built")
   AC_CONFIG_FILES(
                   src/parser/data_xml/Makefile
                 )
fi
AM_CONDITIONAL(BUILD_LIB_FROM_DATA_XML, test "x$panda_USE_LIB_FROM_DATA_XML" = xyes)

################ Tree Parser #########################
if test "x$panda_USE_LIB_TREE_PARSER" = xyes; then
  AC_DEFINE(HAVE_TREE_PARSER_BUILT, 1, "Define if from tree parser library is built")
  AC_CONFIG_FILES(
                   src/parser/compiler/Makefile
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
if test "x$panda_USE_FROM_C" = xyes || test "x$panda_USE_LIB_FROM_PRAGMA" = xyes || test "x$panda_USE_LIB_HLS" = xyes || test "x$panda_USE_LIB_POLIXML" = xyes || test "x$panda_USE_LIB_VCD_PARSER" = xyes; then
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
if test "x$panda_USE_LIB_POLIXML" = xyes; then
  AC_DEFINE(HAVE_POLIXML_BUILT, 1, "Define if polixml library is built")
fi
AM_CONDITIONAL(BUILD_LIB_POLIXML,  test "x$panda_USE_LIB_POLIXML" = xyes)



################################################################################
#                                                                              #
#                              src/pragma                                      #
#                                                                              #
################################################################################
if test "x$panda_USE_LIB_PRAGMA" = xyes; then
  AC_DEFINE(HAVE_PRAGMA_BUILT, 1, "Define if pragma library is built")
fi
AM_CONDITIONAL(BUILD_LIB_PRAGMA,  test "x$panda_USE_LIB_PRAGMA" = xyes)



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

################################## Technology ##################################
if test "x$panda_USE_LIB_TECHNOLOGY" = xyes; then
  AC_DEFINE(HAVE_TECHNOLOGY_BUILT, 1, "Define if technoglogy library is built")
fi
AM_CONDITIONAL(BUILD_LIB_TECHNOLOGY,  test "x$panda_USE_LIB_TECHNOLOGY" = xyes)



################################################################################
#                                                                              #
#                              src/tree                                        #
#                                                                              #
################################################################################

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
AM_CONDITIONAL(BUILD_SYNTHESIS, test "x$panda_USE_LIB_SYNTHESIS_WRAPPER" = xyes)


################ Check for simulation ########################
if test "x$panda_USE_LIB_SIMULATION_WRAPPER" = xyes; then
  AC_DEFINE(HAVE_SIMULATION_WRAPPER_BUILT, 1, "Define if Simulation Tools wrapper is built")
fi
AM_CONDITIONAL(BUILD_SIMULATION, test "x$panda_USE_LIB_SIMULATION_WRAPPER" = xyes)

################ Treegcc wrapper #########################
if test "x$panda_USE_LIB_FROM_C" = xyes; then
  AC_DEFINE(HAVE_FROM_C_BUILT, 1, "Define if gcc wrapper library is built")
fi
AM_CONDITIONAL(BUILD_LIB_FROM_C, test "x$panda_USE_LIB_FROM_C" = xyes)

################ Wrapper stuff ########################
if test "x$panda_USE_SIMULATION_WRAPPER" = xyes || test "x$panda_USE_LIB_SYNTHESIS_WRAPPER" = xyes || test "x$panda_USE_LIB_FROM_C" = xyes || test "x$panda_USE_FLOPOCO" = xyes; then
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
                  etc/Appimage/Makefile
                  etc/containers/Makefile
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
                  etc/scripts/latex_table_format/Makefile
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
   AC_CHECK_CLANG12_I386_VERSION($MIN_CLANG_VERSION,$MAX_CLANG_VERSION,$EXTRA_CLANG_OPTIONS)
   AC_CHECK_CLANG13_I386_VERSION($MIN_CLANG_VERSION,$MAX_CLANG_VERSION,$EXTRA_CLANG_OPTIONS)
   AC_CHECK_CLANG16_I386_VERSION($MIN_CLANG_VERSION,$MAX_CLANG_VERSION,$EXTRA_CLANG_OPTIONS)
   AC_CHECK_CLANGVVD_I386_VERSION($MIN_CLANG_VERSION,$MAX_CLANG_VERSION,$EXTRA_CLANG_OPTIONS)

   if test "x$I386_GCC49_PLUGIN_COMPILER" = x && test "x$I386_GCC5_PLUGIN_COMPILER" = x && test "x$I386_GCC6_PLUGIN_COMPILER" = x && test "x$I386_GCC7_PLUGIN_COMPILER" = x && test "x$I386_GCC8_PLUGIN_COMPILER" = x && test "x$I386_CLANG4_PLUGIN_COMPILER" = x && test "x$I386_CLANG5_PLUGIN_COMPILER" = x && test "x$I386_CLANG6_PLUGIN_COMPILER" = x && test "x$I386_CLANG7_PLUGIN_COMPILER" = x && test "x$I386_CLANG8_PLUGIN_COMPILER" = x && test "x$I386_CLANG9_PLUGIN_COMPILER" = x && test "x$I386_CLANG10_PLUGIN_COMPILER" = x && test "x$I386_CLANG11_PLUGIN_COMPILER" = x && test "x$I386_CLANG12_PLUGIN_COMPILER" = x && test "x$I386_CLANG13_PLUGIN_COMPILER" = x && test "x$I386_CLANG16_PLUGIN_COMPILER" = x && test "x$I386_CLANGVVD_PLUGIN_COMPILER" = x; then
      AC_MSG_ERROR("gcc with support to -m32 and plugin not found")
   fi
fi

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

AM_CONDITIONAL(BUILD_I386_CLANG12, test "x$build_I386_CLANG12" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG12_EMPTY_PLUGIN, test "x$build_I386_CLANG12_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG12_SSA_PLUGIN, test "x$build_I386_CLANG12_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG12_SSA_PLUGINCPP, test "x$build_I386_CLANG12_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG12_EXPANDMEMOPS_PLUGIN, test "x$build_I386_CLANG12_EXPANDMEMOPS_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG12_GEPICANON_PLUGIN, test "x$build_I386_CLANG12_GEPICANON_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG12_CSROA_PLUGIN, test "x$build_I386_CLANG12_CSROA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG12_TOPFNAME_PLUGIN, test "x$build_I386_CLANG12_TOPFNAME_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG12_ASTANALYZER_PLUGIN, test "x$build_I386_CLANG12_ASTANALYZER_PLUGIN" = xyes)

AM_CONDITIONAL(BUILD_I386_CLANG13, test "x$build_I386_CLANG13" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG13_EMPTY_PLUGIN, test "x$build_I386_CLANG13_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG13_SSA_PLUGIN, test "x$build_I386_CLANG13_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG13_SSA_PLUGINCPP, test "x$build_I386_CLANG13_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG13_EXPANDMEMOPS_PLUGIN, test "x$build_I386_CLANG13_EXPANDMEMOPS_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG13_GEPICANON_PLUGIN, test "x$build_I386_CLANG13_GEPICANON_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG13_CSROA_PLUGIN, test "x$build_I386_CLANG13_CSROA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG13_TOPFNAME_PLUGIN, test "x$build_I386_CLANG13_TOPFNAME_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG13_ASTANALYZER_PLUGIN, test "x$build_I386_CLANG13_ASTANALYZER_PLUGIN" = xyes)

AM_CONDITIONAL(BUILD_I386_CLANG16, test "x$build_I386_CLANG16" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG16_EMPTY_PLUGIN, test "x$build_I386_CLANG16_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG16_SSA_PLUGIN, test "x$build_I386_CLANG16_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG16_SSA_PLUGINCPP, test "x$build_I386_CLANG16_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG16_EXPANDMEMOPS_PLUGIN, test "x$build_I386_CLANG16_EXPANDMEMOPS_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG16_GEPICANON_PLUGIN, test "x$build_I386_CLANG16_GEPICANON_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG16_CSROA_PLUGIN, test "x$build_I386_CLANG16_CSROA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG16_TOPFNAME_PLUGIN, test "x$build_I386_CLANG16_TOPFNAME_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANG16_ASTANALYZER_PLUGIN, test "x$build_I386_CLANG16_ASTANALYZER_PLUGIN" = xyes)

AM_CONDITIONAL(BUILD_I386_CLANGVVD, test "x$build_I386_CLANGVVD" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANGVVD_EMPTY_PLUGIN, test "x$build_I386_CLANGVVD_EMPTY_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANGVVD_SSA_PLUGIN, test "x$build_I386_CLANGVVD_SSA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANGVVD_SSA_PLUGINCPP, test "x$build_I386_CLANGVVD_SSA_PLUGINCPP" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANGVVD_EXPANDMEMOPS_PLUGIN, test "x$build_I386_CLANGVVD_EXPANDMEMOPS_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANGVVD_GEPICANON_PLUGIN, test "x$build_I386_CLANGVVD_GEPICANON_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANGVVD_CSROA_PLUGIN, test "x$build_I386_CLANGVVD_CSROA_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANGVVD_TOPFNAME_PLUGIN, test "x$build_I386_CLANGVVD_TOPFNAME_PLUGIN" = xyes)
AM_CONDITIONAL(BUILD_I386_CLANGVVD_ASTANALYZER_PLUGIN, test "x$build_I386_CLANGVVD_ASTANALYZER_PLUGIN" = xyes)

if test "x$panda_USE_I386_GCC" = xyes; then
   AC_CONFIG_FILES(
      etc/gcc_plugin/Makefile
      etc/clang_plugin/Makefile
   )
fi
AM_CONDITIONAL(BUILD_GCC_PLUGIN, test "x$panda_USE_I386_GCC" = xyes)
AM_CONDITIONAL(ENABLE_WERROR, test "x$panda_WERROR" = xyes)



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
      etc/libbambu/Makefile
      etc/libbambu/libm/Makefile
      etc/libbambu/libmdpi/Makefile
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
  LDFLAGS+="-all-static -pthread -static";
else
  AM_LDFLAGS="-pthread";
fi
AC_SUBST(AM_LDFLAGS)

AC_MSG_CHECKING([if std::filesystem requires linking stdc++fs])
AC_LANG_PUSH([C++])
AC_LINK_IFELSE(
    [AC_LANG_SOURCE([
        #include <filesystem>
        int main() {
            std::error_code ec;
            std::filesystem::create_directory("/dev/null", ec);
        }
    ])],
    [ac_cv_fs_stdlib=no],
    [ac_cv_fs_stdlib=yes]
)
AC_LANG_POP([C++])
if test "x$ac_cv_fs_stdlib" = xyes; then
    AC_MSG_RESULT(yes)
    AM_LDFLAGS="$AM_LDFLAGS -lstdc++fs"
else
    AC_MSG_RESULT(no)
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
AC_LANG_PUSH([C++])
m4_ifdef([AC_OPENMP], [AC_OPENMP], [AX_OPENMP([openmp_ok=yes],[openmp_ok=no])])
if test "x$ac_cv_prog_cxx_openmp" != "xunsupported" && test "x$ac_cv_prog_cxx_openmp" != "x"; then
   openmp_ok=yes
fi
if test "x$openmp_ok" = "xyes"; then
   dnl We have it, now set up the flags
   CXXFLAGS+=" $OPENMP_CXXFLAGS "
   AC_DEFINE(HAVE_OPENMP, 1, "Define if OpenMP is supported")
fi
AC_LANG_POP([C++])

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
   -Wno-unknown-pragmas"
   AX_CHECK_COMPILE_FLAG("-Wmissing-include-dirs", CXXFLAGS+=" -Wmissing-include-dirs ",,"-Werror") #it is not doing any real test
   AX_CHECK_COMPILE_FLAG("-Wlogical-op", CXXFLAGS+=" -Wlogical-op ",,"-Werror")
   AX_CHECK_COMPILE_FLAG("-Wunused-local-typedefs", CXXFLAGS+=" -Wunused-local-typedefs ",,"-Werror")
   #Disabled as error because of BOOST_FOREACH
   AX_CXXFLAGS_GCC_OPTION(-Wzero-as-null-pointer-constant,, CXXFLAGS+=" -Wzero-as-null-pointer-constant -Wno-error=zero-as-null-pointer-constant ",)
   AX_CXXFLAGS_GCC_OPTION(-Wuse-after-free,, CXXFLAGS+=" -Wuse-after-free -Wno-error=use-after-free ",)

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


AC_OUTPUT

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
