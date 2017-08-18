dnl
dnl check if a3 is in the path
dnl
AC_DEFUN([AC_CHECK_MPPB],[

AC_DEFINE_UNQUOTED(MPPB_SREC_UPLOAD, "$prefix/bin/srec_upload", "Define the srec_upload")

dnl Check for xenium compiler
xentium_dirs="/opt/xentium-tools-1.0.beta11-linux64 /opt/xentium-tools-1.0.beta10-linux64 /opt/xentium-tools-1.0.beta9-linux64 /opt/xentium-tools-1.0.beta5-linux64 /opt/xentium-tools-1.0.beta10-linux32 /opt/xentium-tools-1.0.beta9-linux32 /opt/xentium-tools-1.0.beta5-linux32"

for dir in $xentium_dirs; do
   if test -f $dir/bin/xentium-clang; then
      echo "checking if xentium compiler is present... yes $dir/bin/xentium-clang"
      XENTIUM_CC=$dir/bin/xentium-clang
      if test -f $dir/bin/xentium-objcopy; then
         echo "checking if xentium objcopy is present... yes $dir/bin/xentium-objcopy"
         XENTIUM_OBJCOPY=$dir/bin/xentium-objcopy
         if test -d $dir/sysroots/default/include; then
            echo "checking if xentium include dir is present... yes $dir/sysroots/default/include"
            XENTIUM_INCLUDE=$dir/sysroots/default/include
            break;
         else
            echo "checking if xentium include dir is present... no $dir/sysroots/default/include"
            XENTIUM_CC=
            XENTIUM_OBJCOPY=
         fi
      else
         XENTIUM_CC=
      fi
   fi
done
if test "x$XENTIUM_CC" = x; then
   AC_MSG_ERROR("xentium compiler not found")
fi
AC_DEFINE_UNQUOTED(XENTIUM_CC, "${XENTIUM_CC}", "Define the xentium compiler")
AC_DEFINE_UNQUOTED(XENTIUM_INCLUDE, "${XENTIUM_INCLUDE}", "Define the xentium include dir")

if test "x$XENTIUM_OBJCOPY" = x; then
   AC_MSG_ERROR("xentium objcopy not found")
fi
AC_DEFINE_UNQUOTED(XENTIUM_OBJCOPY, "${XENTIUM_OBJCOPY}", "Define the xentium objcopy")

dnl Check for mppb software
mppb_dirs="/opt/mppb_software_v1.0 /opt/MPPB_software"
for dir in $mppb_dirs; do
   if test -d $dir; then
      echo "checking if mppb sofware is present... yes $dir"
      MPPB_DIR=$dir
   fi
done
if test "x$MPPB_DIR" = x; then
   AC_MSG_ERROR("Mppb software not found")
fi
AC_DEFINE_UNQUOTED(MPPB_DIR, "${MPPB_DIR}", "Define the mppb software dir")

dnl Check for xentium sysroot
sysroot_dirs="/opt/panda/xentium_sysroot"
for dir in $sysroot_dirs; do
  if test -f $dir/lib/mppb-crt0.o; then
     echo "checkeing if xentium sysroot is present... yes $dir"
     XENTIUM_SYSROOT=$dir
  fi
done
if test "x$XENTIUM_SYSROOT" = x; then
   AC_MSG_ERROR("Xentium sysroot not found")
fi
AC_DEFINE_UNQUOTED(XENTIUM_SYSROOT, "${XENTIUM_SYSROOT}", "Define the xentium sysroot dir")

AC_PROVIDE([$0])
])

dnl
dnl check where sparc-elf compiler is
dnl
AC_DEFUN([AC_CHECK_MPPB_ELF],[
    AC_ARG_WITH(sparc-elf-dir,
    [  --with-sparc-elf-dir=DIR  where the root of sparc-elf is installed ],
    [
       ac_sparc_elf_dir="$withval"
    ])

GCC_TO_BE_CHECKED="/opt/sparc-elf-4.4.2"
if test "x$ac_sparc_elf_dir" = x; then
   for dir in $GCC_TO_BE_CHECKED; do
      AC_CHECK_FILE("$dir/bin/sparc-elf-gcc", ac_sparc_elf_gcc="$dir/bin/sparc-elf-gcc",)
      AC_CHECK_FILE("$dir/bin/sparc-elf-cpp", ac_sparc_elf_cpp="$dir/bin/sparc-elf-cpp",)
      AC_CHECK_FILE("$dir/bin/sparc-elf-objcopy", ac_sparc_elf_objcopy="$dir/bin/sparc-elf-objcopy",)
      AC_CHECK_FILE("$dir/bin/sparc-elf-objdump", ac_sparc_elf_objdump="$dir/bin/sparc-elf-objdump",)
   done
fi

MPPB_ELF_GCC=$ac_sparc_elf_gcc
MPPB_ELF_CPP=$ac_sparc_elf_cpp
MPPB_ELF_OBJDUMP=$ac_sparc_elf_objdump
MPPB_ELF_OBJCOPY=$ac_sparc_elf_objcopy

AC_DEFINE_UNQUOTED(MPPB_ELF_GCC, "${MPPB_ELF_GCC}", "Define the mppb sparc elf compiler")
if test "x$ac_sparc_elf_gcc" = x; then
   echo "checking if mppb sparc elf compiler is present... no"
   AC_MSG_ERROR([mppb sparc elf compiler not found])
else
   echo "checking if mppb sparc elf compiler is present... yes $MPPB_ELF_GCC"
fi

AC_DEFINE_UNQUOTED(MPPB_ELF_CPP, "${MPPB_ELF_CPP}", "Define the mppb sparc elf preprocessor")
if test "x$ac_sparc_elf_cpp" = x; then
   echo "checking if mppb sparc elf preprocessor is present... no"
   AC_MSG_ERROR([mppb sparc elf preprocessor not found])
else
   echo "checking if mppb sparc elf preprocessor is present... yes $MPPB_ELF_CPP"
fi

AC_DEFINE_UNQUOTED(MPPB_ELF_OBJDUMP, "${MPPB_ELF_OBJDUMP}", "Define the mppb sparc elf objdump")
if test "x$ac_sparc_elf_objdump" = x; then
   echo "checking if mppb sparc elf objdump is present... no"
   AC_MSG_ERROR([mppb sparc elf objdump not found])
else
   echo "checking if mppb sparc elf objdump is present... yes $MPPB_ELF_OBJDUMP"
fi

AC_DEFINE_UNQUOTED(MPPB_ELF_OBJCOPY, "${MPPB_ELF_OBJCOPY}", "Define the mppb sparc elf objcopy")
if test "x$ac_sparc_elf_objcopy" = x; then
   echo "checking if mppb sparc elf objcopy is present... no"
   AC_MSG_ERROR([mppb sparc elf objcopy not found])
else
   echo "checking if mppb sparc elf objcopy is present... yes $MPPB_ELF_OBJCOPY"
fi

AC_PROVIDE([$0])
])

