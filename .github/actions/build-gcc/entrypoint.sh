#!/bin/bash
set -e

BRANCH=$1
PATCH_FILE="$(compgen -G gcc-*.patch || echo '')"
DIST_NAME="$2-$(lsb_release -is)_$(lsb_release -rs)"
DIST_DIR="$GITHUB_WORKSPACE/$DIST_NAME"
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
../configure $@
make -j$J bootstrap
make DESTDIR="$DIST_DIR" install

lsb_release -a >> "$DIST_DIR/VERSION"

echo "dist-dir=$DIST_NAME" >> $GITHUB_OUTPUT
