#!/bin/bash
set -e

workspace_dir="$PWD"
report_dir="$1"
shift
if [[ -z "$J" ]]; then
J="1"
fi

function cleanup {
   echo "::endgroup::"
   make --directory=$workspace_dir -f Makefile.init clean
}
trap cleanup EXIT

CLANG_VERSION=""
for arg in $@
do
    if [[ "$arg" = CC=clang-* ]]; then
        CLANG_VERSION="$(sed 's/CC=clang-//g' <<<$arg)"
    fi
done
if [[ -z "$CLANG_VERSION" ]]; then
   echo "Could not infer clang version from configure arguments"
   exit -1
fi

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
   ln -sf "$CLANG_DIR/scan-build" "scan-build-$CLANG_VER"
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

echo "Initializing build environment..."
make -f Makefile.init
echo "::endgroup::"

echo "::group::Configure build environment"
mkdir build
cd build
../configure --prefix=/usr $@
cd ..
echo "::endgroup::"

echo "::group::Compile external libraries"
make --directory=build/ext -j$J
echo "::endgroup::"

echo "::group::Scan build Bambu sources"
scan-build-$CLANG_VERSION -v -v --use-cc=/usr/bin/clang-$CLANG_VERSION --use-c++=/usr/bin/clang++-$CLANG_VERSION --use-analyzer=/usr/bin/clang-$CLANG_VERSION -o "$report_dir" make --directory=build/src -j
echo "::endgroup"

mkdir -p "$report_dir"
echo "::set-output name=report-dir::$report_dir"
