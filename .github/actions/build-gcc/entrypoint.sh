#!/bin/bash
set -e

BRANCH=$1
PATCH_FILE="$(compgen -G gcc-*.patch || echo '')"
DIST_NAME="$2-$(lsb_release -is)_$(lsb_release -rs)"
DESTDIR="$GITHUB_WORKSPACE/$DIST_NAME"
shift
shift

workspace_dir=$PWD

ln -s /usr/include/x86_64-linux-gnu/gmp.h /usr/include/gmp.h

function cleanup {
   cd $workspace_dir
   rm -rf gcc
}
trap cleanup EXIT

git clone --depth 1 --branch $BRANCH git://gcc.gnu.org/git/gcc.git

cd gcc
if [[ -f "../$PATCH_FILE" ]]; then
   git apply ../$PATCH_FILE
   echo "Patch '$PATCH_FILE' applied successfully"
fi
./contrib/download_prerequisites
mkdir build
cd build
../configure "$@"
make -j$J
make DESTDIR="$DESTDIR" install

# Fix libgcc location
version_specific_lib_dir="${DESTDIR}/usr/lib/gcc/x86_64-linux-gnu/$(ls ${DESTDIR}/usr/lib/gcc/x86_64-linux-gnu | head -n 1)"
for lib in $(find $DESTDIR -name 'libgcc_s.so*')
do
   case $lib in
      *lib64* )
         mv $lib ${version_specific_lib_dir}/ ;;
      *libx32* )
         mv $lib ${version_specific_lib_dir}/x32 ;;
      * )
         mv $lib ${version_specific_lib_dir}/32 ;;
   esac
done
rm -rf ${DESTDIR}/usr/lib/gcc/x86_64-linux-gnu/lib*
rm -rf ${DESTDIR}/usr/lib64

echo "dist-dir=$DIST_NAME" >> $GITHUB_OUTPUT
