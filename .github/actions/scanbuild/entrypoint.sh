#!/bin/bash
set -e

report_dir="$1"
shift
args="$@"

workspace_dir="$PWD"
ccache_dir="$workspace_dir/.ccache"
build_dir="$workspace_dir/build"
autoconf_cache_dir="$workspace_dir/.autoconf"

function cleanup {
   echo "::endgroup::"
   make clean
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
if [[ -d "$dist_dir" ]]; then
   echo "Pre-initialized dist dir found. Installing system wide..."
   rsync -rtpl $dist_dir/* /
   rm -rf $dist_dir/*
fi

if [[ "$(ls / | grep 'clang+llvm*' | wc -l)" -gt 0 ]]; then
   CLANG_BINS=("`find /clang+llvm-*/bin -type f -regextype posix-extended -regex '.*clang-[0-9]+\.?[0-9]?'`")
   for clang_exe in $CLANG_BINS
   do
      CLANG_VER=$(sed 's/clang-//g' <<< "$(basename $clang_exe)")
      CLANG_DIR=$(dirname $clang_exe)
      echo "Generating ccache alias for clang-$CLANG_VER"
      ln -sf ../../bin/ccache "/usr/lib/ccache/clang-$CLANG_VER"
      echo "Generating ccache alias for clang++-$CLANG_VER"
      ln -sf ../../bin/ccache "/usr/lib/ccache/clang++-$CLANG_VER"
   done
fi

GCC_BINS=("`find /usr/bin -type f -regextype posix-extended -regex '.*g(cc|\+\+)-[0-9]+\.?[0-9]?'`")
for compiler in $GCC_BINS
do
   echo "Generating ccache alias for $(basename $compiler)"
   ln -sf ../../bin/ccache "/usr/lib/ccache/$(basename $compiler)"
done
echo "::endgroup::"

echo "::group::Scan build sources"
mkdir -p $build_dir
if [[ -d "$autoconf_cache_dir" ]]; then
   echo "Restoring autoconf cache"
   mv $autoconf_cache_dir/* $build_dir/.
fi
mkdir -p "$report_dir"
make scan-build SCAN_BUILD_VERSION="$CLANG_VERSION" SCAN_BUILD_REPORT_DIR="$report_dir" CONFIGURE_FLAGS="$args"
autoconf_caches=("`find $build_dir/. -type f -path '**/config.cache'`")
for cache in $autoconf_caches
do
   mirror_dir="$autoconf_cache_dir/$(dirname $cache)"
   mkdir -p $mirror_dir
   cp $cache $mirror_dir
done
echo "::endgroup::"

echo "report-dir=$report_dir" >> $GITHUB_OUTPUT
