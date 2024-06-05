#!/bin/bash
set -e

BRANCH=$1
DIST_NAME="$2-$(lsb_release -is)_$(lsb_release -rs)"
DIST_DIR="$GITHUB_WORKSPACE/$DIST_NAME/$DIST_NAME"
shift
shift

workspace_dir=$PWD

function cleanup {
   cd $workspace_dir
   rm -rf cmake llvm-project
}
trap cleanup EXIT

# Install CMake from source
git clone --depth 1 --branch v3.26.3 https://gitlab.kitware.com/cmake/cmake.git cmake
cd cmake
./bootstrap --parallel=$J -- -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_USE_OPENSSL=OFF
make -j$J
make install
cd ..

git clone --depth 1 --branch $BRANCH https://github.com/llvm/llvm-project.git

mkdir llvm-project/build
cd llvm-project/build
cmake -G "Unix Makefiles" ../llvm \
   -DCMAKE_INSTALL_PREFIX="$DIST_DIR" \
   -DCMAKE_BUILD_TYPE=Release \
   -DLLVM_STATIC_LINK_CXX_STDLIB=ON \
   -DLLVM_TARGETS_TO_BUILD=X86 \
   -DLLVM_ENABLE_PROJECTS="$@"
make -j$J
make install

lsb_release -a >> "$DIST_DIR/VERSION"

echo "dist-dir=$DIST_NAME" >> $GITHUB_OUTPUT
