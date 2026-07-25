#!/bin/bash -e
set -o pipefail
###
#     PandA builder script
#
#     The following script needs to be exectued from PandA Bambu repository root and 
#     take as arguments the configure options for the PandA Bambu build.
#     
#     A settings.sh file will be sourced if present in ./compilers directory
#     The script will look for frontend compilers in PATH directories.
#
###

if [ -z "$GITHUB_OUTPUT" ]; then
  export GITHUB_OUTPUT=`mktemp`
  echo "Warning: Environment variable GITHUB_OUTPUT is not set."
  echo "Writing output variables to $GITHUB_OUTPUT"
fi

if test $# -eq 1; then
  # GitHub Action passes config-args as a single string; split it into an array.
  read -r -a ARGS <<< "$1"
else
  ARGS=("$@")
fi
WORKSPACE_DIR="$PWD"
BUILD_DIR="${WORKSPACE_DIR}/build"
DIST_DIR="${WORKSPACE_DIR}/panda_dist"
COMPILERS_DIR="${WORKSPACE_DIR}/compilers"
CCACHE_DIR="${WORKSPACE_DIR}/.ccache"
APPIMAGE_NAME="bambu"
APPIMAGE_ENABLED=false
APPIMAGE_RUNTIME_FILE=""

function cleanup {
   local status=$?
   echo "::endgroup::"
   trap - EXIT
   exit ${status}
}
trap cleanup EXIT

echo "::group::Initialize workspace"
if test -d "${COMPILERS_DIR}"; then
   echo "Pre-initialized AppImage dist directory found"
   if test -e "${COMPILERS_DIR}/settings.sh"; then source "${COMPILERS_DIR}/settings.sh"; fi
fi

PATH="/usr/lib/ccache:$PATH"
export PATH
export CCACHE_DIR
mkdir -p "${CCACHE_DIR}"
cat > ${CCACHE_DIR}/ccache.conf << EOF
max_size = 5.0G
cache_dir = ${CCACHE_DIR}
EOF
echo "::endgroup::"

echo "::group::Verify Clang/LLVM frontend development environment"
dpkg-query -W clang-16 libclang-16-dev llvm-16 llvm-16-dev
for required_path in \
   /usr/lib/llvm-16/include/clang/AST/ASTConsumer.h \
   /usr/lib/llvm-16/include/clang/Frontend/FrontendPluginRegistry.h \
   /usr/lib/llvm-16/include/llvm/IR/Module.h \
   /usr/lib/llvm-16/lib/cmake/llvm/LLVMConfig.cmake \
   /usr/lib/llvm-16/lib/cmake/clang/ClangConfig.cmake; do
   if test ! -f "${required_path}"; then
      echo "::error file=${required_path}::Required LLVM/Clang development file is missing."
      exit 1
   fi
   ls -l "${required_path}"
done
clang-16 --version
clang++-16 --version
clang-cpp-16 --version
llvm-config-16 --version
llvm-link-16 --version
llvm-ar-16 --version
opt-16 --version
verilator --version
echo "::endgroup::"

echo "::group::Configure PandA build (CMake)"
mkdir -p "${BUILD_DIR}" "${DIST_DIR}"

# Map legacy configure-style flags to CMake options.
DIST_COMPILERS=""
CMAKE_ARGS=()
for arg in "${ARGS[@]}"; do
   case "${arg}" in
      --enable-release) CMAKE_ARGS+=("-DPANDA_ENABLE_RELEASE=ON");;
      --disable-release) CMAKE_ARGS+=("-DPANDA_ENABLE_RELEASE=OFF");;
      --enable-asserts) CMAKE_ARGS+=("-DPANDA_ENABLE_ASSERTS=ON");;
      --disable-asserts) CMAKE_ARGS+=("-DPANDA_ENABLE_ASSERTS=OFF");;
      --enable-Werror) CMAKE_ARGS+=("-DPANDA_ENABLE_WERROR=ON");;
      --disable-Werror) CMAKE_ARGS+=("-DPANDA_ENABLE_WERROR=OFF");;
      --enable-debug) CMAKE_ARGS+=("-DPANDA_ENABLE_DEBUG=ON");;
      --disable-debug) CMAKE_ARGS+=("-DPANDA_ENABLE_DEBUG=OFF");;
      --with-compilers=*) DIST_COMPILERS="${arg#*=}";;
      --with-appimage=*) APPIMAGE_NAME="${arg#*=}"; APPIMAGE_ENABLED=true;;
      --with-appimage) APPIMAGE_ENABLED=true;;
      -DPANDA_APPIMAGE_NAME=*) APPIMAGE_NAME="${arg#*=}"; APPIMAGE_ENABLED=true; CMAKE_ARGS+=("${arg}");;
      -DPANDA_APPIMAGE_RUNTIME_FILE=*) APPIMAGE_RUNTIME_FILE="${arg#*=}"; APPIMAGE_ENABLED=true;;
      --with-opt-level=*) ;; # handled by CMAKE_BUILD_TYPE already
      --enable-opt|--disable-opt) ;; # no-op in CMake path
      *) CMAKE_ARGS+=("${arg}");;
   esac
