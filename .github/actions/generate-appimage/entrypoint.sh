#!/bin/bash
###
#     AppImage builder script
#
#     The following script needs to be exectued from PandA Bambu repository root and 
#     take as arguments the configure options for the PandA Bambu build.
#     An appimage executable will be generated as an output.
#     Consider that the script will share the whole repository with the container.
#     
#     The script will look for frontend compilers in into the ./dist and ./compiler 
#     directories. All compiler binaries and librares found inside the aforementioned 
#     directories will be copied into the root filesystem of the container, while only 
#     the ones from ./dist directory will be included into the generated AppImage.
#     When a compiler is added to the ./compiler directory only, it will not be available 
#     inside the final AppImage, thus it is important to exclude it during PandA Bambu
#     configuration, passing the correct configuration option (e.g. --with-gcc8=/bin/false
#     if gcc-8 is not to be cosidered during PandA Bambu compilation).
#
###
set -e

workspace_dir="$PWD"
dist_dir="$workspace_dir/dist"
build_dir="$workspace_dir/build"
ccache_dir="$workspace_dir/.ccache"
autoconf_cache_dir="$workspace_dir/.autoconf"

function cleanup {
   echo "::endgroup::"
   make --directory=$workspace_dir -f Makefile.init clean
}
trap cleanup EXIT

#Necessary for gcc-4.5
export LIBRARY_PATH=/usr/lib/x86_64-linux-gnu
ln -sf /usr/include/x86_64-linux-gnu/gmp.h /usr/include/gmp.h

echo "::group::Initialize workspace"
export PATH=/usr/lib/ccache:$PATH
mkdir -p ~/.ccache/
cat > ~/.ccache/ccache.conf << EOF
max_size = 5.0G
cache_dir = $ccache_dir
EOF
if [[ -d "$dist_dir" ]]; then
   echo "Pre-initialized dist dir found. Installing system wide..."
   cp -r $dist_dir/. /
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
find $dist_dir/clang+llvm-*/bin '(' -type f -o -type l ')' ! '(' $NO_DELETE ')' -delete
rm -f $dist_dir/clang+llvm*/lib/*.a
rm -rf $dist_dir/clang+llvm*/share
rm -rf $dist_dir/usr/share

mkdir -p "$dist_dir/usr/bin"
for clang_exe in $CLANG_BINS
do
   CLANG_VER=$(sed 's/clang-//g' <<< "$(basename $clang_exe)")
   CLANG_DIR=$(dirname $clang_exe)
   echo "Generating system links for clang/llvm $CLANG_VER"
   for app in "${CLANG_EXES[@]}"
   do
      if [[ -f "$CLANG_DIR/$app" ]]; then
         ln -sf "$CLANG_DIR/$app" "/usr/bin/$app-$CLANG_VER"
         ln -sf "../..$CLANG_DIR/$app" "$dist_dir/usr/bin/$app-$CLANG_VER"
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

max_gcc_ver="$(ls -x -v -1a $dist_dir/usr/include/c++ 2> /dev/null | tail -1)"
if [[ -z "${max_gcc_ver}" ]]
then
  echo "At least one gcc version must be bundled in the AppImage"
  exit -1
fi
echo "Latest bundled GCC version: ${max_gcc_ver}"

echo "Initializing build environment..."
make -f Makefile.init
echo "::endgroup::"

echo "::group::Configure build environment"
mkdir -p $build_dir
cd $build_dir
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
cd $workspace_dir
echo "::endgroup::"

echo "::group::Build Bambu"
make --directory=$build_dir -j$J install-strip DESTDIR="$dist_dir"
echo "::endgroup"

echo "::group::Package Appimage"

echo "Inflating libraries..."
mkdir $dist_dir/lib
mkdir $dist_dir/lib/x86_64-linux-gnu/
cp -d /lib/x86_64-linux-gnu/libtinfo.so* $dist_dir/lib/x86_64-linux-gnu/
cp -d /usr/lib/x86_64-linux-gnu/libicu*.so* $dist_dir/lib/x86_64-linux-gnu/
cp -d /usr/lib/libbdd.so* $dist_dir/usr/lib/

echo "Inflating metadata..."
cp style/img/panda.png.in $dist_dir/bambu.png
cat > $dist_dir/bambu.desktop << EOF
[Desktop Entry]
Name=bambu
Exec=tool_select.sh
Icon=bambu
Type=Application
Terminal=true
Categories=Development;
EOF
cat > $dist_dir/usr/bin/tool_select.sh << EOF
#!/bin/bash
export LC_ALL="C"
unset PYTHONHOME  # Python is not bundled with this AppImage
unset PYTHONPATH
BINARY_NAME=\$(basename "\$ARGV0")
BINARY_PATH="\$APPDIR/usr/bin/\$BINARY_NAME"
if [ "\$BINARY_NAME" == "debug_terminal" ]; then
   BINARY_PATH="/bin/bash"
fi
if [ ! -e "\$BINARY_PATH" ]; then
   BINARY_PATH="\$APPDIR/usr/bin/bambu"
fi
\$BINARY_PATH "\$@"
EOF
chmod a+x $dist_dir/usr/bin/tool_select.sh

echo "Generating appimage..."
curl -L https://github.com/AppImage/AppImageKit/releases/download/continuous/AppRun-x86_64 -o $dist_dir/AppRun -s
chmod +x $dist_dir/AppRun
ARCH=x86_64 appimagetool $dist_dir 2> /dev/null

echo "::set-output name=appimage::$(ls *.AppImage)"
echo "::set-output name=dist-dir::dist"
