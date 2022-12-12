#!/bin/bash
set -e

workspace_dir="$PWD"
ccache_dir="$workspace_dir/.ccache"
autoconf_cache_dir="$workspace_dir/.autoconf"
report_dir="$1"
shift

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
cache_dir = $ccache_dir
EOF
if [[ -d "dist" ]]; then
   echo "Pre-initialized dist dir found. Installing system wide..."
   rsync -rtpl dist/. /
   rm -rf dist
fi

if [[ -d "compiler" ]]; then
   echo "Bambu compiler dir found. Installing system wide..."
   rsync -rtpl compiler/. /
   rm -rf compiler
fi

GCC_BINS=("`find /usr/bin -type f -regextype posix-extended -regex '.*g(cc|\+\+)-[0-9]+\.?[0-9]?'`")
CLANG_BINS=("`find /clang+llvm-*/bin -type f -regextype posix-extended -regex '.*clang-[0-9]+\.?[0-9]?'`")
CLANG_EXES=("clang" "clang++" "clang-cl" "clang-cpp" "ld.lld" "lld" "lld-link" "llvm-ar" "llvm-config" "llvm-dis" "llvm-link" "llvm-lto" "llvm-lto2" "llvm-ranlib" "mlir-opt" "mlir-translate" "opt" "scan-build")

for clang_exe in $CLANG_BINS
do
   CLANG_VER=$(sed 's/clang-//g' <<< "$(basename $clang_exe)")
   CLANG_DIR=$(dirname $clang_exe)
   echo "Generating system links for clang/llvm $CLANG_VER"
   for app in "${CLANG_EXES[@]}"
   do
      if [[ -f "$CLANG_DIR/$app" ]]; then
         ln -sf "$CLANG_DIR/$app" "/usr/bin/$app-$CLANG_VER"
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

max_gcc_ver="$(ls -x -v -1a /usr/include/c++ 2> /dev/null | tail -1)"
if [[ -z "${max_gcc_ver}" ]]
then
  echo "At least one gcc version must be there"
  exit -1
fi
echo "Latest bundled GCC version: ${max_gcc_ver}"
cd $workspace_dir

echo "Initializing build environment..."
make -f Makefile.init
echo "::endgroup::"

echo "::group::Configure build environment"
mkdir build
cd build
if [[ -d "$autoconf_cache_dir" ]]; then
   echo "Restoring autoconf cache"
   mv $autoconf_cache_dir/* .
fi
../configure --prefix=/usr -C $@
autoconf_caches=("`find . -type f -path '**/config.cache'`")
for cache in $autoconf_caches
do
   mirror_dir="$autoconf_cache_dir/$(dirname $cache)"
   mkdir -p $mirror_dir
   cp $cache $mirror_dir
done
cd ..
echo "::endgroup::"

echo "::group::Compile external libraries"
make --directory=build/ext -j$J
echo "::endgroup::"

echo "::group::Scan build Bambu sources"
scan-build-$CLANG_VERSION -v -v --use-cc=/usr/bin/clang-$CLANG_VERSION --use-c++=/usr/bin/clang++-$CLANG_VERSION --use-analyzer=/usr/bin/clang-$CLANG_VERSION -o "$report_dir" make --directory=build/src -j$J
echo "::endgroup"

mkdir -p "$report_dir"
echo "report-dir=$report_dir" >> $GITHUB_OUTPUT
