#!/bin/bash
set -e

workspace_dir=$PWD
jobs="$1"
shift

function cleanup {
   echo "::endgroup::"
   make --directory=$workspace_dir -f Makefile.init clean
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
   mv dist/. /
fi

GCC_BINS=("`find /usr/bin -type f -regextype posix-extended -regex '.*g(cc|\+\+)-[0-9]+\.?[0-9]?'`")
CLANG_BINS=("`find /clang+llvm-*/bin -type f -regextype posix-extended -regex '.*clang-[0-9]+\.?[0-9]?'`")
cd /usr/bin
for clang_exe in $CLANG_BINS
do
   CLANG_VER=$(sed 's/clang-//g' <<< "$(basename $clang_exe)")
   CLANG_DIR=$(dirname $clang_exe)
   echo "Generating system links for clang/llvm $CLANG_VER"
   ln -sf "$CLANG_DIR/clang-$CLANG_VER" "clang-$CLANG_VER"
   ln -sf "$CLANG_DIR/clang-$CLANG_VER" "clang++-$CLANG_VER"
   ln -sf "$CLANG_DIR/clang-$CLANG_VER" "clang-cpp-$CLANG_VER"
   ln -sf "$CLANG_DIR/llvm-config" "llvm-config-$CLANG_VER"
   ln -sf "$CLANG_DIR/llvm-link" "llvm-link-$CLANG_VER"
   ln -sf "$CLANG_DIR/opt" "opt-$CLANG_VER"
done
cd /usr/lib/ccache
for compiler in $GCC_BINS
do
   echo "Generating ccache alias for $(basename $compiler)"
   ln -sf ../../bin/ccache "$(basename $compiler)"
done
for compiler in $CLANG_BINS
do
   CLANG_VER=$(sed 's/clang-//g' <<< "$(basename $compiler)")
   echo "Generating ccache alias for clang-$CLANG_VER"
   ln -sf ../../bin/ccache "clang-$CLANG_VER"
   echo "Generating ccache alias for clang++-$CLANG_VER"
   ln -sf ../../bin/ccache "clang++-$CLANG_VER"
done
cd $workspace_dir
echo "::endgroup::"

echo "::group::Make distribution"
make -f Makefile.init dist J="$jobs"
echo "::endgroup::"
