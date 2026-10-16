#!/bin/bash -e

if [ -z "$GITHUB_OUTPUT" ]; then
  export GITHUB_OUTPUT=`mktemp`
  echo "Warning: Environment variable GITHUB_OUTPUT is not set."
  echo "Writing output variables to $GITHUB_OUTPUT"
fi

workspace_dir="$PWD"

REF="$1"
DIST_NAME="$2"
PATCH_FILE="${workspace_dir}/$(compgen -G llvmorg-*.patch || echo '')"
DESTDIR="${workspace_dir}/${DIST_NAME}/${DIST_NAME}"
shift
shift

echo "LLVM reference: ${REF}"
echo "Package name  : ${DIST_NAME}"
echo "Patch file   : ${PATCH_FILE}"
echo "Build CC      : `gcc --version|head -n 1`"
echo "Build CXX     : `g++ --version|head -n 1`"
echo "Build options : $@"

function cleanup {
   rm -rf "${workspace_dir}/llvm-project"
}
trap cleanup EXIT

git -c http.sslVerify=false clone --depth 1 --branch "${REF}" https://github.com/llvm/llvm-project.git llvm-project

cd llvm-project
if test -f "${PATCH_FILE}"; then
   git apply "${PATCH_FILE}"
   echo "Patch '$(basename ${PATCH_FILE})' applied successfully"
fi
eval cmake -B build llvm "$@" -DCMAKE_INSTALL_PREFIX="${DESTDIR}"
cmake --build build --target install --parallel $J

echo "dist-dir=${DESTDIR#${workspace_dir}/}" >> ${GITHUB_OUTPUT}
