#!/bin/bash
#####################################################################
# AppImage build environment setup
#
# The following script takes as arguments the appimage package 
# directory and the list of compilers to include into the appimage.
#####################################################################
set -e
script_dir="$(dirname $(readlink -e $0))"
repo_dir="${script_dir}/../.."

pkg_dir="$1"

mkdir -p ${pkg_dir}

echo "Inifating compilers..."
bash ${script_dir}/compiler-download.sh $@

GCC_BINS=("`find ${pkg_dir}/usr/bin -type f -regextype posix-extended -regex '.*g(cc|\+\+)-[0-9]+\.?[0-9]?' 2> /dev/null`")
CLANG_BINS=("`find ${pkg_dir}/clang+llvm-*/bin -type f -regextype posix-extended -regex '.*clang-[0-9]+\.?[0-9]?' 2> /dev/null`")
CLANG_EXES=("clang" "clang++" "clang-cl" "clang-cpp" "ld.lld" "lld" "lld-link" "llvm-ar" "llvm-config" "llvm-dis" "llvm-link" "llvm-lto" "llvm-lto2" "llvm-ranlib" "mlir-opt" "mlir-translate" "opt")
NO_DELETE="-name clang"
for bin in ${CLANG_BINS}
do
   NO_DELETE+=" -o -name $(basename $bin)"
done
NO_DELETE+=" ${CLANG_EXES[@]/#/-o -name }"
find ${pkg_dir}/clang+llvm-*/bin '(' -type f -o -type l ')' ! '(' ${NO_DELETE} ')' -delete
rm -f ${pkg_dir}/clang+llvm*/lib/*.a
rm -rf ${pkg_dir}/clang+llvm*/share
rm -rf ${pkg_dir}/usr/share

mkdir -p "${pkg_dir}/usr/bin"
for clang_exe in ${CLANG_BINS}
do
   CLANG_VER=$(sed 's/clang-//g' <<< "$(basename ${clang_exe})")
   CLANG_DIR=$(dirname ${clang_exe#${pkg_dir}})
   echo "Generating usr/bin links for clang/llvm ${CLANG_VER}"
   for app in "${CLANG_EXES[@]}"
   do
      if [[ -f "${CLANG_DIR}/${app}" ]]; then
         ln -sf "../..${CLANG_DIR}/${app}" "${pkg_dir}/usr/bin/${app}-${CLANG_VER}"
      fi
   done
done

max_gcc_ver="$(ls -x -v -1a ${pkg_dir}/usr/include/c++ 2> /dev/null | tail -1)"
if [[ -z "${max_gcc_ver}" ]]
then
  echo "At least one gcc version must be bundled in the AppImage"
  exit -1
fi
echo "Latest bundled GCC version: ${max_gcc_ver}"

echo "Inflating libraries..."
mkdir -p ${pkg_dir}/lib/x86_64-linux-gnu/
cp -d /lib/x86_64-linux-gnu/libtinfo.so* ${pkg_dir}/lib/x86_64-linux-gnu/
cp -d /usr/lib/x86_64-linux-gnu/libicu*.so* ${pkg_dir}/lib/x86_64-linux-gnu/
cp -d /usr/lib/libbdd.so* ${pkg_dir}/usr/lib/

echo "Inflating metadata..."
cp ${repo_dir}/style/img/panda.png.in ${pkg_dir}/bambu.png
cat > ${pkg_dir}/bambu.desktop << EOF
[Desktop Entry]
Name=bambu
Exec=tool_select.sh
Icon=bambu
Type=Application
Terminal=true
Categories=Development;
EOF
# Change back to OWD - Original Working Directory
# Set by AppImage
# see https://github.com/AppImage/AppImageKit/blob/master/src/runtime.c
cat > ${pkg_dir}/usr/bin/tool_select.sh << EOF
#!/bin/bash
export LC_ALL="C"
unset PYTHONHOME  # Python is not bundled with this AppImage
unset PYTHONPATH
BINARY_NAME=\$(basename "\${ARGV0}")
BINARY_PATH="\${APPDIR}/usr/bin/\${BINARY_NAME}"
if [ "\$BINARY_NAME" == "\$(basename \${SHELL})" ]; then
   BINARY_PATH="\${SHELL}"
fi
if [ ! -e "\$BINARY_PATH" ]; then
   BINARY_PATH="\$APPDIR/usr/bin/bambu"
fi
cd "\$OWD"
\$BINARY_PATH "\$@"
EOF
chmod a+x ${pkg_dir}/usr/bin/tool_select.sh

echo "Inflating AppImage runtime..."
curl -L https://github.com/AppImage/AppImageKit/releases/download/continuous/AppRun-x86_64 -o ${pkg_dir}/AppRun -s
chmod +x ${pkg_dir}/AppRun

echo "AppImage package environment ready"
