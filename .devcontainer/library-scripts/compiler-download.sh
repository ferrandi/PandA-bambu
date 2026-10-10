#!/usr/bin/env bash
set -e
export XZ_OPT='-T0'

install_dir="$1"
compilers_list="$2"
backup_dir="$3"

bambuhls_compiler_url="https://release.bambuhls.eu/compiler"
failure_file="${install_dir}/download_failed"

extract() {
   url="$1"
   filename="$(basename ${url})"
   backup_dir="$3"
   backup_file="${backup_dir}/${filename}"
   target_dir="$2"
   mkdir -p "${target_dir}"
   if test -d "${backup_dir}"; then
      if test ! -f "${backup_file}"; then
         # Storing into backup directory
         wget "${url}" -nv -t 5 -T 5 -O "${backup_file}"
      fi
      # Extracting from backup directory
      tar -C "${target_dir}" --no-same-owner -xJf "${backup_file}"
   else
      # Inflating target
      wget "${url}" -nv -t 5 -T 5 -O- | tar -C "${target_dir}" --no-same-owner -xJf -
   fi
}

inflate() {
   url=""
   case $1 in
      clang-19 )
         url="${bambuhls_compiler_url}/clang+llvm-19.1.7-x86_64-linux-gnu-compat.tar.xz"
         ;;
      clang-16 )
         url="${bambuhls_compiler_url}/clang+llvm-16.0.6-x86_64-oraclelinux_7.6.tar.xz"
         ;;
      clang-13 )
         url="${bambuhls_compiler_url}/clang+llvm-13.0.1-x86_64-oraclelinux_7.6.tar.xz"
         ;;
      clang-12 )
         url="${bambuhls_compiler_url}/clang+llvm-12.0.1-x86_64-oraclelinux_7.6.tar.xz"
         ;;
      clang-11 )
         url="${bambuhls_compiler_url}/clang+llvm-11.1.0-x86_64-oraclelinux_7.6.tar.xz"
         ;;
      clang-10 )
         url="${bambuhls_compiler_url}/clang+llvm-10.0.1-x86_64-oraclelinux_7.6.tar.xz"
         ;;
      clang-9 )
         url="${bambuhls_compiler_url}/clang+llvm-9.0.1-x86_64-oraclelinux_7.6.tar.xz"
         ;;
      clang-8 )
         url="${bambuhls_compiler_url}/clang+llvm-8.0.1-x86_64-oraclelinux_7.6.tar.xz"
         ;;
      clang-7 )
         url="${bambuhls_compiler_url}/clang+llvm-7.1.0-x86_64-oraclelinux_7.6.tar.xz"
         ;;
      clang-6.0 )
         url="${bambuhls_compiler_url}/clang+llvm-6.0.1-x86_64-oraclelinux_7.6.tar.xz"
         ;;
      clang-5.0 )
         url="${bambuhls_compiler_url}/clang+llvm-5.0.2-x86_64-oraclelinux_7.6.tar.xz"
         ;;
      clang-4.0 )
         url="${bambuhls_compiler_url}/clang+llvm-4.0.1-x86_64-oraclelinux_7.6.tar.xz"
         ;;
      gcc-8 )
         url="${bambuhls_compiler_url}/gcc-8.5.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz"
         ;;
      * )
         >&2 echo "ERROR: Unknown compiler required: $1"
         exit -1
         ;;
   esac
   extract "$url" "$2" "$3"
}

mkdir -p "${install_dir}"
install_dir="$(readlink -e ${install_dir})"
rm -f "${failure_file}"
if test ! -z "${backup_dir}"; then
   mkdir -p "${backup_dir}"
fi
install_list=""
IFS=','; for compiler in ${compilers_list}; do
   compiler_dir="${install_dir}/${compiler}"
   echo "Installing compiler ${compiler} into ${compiler_dir}"
   if test -d "${compiler_dir}"; then
      >&2 echo "WARNING: directory ${compiler_dir} already present, skipping..."
      continue
   fi
   install_list="${install_list}${install_list:+,}${compiler}"
   inflate "${compiler}" "${compiler_dir}" "${backup_dir}" || touch "${failure_file}" &
done
wait

if test -e "${failure_file}"; then
   exit -1
fi

CLANG_EXES="clang clang++ clang-cl clang-cpp ld.lld lld lld-link llvm-ar llvm-config llvm-dis llvm-link llvm-lto llvm-lto2 llvm-ranlib mlir-opt mlir-translate opt scan-build"
IFS=','; for compiler in ${install_list}; do
   if ! echo "${compiler}" | grep -q "clang"; then continue; fi
   clang_dir="${install_dir}/${compiler}"
   CLANG_VER=`sed 's/clang-//g' <<< "${compiler}"`
   CLANG_EXE=`find ${clang_dir} -type f -regextype posix-extended -regex '.*/bin/clang-'"${CLANG_VER}" 2> /dev/null`
   CLANG_DIR=`dirname ${CLANG_EXE}`
   CLANG_BIN="${CLANG_DIR#${clang_dir}/}"
   bin_dir=`sed 's|^[^/]\+/||' <<< "${CLANG_BIN}"`
   mkdir -p "${clang_dir}/${bin_dir}"
   RELATIVE_DIR=`sed 's#[^/][^/]*#..#g' <<< "${bin_dir}"`
   echo "Generating ${clang_dir}/${bin_dir} links for clang/llvm ${CLANG_VER}"
   IFS=' '; for app in ${CLANG_EXES}; do
      if [[ -f "${CLANG_DIR}/${app}" ]]; then
         echo "  ${app}-${CLANG_VER}... ok"
         ln -sf "${RELATIVE_DIR}/${CLANG_BIN}/${app}" "${clang_dir}/${bin_dir}/${app}-${CLANG_VER}"
      else
         echo "  ${app}-${CLANG_VER}... not available"
      fi
   done
done

SETTINGS="${install_dir}/settings.sh"
printf 'BAMBU_HLS_COMPILERS=$(dirname $(readlink -e ${BASH_SOURCE[0]:-${(%%):-%%x}}))\n' > ${SETTINGS}
IFS=$'\n'; for compiler_dir in `find "${install_dir}" -maxdepth 1 -mindepth 1 -type d | sort -V`; do
   IFS=$'\n'; rm -rf `find "${compiler_dir}" -name '*.la'` # I hate libtool
   relative_dir=`basename "${compiler_dir}"`
   printf "export PATH=\${BAMBU_HLS_COMPILERS}/${relative_dir}/bin:\$PATH\n" >> ${SETTINGS}
done
chmod +x "${SETTINGS}"
