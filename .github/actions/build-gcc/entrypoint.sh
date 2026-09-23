#!/bin/bash -e
if [ -z "$GITHUB_OUTPUT" ]; then
  export GITHUB_OUTPUT=`mktemp`
  echo "Warning: Environment variable GITHUB_OUTPUT is not set."
  echo "Writing output variables to $GITHUB_OUTPUT"
fi

workspace_dir="$PWD"

REF="$1"
DIST_NAME="$2"
PATCH_FILE="${workspace_dir}/$(compgen -G gcc-*.patch || echo '')"
DESTDIR="${workspace_dir}/${DIST_NAME}"
shift
shift

echo "GCC reference: ${REF}"
echo "Package name : ${DIST_NAME}"
echo "Patch file   : ${PATCH_FILE}"
echo "Build CC     : `gcc --version|head -n 1`"
echo "Build CXX    : `g++ --version|head -n 1`"
echo "Build options: $@"

function cleanup {
   rm -rf ${workspace_dir}/gcc
}
trap cleanup EXIT

git -c http.sslVerify=false clone --depth 1 --branch "${REF}" https://gcc.gnu.org/git/gcc.git gcc

cd gcc
if test -f "${PATCH_FILE}"; then
   git apply "${PATCH_FILE}"
   echo "Patch '$(basename ${PATCH_FILE})' applied successfully"
fi
./contrib/download_prerequisites
mkdir build
cd build
eval ../configure "$@" --prefix=/ --enable-version-specific-runtime-libs
make -j$J
make DESTDIR="$DESTDIR" install -j$J

# Fix libgcc location
version_specific_lib_dir="${DESTDIR}/lib/gcc/x86_64-linux-gnu/`ls ${DESTDIR}/lib/gcc/x86_64-linux-gnu | head -n 1`"
IFS=$'\n'; for lib in `find ${DESTDIR} -name 'libgcc_s.so*'`
do
   case $lib in
      *lib64* )
         mv ${lib} ${version_specific_lib_dir}/ ;;
      *libx32* )
         mv ${lib} ${version_specific_lib_dir}/x32 ;;
      * )
         mv ${lib} ${version_specific_lib_dir}/32 ;;
   esac
done
rm -rf ${DESTDIR}/lib/gcc/x86_64-linux-gnu/lib*
rm -rf ${DESTDIR}/lib64

# libtoolize should mind its own business and stop bloating my distributions
rm -f `find "${DESTDIR}" -name '*.la'`

echo "dist-dir=${DESTDIR#${workspace_dir}/}" >> ${GITHUB_OUTPUT}
