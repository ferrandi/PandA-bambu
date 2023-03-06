#!/bin/bash
set -e

workspace_dir=$PWD
dist_dir="$workspace_dir/panda_dist"
ccache_dir="$workspace_dir/.ccache"
autoconf_cache_dir="$workspace_dir/.autoconf"

function build_all() {
   local build_name="$1"
   shift
   local build_dir="$PWD/obj_$build_name"
   local ac_cache_dir="$autoconf_cache_dir/obj_$build_name"
   mkdir -p $build_dir
   cd $build_dir
   echo "::group::Configure dist environment"
   if [[ -d "$ac_cache_dir" ]]; then
      echo "Restoring autoconf cache"
      mv $ac_cache_dir/* .
   fi
   ../configure --prefix=$PWD/install -C $@
   autoconf_caches=("`find . -type f -path '**/config.cache'`")
   for cache in $autoconf_caches
   do
      mirror_dir="$ac_cache_dir/$(dirname $cache)"
      mkdir -p $mirror_dir
      cp $cache $mirror_dir
   done
   cd ..

   echo "Make dist"
   make --directory=$build_dir -j$J

   echo "Make documentation"
   make --directory=$build_dir -j$J documentation

   echo "Make install"
   make --directory=$build_dir install
}

function cleanup {
   echo "::endgroup::"
   cd $workspace_dir
   rm -r $dist_dir
}
trap cleanup EXIT

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
CLANG_EXES=("clang" "clang++" "clang-cl" "clang-cpp" "ld.lld" "lld" "lld-link" "llvm-ar" "llvm-config" "llvm-dis" "llvm-link" "llvm-lto" "llvm-lto2" "llvm-ranlib" "mlir-opt" "mlir-translate" "opt")

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

echo "Initializing build environment..."
make -f Makefile.init
echo "::endgroup::"

mkdir -p $dist_dir
cd $dist_dir
echo "::group::Configure dist environment"
if [[ -d "$autoconf_cache_dir" ]]; then
   echo "Restoring autoconf cache"
   mv $autoconf_cache_dir/* .
fi
../configure --prefix=$PWD/install -C $@
autoconf_caches=("`find . -type f -path '**/config.cache'`")
for cache in $autoconf_caches
do
   mirror_dir="$autoconf_cache_dir/$(dirname $cache)"
   mkdir -p $mirror_dir
   cp $cache $mirror_dir
done
cd ..
echo "::endgroup::"

echo "::group::Make distribution"
make --directory=$dist_dir -j$J dist
echo "::endgroup::"

echo "::group::Initialize dist environment"
tar xvf $dist_dir/panda-*.tar.gz
cd panda-*
echo "Initializing build environment..."
make -f Makefile.init
echo "::endgroup::"

echo "::group::Build dist release"
build_all "release" $@ --enable-Werror --enable-release
echo "::endgroup::"

echo "::group::Build dist non-release"
build_all "non-release" $@ --enable-Werror --disable-release
