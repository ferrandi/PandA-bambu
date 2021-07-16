#!/bin/bash
set -e

workspace_dir=$PWD

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
done
mkdir -p "$workspace_dir/dist/usr/bin"
cd "$workspace_dir/dist/usr/bin"
for clang_exe in $CLANG_BINS
do
   CLANG_VER=$(sed 's/clang-//g' <<< "$(basename $clang_exe)")
   CLANG_DIR=$(dirname $clang_exe)
   echo "Generating dist links for clang/llvm $CLANG_VER"
   ln -sf "../..$CLANG_DIR/clang-$CLANG_VER" "clang-$CLANG_VER"
   ln -sf "../..$CLANG_DIR/clang-$CLANG_VER" "clang++-$CLANG_VER"
   ln -sf "../..$CLANG_DIR/clang-$CLANG_VER" "clang-cpp-$CLANG_VER"
   ln -sf "../..$CLANG_DIR/llvm-config" "llvm-config-$CLANG_VER"
   ln -sf "../..$CLANG_DIR/llvm-link" "llvm-link-$CLANG_VER"
   ln -sf "../..$CLANG_DIR/opt" "opt-$CLANG_VER"
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

echo "::group::Build application package"
make --directory=build -j$J install-strip DESTDIR="$workspace_dir/dist"

echo "Inflating python interpreter..."
wget https://github.com/niess/python-appimage/releases/download/python3.9/python3.9.6-cp39-cp39-manylinux1_x86_64.AppImage -q
chmod +x python*-cp39-cp39-manylinux1_x86_64.AppImage
./python*-cp39-cp39-manylinux1_x86_64.AppImage --appimage-extract 2>&1> /dev/null
rm squashfs-root/python.png squashfs-root/python*.desktop squashfs-root/usr/share/metainfo/python*.appdata.xml squashfs-root/AppRun
rsync -a squashfs-root/ dist
rm -rf squashfs-root python*-cp39-cp39-manylinux1_x86_64.AppImage

rm -f `find dist -type f -name clang-tidy`
rm -f `find dist -type f -name clang-query`
rm -f `find dist -type f -name clang-change-namespace`
rm -f `find dist -type f -name clang-reorder-fields`
rm -f `find dist -type f -name clang-func-mapping`
rm -f `find dist -type f -name sancov`
rm -f dist/clang+llvm*/lib/*.a

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
Exec=bambu_wrapper.sh
Icon=bambu
Type=Application
Terminal=true
Categories=Development;
EOF
cat > dist/usr/bin/bambu_wrapper.sh << EOF
#!/bin/bash
export LC_ALL="C"
export PYTHONHOME=
export PYTHONPATH=
if [ "\$#" -eq 0 ]; then
   \$APPDIR/usr/bin/bambu
else
   \$APPDIR/usr/bin/bambu "\$@"
   return_value=\$?
   if test \$return_value != 0; then
      exit \$return_value
   fi
   exit 0
fi
EOF
chmod a+x dist/usr/bin/bambu_wrapper.sh

echo "Generating appimage..."
curl -L https://github.com/AppImage/AppImageKit/releases/download/continuous/AppRun-x86_64 -o dist/AppRun
chmod +x dist/AppRun
ARCH=x86_64 appimagetool dist 2> /dev/null

echo "::set-output name=appimage::$(ls *.AppImage)"
echo "::set-output name=dist-dir::dist"
echo "::endgroup::"
