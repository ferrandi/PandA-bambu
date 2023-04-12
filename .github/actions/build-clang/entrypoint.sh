#!/bin/bash
set -e

BRANCH=$1
DIST_NAME="$2-$(lsb_release -is)_$(lsb_release -rs)"
DIST_DIR="$GITHUB_WORKSPACE/$DIST_NAME"
shift
shift

workspace_dir=$PWD


function cleanup {
   cd $workspace_dir
   rm -rf llvm-project
}
trap cleanup EXIT

git clone --depth 1 --branch $BRANCH https://github.com/llvm/llvm-project.git

cd llvm-project
mkdir build
cd build
cmake $@ -G "Unix Makefile" ../llvm
make -j$J 
make DESTDIR="$DIST_DIR" install

lsb_release -a >> "$DIST_DIR/VERSION"

echo "dist-dir=$DIST_NAME" >> $GITHUB_OUTPUT
