#!/bin/bash
set -e

workspace_dir=$PWD

function cleanup {
   echo "::endgroup::"
   make --directory=$workspace_dir -f etc/scripts/Makeall clean
}
trap cleanup EXIT

echo "::group::Initialize workspace"
export PATH=/usr/lib/ccache:$PATH
mkdir -p ~/.ccache/
cat > ~/.ccache/ccache.conf << EOF
max_size = 5.0G
cache_dir = $workspace_dir/.ccache
EOF
if [[ -d "dist" ]]; then
   echo "Pre-initialized dist dir found. Installing system wide..."
   cp -r dist/. /
fi

if [[ -d "compiler" ]]; then
   echo "Bambu compiler dir found. Installing system wide..."
   cp -r compiler/. /
fi

GCC_BINS=("`find /usr/bin -type f -regextype posix-extended -regex '.*g(cc|\+\+)-[0-9]+\.?[0-9]?'`")
CLANG_BINS=("`find /clang+llvm-*/bin -type f -regextype posix-extended -regex '.*clang-[0-9]+\.?[0-9]?'`")
CLANG_EXES=("clang" "clang++" "clang-cl" "clang-cpp" "ld.lld" "lld" "lld-link" "llvm-ar" "llvm-config" "llvm-dis" "llvm-link" "llvm-lto" "llvm-lto2" "llvm-ranlib" "mlir-opt" "mlir-translate" "opt")
NO_DELETE="-name clang"
for bin in $CLANG_BINS
do
   NO_DELETE+=" -o -name $(basename $bin)"
done
NO_DELETE+=" ${CLANG_EXES[@]/#/-o -name }"
find dist/clang+llvm-*/bin '(' -type f -o -type l ')' ! '(' $NO_DELETE ')' -delete
rm -f dist/clang+llvm*/lib/*.a
rm -rf dist/clang+llvm*/share
rm -rf dist/usr/share

mkdir -p "$workspace_dir/dist/usr/bin"
for clang_exe in $CLANG_BINS
do
   CLANG_VER=$(sed 's/clang-//g' <<< "$(basename $clang_exe)")
   CLANG_DIR=$(dirname $clang_exe)
   echo "Generating system links for clang/llvm $CLANG_VER"
   for app in "${CLANG_EXES[@]}"
   do
      if [[ -f "$CLANG_DIR/$app" ]]; then
         ln -sf "$CLANG_DIR/$app" "/usr/bin/$app-$CLANG_VER"
         ln -sf "../..$CLANG_DIR/$app" "$workspace_dir/dist/usr/bin/$app-$CLANG_VER"
      fi
   done
   echo "Generating ccache alias for clang-$CLANG_VER"
   ln -sf ../../bin/ccache "/usr/lib/ccache/clang-$CLANG_VER"
   echo "Generating ccache alias for clang++-$CLANG_VER"
   ln -sf ../../bin/ccache "/usr/lib/ccache/clang++-$CLANG_VER"
done

for compiler in $GCC_BINS
do
   echo "Generating ccache alias for $(basename $compiler)"
   ln -sf ../../bin/ccache "/usr/lib/ccache/$(basename $compiler)"
done

max_gcc_ver="$(ls -x -v -1a dist/usr/include/c++ 2> /dev/null | tail -1)"
if [[ -z "${max_gcc_ver}" ]]
then
  echo "At least one gcc version must be bundled in the AppImage"
  exit -1
fi
echo "Latest bundled GCC version: ${max_gcc_ver}"
echo "::endgroup::"

echo "::group::Make distribution"
CONFIGURE_OPTIONS="$@"
make -f etc/scripts/Makeall dist J=$J CONFIGURE_OPTIONS="$CONFIGURE_OPTIONS"