done

if test -n "${DIST_COMPILERS}"; then
   CMAKE_ARGS+=("-DPANDA_DIST_COMPILERS=${DIST_COMPILERS}")
   if test -n "${LOCAL_COMPILERS_BACKUP}"; then
      CMAKE_ARGS+=("-DPANDA_DIST_COMPILERS_BACKUP=${LOCAL_COMPILERS_BACKUP}")
   fi
fi

CMAKE_ARGS+=(
   "-DCMAKE_BUILD_TYPE=Release"
   "-DCMAKE_INSTALL_MESSAGE=LAZY"
   "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
   "-DPANDA_BUILD_BAMBU=ON"
   "-DPANDA_BUILD_CC=ON"
   "-DPANDA_BUILD_EUCALYPTUS=ON"
)

if ${APPIMAGE_ENABLED}; then
   APPIMAGE_ROOT="${BUILD_DIR}/appimage-root"
   CMAKE_ARGS+=("-DPANDA_APPIMAGE_NAME=${APPIMAGE_NAME}" "-DPANDA_APPIMAGE_ROOT=${APPIMAGE_ROOT}")
   CMAKE_ARGS+=("-DCMAKE_INSTALL_PREFIX=${APPIMAGE_ROOT}/usr")
   DIST_DIR="${APPIMAGE_ROOT}"
else
   CMAKE_ARGS+=("-DCMAKE_INSTALL_PREFIX=${DIST_DIR}")
fi

cmake -S "${WORKSPACE_DIR}" -B "${BUILD_DIR}" "${CMAKE_ARGS[@]}"
echo "::endgroup::"

echo "::group::Resolve selected Clang frontend"
CONFIG_HEADERS_DIR="${BUILD_DIR}/config_headers"
FRONTEND_COMPILER="$(sed -n 's/^#define LIBBAMBU_COMPILER "\(.*\)"/\1/p' "${CONFIG_HEADERS_DIR}/config_LIBBAMBU_COMPILER.hpp")"
CLANG_PLUGIN_SUBDIR="$(sed -n 's/^#define LIBBAMBU_COMPILER_DIR "\(.*\)"/\1/p' "${CONFIG_HEADERS_DIR}/config_LIBBAMBU_COMPILER_DIR.hpp")"
if test -z "${FRONTEND_COMPILER}" || test -z "${CLANG_PLUGIN_SUBDIR}"; then
   echo "::error::Unable to resolve the selected frontend compiler and plugin directory."
   exit 1
fi
FRONTEND_VERSION="${FRONTEND_COMPILER#I386_CLANG}"
CLANG_PLUGIN_DIR="${BUILD_DIR}/${CLANG_PLUGIN_SUBDIR}"
echo "Selected frontend: ${FRONTEND_COMPILER}"
echo "Expected plugin directory: ${CLANG_PLUGIN_DIR}"
echo "::endgroup::"

PLUGIN_TARGETS=(
   "clang_plugin_${FRONTEND_VERSION}_ast"
   "clang_plugin_${FRONTEND_VERSION}_customsroa"
   "clang_plugin_${FRONTEND_VERSION}_expandmemops"
   "clang_plugin_${FRONTEND_VERSION}_ssa"
)
echo "::group::Build selected PandA Clang plugins"
cmake --build "${BUILD_DIR}" --target "${PLUGIN_TARGETS[@]}" --parallel "${J:-1}"
echo "::endgroup::"

