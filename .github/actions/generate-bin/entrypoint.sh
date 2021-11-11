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
#!/bin/bash
set -e

workspace_dir=$PWD

function cleanup {
   echo "::endgroup::"
   make --directory=$workspace_dir -f Makefile.init clean
}
trap cleanup EXIT

#Necessary for gcc-4.5
export LIBRARY_PATH=/usr/lib/x86_64-linux-gnu
ln -s /usr/include/x86_64-linux-gnu/gmp.h /usr/include/gmp.h

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

echo "Initializing build environment..."
make -f Makefile.init
echo "::endgroup::"

echo "::group::Configure build environment"
mkdir build
cd build
../configure --prefix=/usr $@
cd ..
echo "::endgroup::"

echo "::group::Build Bambu"
make --directory=build -j$J install-strip DESTDIR="$workspace_dir/dist"
echo "::endgroup"

echo "::group::Package Appimage"

echo "Inflating libraries..."
mkdir dist/lib
mkdir dist/lib/x86_64-linux-gnu/
cp /lib/x86_64-linux-gnu/libtinfo.so.5.* dist/lib/x86_64-linux-gnu/
ln -s /lib/x86_64-linux-gnu/libtinfo.so.5.* dist/lib/x86_64-linux-gnu/libtinfo.so.5
cp /usr/lib/libbdd.so.0.0.0 dist/usr/lib/libbdd.so.0.0.0
cd dist/usr/lib/
ln -s libbdd.so.0.0.0 libbdd.so.0
cd ../../..

echo "Inflating metadata..."
cp style/img/panda.png.in dist/bambu.png
cat > dist/bambu.desktop << EOF
[Desktop Entry]
Name=bambu
Exec=tool_select.sh
Icon=bambu
Type=Application
Terminal=true
Categories=Development;
EOF
cat > dist/usr/bin/tool_select.sh << EOF
#!/bin/bash
export LC_ALL="C"
BINARY_NAME=\$(basename "\$ARGV0")
BINARY_PATH="\$APPDIR/usr/bin/\$BINARY_NAME"
if [ ! -e "\$BINARY_PATH" ]; then
   BINARY_PATH="\$APPDIR/usr/bin/bambu"
fi
\$BINARY_PATH "\$@"
EOF
chmod a+x dist/usr/bin/tool_select.sh

echo "Generating appimage..."
curl -L https://github.com/AppImage/AppImageKit/releases/download/continuous/AppRun-x86_64 -o dist/AppRun -s
chmod +x dist/AppRun
ARCH=x86_64 appimagetool dist 2> /dev/null

echo "::set-output name=appimage::$(ls *.AppImage)"
echo "::set-output name=dist-dir::dist"
