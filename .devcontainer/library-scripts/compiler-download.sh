#!/bin/bash
set -e

export XZ_OPT='-T0'

install_dir="$1"
compilers_list="$2"
backup_dir="$3"

if [ ! -z "${backup_dir}" ]; then
   echo "Creating backup directory"
   mkdir -p ${backup_dir}
fi

bambuhls_compiler_url="https://release.bambuhls.eu/compiler"
wget_opt="-nv -t 5 -T 5 -O -"
failure_file="${install_dir}/download_failed"

extract() {
   url="$1"
   filename="$(basename ${url})"
   backup_file="$3/${filename}"
   if [ -d "$3" ]; then
      if [ ! -f "${backup_file}" ]; then
         echo "Storing ${filename} into backup directory"
         wget ${url} -nv -t 5 -T 5 -O ${backup_file}
      fi
      echo "Extracting ${filename} from backup directory"
      tar -C $2 --no-same-owner -xJf ${backup_file}
   else
      echo "Inflating ${filename} from ${url}"
      wget ${url} $wget_opt | tar -C $2 --no-same-owner -xJf -
   fi
}

inflate() {
   echo "Installing $1 into $2"
   url=""
   case $1 in
      clang-16 )
         url="https://github.com/llvm/llvm-project/releases/download/llvmorg-16.0.0/clang+llvm-16.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz"
         ;;
      clang-13 )
         url="https://github.com/llvm/llvm-project/releases/download/llvmorg-13.0.1/clang+llvm-13.0.1-x86_64-linux-gnu-ubuntu-18.04.tar.xz"
         ;;
      clang-12 )
         url="https://github.com/llvm/llvm-project/releases/download/llvmorg-12.0.1/clang+llvm-12.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz"
         ;;
      clang-11 )
         url="https://github.com/llvm/llvm-project/releases/download/llvmorg-11.1.0/clang+llvm-11.1.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz"
         ;;
      clang-10 )
         url="https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.1/clang+llvm-10.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz"
         ;;
      clang-9 )
         url="https://github.com/llvm/llvm-project/releases/download/llvmorg-9.0.1/clang+llvm-9.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz"
         ;;
      clang-8 )
         url="https://releases.llvm.org/8.0.0/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz"
         ;;
      clang-7 )
         url="https://releases.llvm.org/7.0.1/clang+llvm-7.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz"
         ;;
      clang-6.0 )
         url="https://releases.llvm.org/6.0.1/clang+llvm-6.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz"
         ;;
      clang-5.0 )
         url="https://releases.llvm.org/5.0.2/clang+llvm-5.0.2-x86_64-linux-gnu-ubuntu-16.04.tar.xz"
         ;;
      clang-4.0 )
         url="https://releases.llvm.org/4.0.0/clang+llvm-4.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz"
         ;;
      gcc-4.9 )
         url="${bambuhls_compiler_url}/gcc-4.9-bambu-Ubuntu_16.04.tar.xz"
         ;;
      gcc-5 )
         url="${bambuhls_compiler_url}/gcc-5-bambu-Ubuntu_16.04.tar.xz"
         ;;
      gcc-6 )
         url="${bambuhls_compiler_url}/gcc-6-bambu-Ubuntu_16.04.tar.xz"
         ;;
      gcc-7 )
         url="${bambuhls_compiler_url}/gcc-7-bambu-Ubuntu_16.04.tar.xz"
         ;;
      gcc-8 )
         url="${bambuhls_compiler_url}/gcc-8-bambu-Ubuntu_16.04.tar.xz"
         ;;
      * )
         echo "Unknown compiler required"
         exit -1
         ;;
   esac
   extract $url $2 $3
} 
IFS=',' read -r -a compilers <<< "${compilers_list}"
compilers=( $(IFS=$'\n'; echo "${compilers[*]}" | sort -V) )
for compiler in "${compilers[@]}"
do
   echo "Required compiler: ${compiler}"
   case "${compiler}" in
      "gcc-"*)
         inflate ${compiler} ${install_dir} ${backup_dir} ;;
      *)
         inflate ${compiler} ${install_dir} ${backup_dir} || touch ${failure_file} & ;;
   esac
done
wait

if [ -e "${failure_file}" ]; then
   exit -1
fi

CLANG_BINS=("`find ${install_dir}/clang+llvm-*/bin -type f -regextype posix-extended -regex '.*clang-[0-9]+\.?[0-9]?' 2> /dev/null`")
CLANG_EXES=("clang" "clang++" "clang-cl" "clang-cpp" "ld.lld" "lld" "lld-link" "llvm-ar" "llvm-config" "llvm-dis" "llvm-link" "llvm-lto" "llvm-lto2" "llvm-ranlib" "mlir-opt" "mlir-translate" "opt" "scan-build")

mkdir -p "${install_dir}/usr/bin"
for clang_exe in ${CLANG_BINS}
do
   CLANG_VER=$(sed 's/clang-//g' <<< "$(basename ${clang_exe})")
   CLANG_DIR=$(dirname ${clang_exe})
   CLANG_BIN=${CLANG_DIR#${install_dir}}
   echo "Generating ${install_dir}/usr/bin links for clang/llvm ${CLANG_VER}"
   for app in "${CLANG_EXES[@]}"
   do
      if [[ -f "${CLANG_DIR}/${app}" ]]; then
         echo " ${app}-${CLANG_VER}... ok"
         ln -sf "../..${CLANG_BIN}/${app}" "${install_dir}/usr/bin/${app}-${CLANG_VER}"
      else
         echo " ${app}-${CLANG_VER}... not available"
      fi
   done
done
