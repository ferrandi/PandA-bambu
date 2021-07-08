#!/bin/bash
set -e

BRANCH=$1
DIST_NAME="$2-$(lsb_release -is)_$(lsb_release -rs)"
DIST_DIR="$GITHUB_WORKSPACE/$DIST_NAME"
shift
shift

git clone --depth 1 --branch $BRANCH git://gcc.gnu.org/git/gcc.git

cd gcc
./contrib/download_prerequisites
mkdir build
cd build
../configure $@
make -j bootstrap
make DESTDIR="$DIST_DIR" install

lsb_release -a >> "$DIST_DIR/VERSION"

echo "::set-output name=dist-dir::$DIST_NAME"