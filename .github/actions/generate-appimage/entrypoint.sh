#!/bin/bash
###
#     AppImage builder script
#
#     The following script needs to be exectued from PandA Bambu repository root and 
#     take as arguments the configure options for the PandA Bambu build.
#     An appimage executable will be generated as an output.
#     Consider that the script will share the whole repository with the container.
#     
#     The script will look for frontend compilers in into the ./dist directory. 
#     All compiler binaries and librares found inside the aforementioned 
#     directory will be moved into the root filesystem of the container.
#
###
set -e
args="$@"
workspace_dir="$PWD"
dist_dir="$workspace_dir/dist"
build_dir="$workspace_dir/build"
ccache_dir="$workspace_dir/.ccache"
autoconf_cache_dir="$workspace_dir/.autoconf"

function cleanup {
   echo "::endgroup::"
   make clean
}
trap cleanup EXIT

ln -sf /usr/include/x86_64-linux-gnu/gmp.h /usr/include/gmp.h

echo "::group::Initialize workspace"
export PATH=/usr/lib/ccache:$PATH
mkdir -p ~/.ccache/
cat > ~/.ccache/ccache.conf << EOF
max_size = 5.0G
cache_dir = $ccache_dir
EOF
if [[ -d "$dist_dir" && ! -z "$(ls -A $dist_dir)" ]]; then
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

echo "::group::Build AppImage"
mkdir -p $build_dir
if [[ -d "$autoconf_cache_dir" ]]; then
   echo "Restoring autoconf cache"
   mv $autoconf_cache_dir/* $build_dir/.
fi
make appimage BUILD_DIR="$build_dir" PKG_DIR="$dist_dir" CONFIGURE_FLAGS="$args"
autoconf_caches=("`find $build_dir/. -type f -path '**/config.cache'`")
for cache in $autoconf_caches
do
   mirror_dir="$autoconf_cache_dir/$(dirname $cache)"
   mkdir -p $mirror_dir
   cp $cache $mirror_dir
done
echo "::endgroup::"

echo "appimage=$(ls *.AppImage)" >> $GITHUB_OUTPUT
echo "dist-dir=dist" >> $GITHUB_OUTPUT
