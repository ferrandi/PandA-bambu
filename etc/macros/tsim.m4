dnl
dnl check where the tsim is
dnl
AC_DEFUN([AC_CHECK_TSIM],[
    AC_ARG_WITH(TSIM-dir,
    [  --with-tsim-dir=DIR  where the root of tsim is installed ],
    [
       ac_tsim_dir="$withval"
    ])

dnl check for tsim
if test "x$ac_tsim_dir" = x; then
   for dir in /opt/tsim-eval/tsim/linux/; do
      AC_CHECK_FILE("$dir/tsim-leon3", ac_tsim="$dir/tsim-leon3",)
   done
fi

TSIM_SIMULATOR=$ac_tsim
AC_DEFINE_UNQUOTED(TSIM_SIMULATOR, "${TSIM_SIMULATOR}", "Define the tsim simulator")

if test "x$ac_tsim" = x; then
   echo "checking if tsim is present... no"
   AC_MSG_ERROR([tsim simulator not found])
else
   echo "checking if tsim is present... yes $TSIM_SIMULATOR"
fi

AC_PROVIDE([$0])
])

dnl
dnl check where sparc-elf compiler is
dnl
AC_DEFUN([AC_CHECK_SPARC_ELF],[
    AC_ARG_WITH(sparc-elf-dir,
    [  --with-sparc-elf-dir=DIR  where the root of sparc-elf is installed ],
    [
       ac_sparc_elf_dir="$withval"
    ])

GCC_TO_BE_CHECKED="/opt/sparc-elf-4.4.1 /opt/sparc-elf-4.4.2"
if test "x$ac_spar_elf_dir" = x; then
   for dir in $GCC_TO_BE_CHECKED; do
      AC_CHECK_FILE("$dir/bin/sparc-elf-gcc", ac_sparc_elf_gcc="$dir/bin/sparc-elf-gcc",)
      AC_CHECK_FILE("$dir/bin/sparc-elf-cpp", ac_sparc_elf_cpp="$dir/bin/sparc-elf-cpp",)
      AC_CHECK_FILE("$dir/bin/sparc-elf-objcopy", ac_sparc_elf_objcopy="$dir/bin/sparc-elf-objcopy",)
      AC_CHECK_FILE("$dir/bin/sparc-elf-objdump", ac_sparc_elf_objdump="$dir/bin/sparc-elf-objdump",)
   done
fi

SPARC_ELF_GCC=$ac_sparc_elf_gcc
SPARC_ELF_CPP=$ac_sparc_elf_cpp
SPARC_ELF_OBJDUMP=$ac_sparc_elf_objdump
SPARC_ELF_OBJCOPY=$ac_sparc_elf_objcopy

AC_DEFINE_UNQUOTED(SPARC_ELF_GCC, "${SPARC_ELF_GCC}", "Define the sparc elf compiler")
if test "x$ac_sparc_elf_gcc" = x; then
   echo "checking if sparc elf compiler is present... no"
   AC_MSG_ERROR([sparc elf compiler not found])
else
   echo "checking if sparc elf compiler is present... yes $SPARC_ELF_GCC"
fi

AC_DEFINE_UNQUOTED(SPARC_ELF_CPP, "${SPARC_ELF_CPP}", "Define the sparc elf preprocessor")
if test "x$ac_sparc_elf_cpp" = x; then
   echo "checking if sparc elf preprocessor is present... no"
   AC_MSG_ERROR([sparc elf preprocessor not found])
else
   echo "checking if sparc elf preprocessor is present... yes $SPARC_ELF_CPP"
fi

AC_DEFINE_UNQUOTED(SPARC_ELF_OBJDUMP, "${SPARC_ELF_OBJDUMP}", "Define the sparc elf objdump")
if test "x$ac_sparc_elf_objdump" = x; then
   echo "checking if sparc elf objdump is present... no"
   AC_MSG_ERROR([sparc elf objdump not found])
else
   echo "checking if sparc elf objdump is present... yes $SPARC_ELF_OBJDUMP"
fi

AC_DEFINE_UNQUOTED(SPARC_ELF_OBJCOPY, "${SPARC_ELF_OBJCOPY}", "Define the sparc elf objcopy")
if test "x$ac_sparc_elf_objcopy" = x; then
   echo "checking if sparc elf objcopy is present... no"
   AC_MSG_ERROR([sparc elf objcopy not found])
else
   echo "checking if sparc elf objcopy is present... yes $SPARC_ELF_OBJCOPY"
fi

AC_PROVIDE([$0])
])
