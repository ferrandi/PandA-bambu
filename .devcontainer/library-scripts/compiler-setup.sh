#!/bin/bash
###
#     Build environment setup
#
#     Download requested compilers and generate neccessary symlinks to enable ccache.
#     The script takes a single argument containing a comma separated list of compilers.
#     It is safe to call the script multiple times, but be aware that installing older
#     versions of GCC compilers after newer ones may cause compatibility issues.
###
set -e

ln -sf /usr/include/x86_64-linux-gnu/gmp.h /usr/include/gmp.h

GCC_BINS=("`find /usr/bin -type f -regextype posix-extended -regex '.*g(cc|\+\+)-[0-9]+\.?[0-9]?'`")
CLANG_BINS=("`find /clang+llvm-*/bin -type f -regextype posix-extended -regex '.*clang-[0-9]+\.?[0-9]?'`")
CLANG_EXES=("clang" "clang++" "clang-cl" "clang-cpp" "ld.lld" "lld" "lld-link" "llvm-ar" "llvm-config" "llvm-dis" "llvm-link" "llvm-lto" "llvm-lto2" "llvm-ranlib" "mlir-opt" "mlir-translate" "opt")

for clang_exe in ${CLANG_BINS}
do
   CLANG_VER=$(sed 's/clang-//g' <<< "$(basename ${clang_exe})")
   CLANG_DIR=$(dirname ${clang_exe})
   echo "Generating system links for clang/llvm ${CLANG_VER}"
   for app in "${CLANG_EXES[@]}"
   do
      if [[ -f "${CLANG_DIR}/${app}" ]]; then
         ln -sf "${CLANG_DIR}/${app}" "/usr/bin/${app}-${CLANG_VER}"
      fi
   done
   echo "Generating ccache alias for clang-${CLANG_VER}"
   ln -sf ../../bin/ccache "/usr/lib/ccache/clang-${CLANG_VER}"
   echo "Generating ccache alias for clang++-${CLANG_VER}"
   ln -sf ../../bin/ccache "/usr/lib/ccache/clang++-${CLANG_VER}"
done

for compiler in ${GCC_BINS}
do
   echo "Generating ccache alias for $(basename ${compiler})"
   ln -sf ../../bin/ccache "/usr/lib/ccache/$(basename ${compiler})"
done