echo "::group::Verify PandA Clang plugins in ${CLANG_PLUGIN_DIR}"
for plugin in ASTAnalyzer.so customSROA.so expandMemOps.so dumpBambuIrSSA.so; do
   plugin_path="${CLANG_PLUGIN_DIR}/${plugin}"
   if test ! -f "${plugin_path}"; then
      echo "::error file=${plugin_path}::Required PandA Clang plugin is missing."
      exit 1
   fi
   file "${plugin_path}"
   ldd "${plugin_path}" | tee "${BUILD_DIR}/${plugin}.ldd.txt"
   if grep -q "not found" "${BUILD_DIR}/${plugin}.ldd.txt"; then
      echo "::error file=${plugin_path}::PandA Clang plugin has unresolved shared-library dependencies."
      exit 1
   fi
done
echo "::endgroup::"

echo "::group::Build PandA project"
cmake --build "${BUILD_DIR}" --parallel "${J:-1}"
echo "::endgroup::"

if test -e "${BUILD_DIR}/compile_commands.json"; then
   echo "::group::Export Compilation Database"
   COMPILATION_DB="${WORKSPACE_DIR}/compilation_db"
   mkdir -p "${COMPILATION_DB}/build"
   cp -r "${BUILD_DIR}/config_headers" "${COMPILATION_DB}/build/"
   mv "${BUILD_DIR}/compile_commands.json" "${COMPILATION_DB}/build/"
   echo "compilation-db=${COMPILATION_DB#${WORKSPACE_DIR}/}" >> ${GITHUB_OUTPUT}
   echo "::endgroup::"
fi

echo "::group::Package PandA distribution"
cmake --install "${BUILD_DIR}" --strip

if ${APPIMAGE_ENABLED}; then
   if cmake --build "${BUILD_DIR}" --target appimage_bundle; then
      APPIMAGE_PATH="${APPIMAGE_ROOT}/${APPIMAGE_NAME}.AppImage"
      if test -f "${APPIMAGE_PATH}"; then
         mkdir -p "${DIST_DIR}/bin"
         ln -sf "$(basename "${APPIMAGE_PATH}")" "${DIST_DIR}/bin/bambu"
         ln -sf "$(basename "${APPIMAGE_PATH}")" "${DIST_DIR}/bin/eucalyptus"
         ln -sf "$(basename "${APPIMAGE_PATH}")" "${DIST_DIR}/bin/tree-panda-cc"
      fi
   else
      echo "Warning: appimage_bundle target failed; continuing without AppImage"
   fi
fi
echo "::endgroup::"

# TODO: The installed distribution currently embeds build-tree plugin paths.
# Install the plugins into panda_dist and make plugin discovery relative to the
# installation prefix before treating the distribution as relocatable.
if test "${SYNTHESIS_SMOKE:-false}" = "true"; then
   SYNTHESIS_OUTPUT_DIR="${WORKSPACE_DIR}/synthesis-smoke"
   mkdir -p "${SYNTHESIS_OUTPUT_DIR}"
   echo "::group::Bambu function co-simulation — existing XML vectors"
   smoke_start_seconds=${SECONDS}
   set +e
   (
      cd "${SYNTHESIS_OUTPUT_DIR}"
      # shellcheck disable=SC1091 -- generated by the PandA installation
      source "${DIST_DIR}/settings.sh"
      "${DIST_DIR}/bin/bambu" \
         -O3 \
         "${WORKSPACE_DIR}/examples/mm/module.c" \
         --simulate \
         --generate-tb="${WORKSPACE_DIR}/examples/mm/test.xml" \
         --simulator=VERILATOR \
         --pretty-print=a.c \
         --channels-type=MEM_ACC_NN \
         --device-name=EP2C70F896C6 \
         --memory-allocation-policy=EXT_PIPELINED_BRAM \
         --experimental-setup=BAMBU \
         --top-fname=mm \
         --compiler="${FRONTEND_COMPILER}" \
         --output-directory="${SYNTHESIS_OUTPUT_DIR}" \
         --no-clean
   )
   smoke_status=$?
   set -e
   smoke_elapsed_seconds=$((SECONDS - smoke_start_seconds))
   printf 'Bambu XML Verilator co-simulation runtime: %s seconds\n' "${smoke_elapsed_seconds}" |
      tee "${SYNTHESIS_OUTPUT_DIR}/runtime.txt"
   echo "::endgroup::"
   if test "${smoke_status}" -ne 0; then
      exit "${smoke_status}"
   fi
fi

echo "dist-dir=${DIST_DIR#${WORKSPACE_DIR}/}" >> ${GITHUB_OUTPUT}
