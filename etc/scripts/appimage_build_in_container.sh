#!/usr/bin/env bash
# Builds and packages a portable AppImage inside a legacy-compatible container.
# Called by the CMake appimage_bundle target.
set -euo pipefail

source_dir=""
build_dir=""
appdir=""
cache_init=""
cache_init_in_container=""
build_dir_in_container=""
appdir_in_container=""
generator=""
parallel=""
compilers=""
compilers_backup=""
appimage_name=""
appimage_arch="x86_64"
max_glibc=""
runtime_file=""
runtime="auto"
image=""
binds=()

while test $# -gt 0; do
   case "$1" in
      --source-dir)          source_dir="$2";          shift 2 ;;
      --build-dir)           build_dir="$2";           shift 2 ;;
      --appdir)              appdir="$2";              shift 2 ;;
      --cache-init)          cache_init="$2";          shift 2 ;;
      --cache-init-in-container) cache_init_in_container="$2"; shift 2 ;;
      --build-dir-in-container)  build_dir_in_container="$2";  shift 2 ;;
      --appdir-in-container)     appdir_in_container="$2";     shift 2 ;;
      --generator)           generator="$2";           shift 2 ;;
      --parallel)            parallel="$2";            shift 2 ;;
      --compilers)           compilers="$2";           shift 2 ;;
      --compilers-backup)    compilers_backup="$2";    shift 2 ;;
      --appimage-name)       appimage_name="$2";       shift 2 ;;
      --appimage-arch)       appimage_arch="$2";       shift 2 ;;
      --max-glibc)           max_glibc="$2";           shift 2 ;;
      --runtime-file)        runtime_file="$2";        shift 2 ;;
      --runtime)             runtime="$2";             shift 2 ;;
      --image)               image="$2";               shift 2 ;;
      --bind)                binds+=("$2");            shift 2 ;;
      *)
         echo "ERROR: Unknown argument: $1" >&2
         exit 1
         ;;
   esac
done

if test -z "${source_dir}" || test -z "${build_dir}" || test -z "${appdir}" || test -z "${cache_init}" \
   || test -z "${cache_init_in_container}" || test -z "${build_dir_in_container}" \
   || test -z "${appdir_in_container}" || test -z "${appimage_name}" || test -z "${image}"; then
   echo "ERROR: Missing required arguments" >&2
   exit 1
fi

if test ! -d "${source_dir}"; then
   echo "ERROR: Source directory not found: ${source_dir}" >&2
   exit 1
fi

mkdir -p "${build_dir}" "${appdir}"

if test ! -f "${cache_init}"; then
   echo "ERROR: Cache init file not found: ${cache_init}" >&2
   exit 1
fi

if test -z "${runtime}" || test "${runtime}" = "auto"; then
   if command -v docker > /dev/null 2>&1; then
      runtime="docker"
   elif command -v podman > /dev/null 2>&1; then
      runtime="podman"
   else
      echo "ERROR: Neither docker nor podman is available" >&2
      exit 1
   fi
fi

if ! command -v "${runtime}" > /dev/null 2>&1; then
   echo "ERROR: Container runtime not available: ${runtime}" >&2
   exit 1
fi

container_args=(
   run
   --rm
   --user "$(id -u):$(id -g)"
   -e HOME=/build/.container-home
   -e XDG_CACHE_HOME=/build/.container-cache
   -v "${source_dir}:/src:ro"
   -v "${build_dir}:/build"
   -v "${appdir}:${appdir_in_container}"
   -w /src
)

for bind_path in "${binds[@]+"${binds[@]}"}"; do
   test -n "${bind_path}" || continue
   if test -e "${bind_path}"; then
      container_args+=(-v "${bind_path}:${bind_path}")
   fi
done

# Build the inner command as a single bash -lc script.
# Shell variables from the outer script (e.g. ${compilers}) are expanded
# when this array is constructed.
container_command=(
   bash
   -lc
   "set -euo pipefail

mkdir -p /build/.container-home /build/.container-cache
mkdir -p '${build_dir_in_container}'
mkdir -p '${appdir_in_container}'

# Clean previous AppDir contents
if ! find '${appdir_in_container}' -mindepth 1 -maxdepth 1 -exec rm -rf {} +; then
   echo 'ERROR: Failed to clean ${appdir_in_container}; remove or chown stale files in the AppDir on the host.' >&2
   exit 1
fi

# Download compilers if requested (must happen before ccache PATH setup)
if test -n '${compilers}'; then
   echo '--- Downloading compilers: ${compilers}'
   bash /src/.devcontainer/library-scripts/compiler-download.sh /build/compat-compilers '${compilers}' '${compilers_backup}'
   source /build/compat-compilers/settings.sh
fi

# Enable ccache (prepend AFTER compiler download so ccache wraps them)
if test -d /usr/lib/ccache; then
   export PATH=/usr/lib/ccache:\$PATH
fi
if command -v ccache > /dev/null 2>&1; then
   export CCACHE_DIR=/build/.container-ccache
   export CCACHE_BASEDIR=/src
   export CCACHE_NOHASHDIR=1
   mkdir -p \"\${CCACHE_DIR}\"
   echo '--- ccache enabled (CCACHE_DIR='\"\${CCACHE_DIR}\"')'
fi

# Bootstrap ninja if needed
if test '${generator}' = 'Ninja' && ! command -v ninja > /dev/null 2>&1; then
   mkdir -p /build/.container-tools
   if test ! -x /build/.container-tools/ninja; then
      echo '--- Bootstrapping ninja'
      wget -q -O /build/.container-tools/ninja.zip https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-linux.zip
      python3 -c \"import zipfile; zipfile.ZipFile('/build/.container-tools/ninja.zip').extractall('/build/.container-tools')\"
      chmod +x /build/.container-tools/ninja
   fi
   export PATH=/build/.container-tools:\$PATH
fi

# Bootstrap file/libmagic for appimagetool (non-root)
if ! command -v file > /dev/null 2>&1; then
   file_root=/build/.container-tools/file-root
   if test ! -x \"\${file_root}/usr/bin/file\"; then
      echo '--- Bootstrapping file/libmagic'
      apt_root=/build/.container-tools/file-apt
      mkdir -p \"\${apt_root}/state/lists/partial\" \"\${apt_root}/cache/archives/partial\" \"\${file_root}\"
      apt-get -o Dir::State=\"\${apt_root}/state\" -o Dir::Cache=\"\${apt_root}/cache\" update > /dev/null 2>&1 || true
      (
         cd \"\${apt_root}\"
         rm -f ./*.deb
         apt-get -o Dir::State=\"\${apt_root}/state\" -o Dir::Cache=\"\${apt_root}/cache\" download file libmagic1 > /dev/null 2>&1 || true
         for deb in ./*.deb; do
            test -f \"\${deb}\" && dpkg-deb -x \"\${deb}\" \"\${file_root}\"
         done
      )
   fi
   if test -x \"\${file_root}/usr/bin/file\"; then
      export PATH=\"\${file_root}/usr/bin:\$PATH\"
      if test -d \"\${file_root}/usr/lib/x86_64-linux-gnu\"; then
         export LD_LIBRARY_PATH=\"\${file_root}/usr/lib/x86_64-linux-gnu\${LD_LIBRARY_PATH:+:\$LD_LIBRARY_PATH}\"
      fi
   fi
fi

# Configure
echo '--- Configuring inside container'
cmake_args=( cmake -S /src -B '${build_dir_in_container}' -C '${cache_init_in_container}' )
if test -n '${generator}'; then
   cmake_args+=( -G '${generator}' )
fi
\"\${cmake_args[@]}\"

# Build
echo '--- Building inside container'
build_args=( cmake --build '${build_dir_in_container}' )
if test -n '${parallel}'; then
   build_args+=( --parallel '${parallel}' )
fi
\"\${build_args[@]}\"

# Install
echo '--- Installing to AppDir'
cmake --install '${build_dir_in_container}' --strip

# Stage GCC toolchain, 32-bit sysroot, and system headers for the bundled clang
# compilers.  The clang wrappers inject --gcc-toolchain so clang always resolves
# C++ headers from the AppImage, and --sysroot for -m32/-mx32 invocations.
# The compatibility container provides the crtbegin/libgcc toolchain pieces,
# the glibc multilib startup/runtime files, and the complete hosted header tree.
if test -d '${appdir_in_container}/usr/compilers' && test -d /usr/lib/gcc && test -d /usr/lib32 && test -d /lib32; then
   echo '--- Staging GCC toolchain and 32-bit sysroot'
   mkdir -p '${appdir_in_container}/usr/lib' '${appdir_in_container}/usr/lib32' '${appdir_in_container}/lib32' \
            '${appdir_in_container}/usr/include'
   cp -a /usr/lib/gcc '${appdir_in_container}/usr/lib/'
   cp -a /usr/lib32/. '${appdir_in_container}/usr/lib32/'
   cp -a /lib32/. '${appdir_in_container}/lib32/'
   while IFS= read -r -d '' compat_symlink; do
      compat_target=\$(readlink \$compat_symlink)
      case \$compat_target in
         /lib32/*)
            ln -snf ../../lib32/\$(basename \$compat_target) \$compat_symlink
            ;;
      esac
   done < <(find '${appdir_in_container}/usr/lib32' -maxdepth 1 -type l -print0)
   rm -f '${appdir_in_container}/usr/lib32/libm.so'
   # Remove libraries not needed for testbench compilation that violate the
   # GLIBC version cap (AddressSanitizer, quad-precision float).
   rm -f '${appdir_in_container}/usr/lib32'/libasan* \
         '${appdir_in_container}/usr/lib32'/libquadmath*
   if test -d /usr/include; then
      cp -a /usr/include/. '${appdir_in_container}/usr/include/'
      if test ! -e '${appdir_in_container}/usr/include/asm' && test -d '${appdir_in_container}/usr/include/x86_64-linux-gnu/asm'; then
         ln -s x86_64-linux-gnu/asm '${appdir_in_container}/usr/include/asm'
      fi
   fi
   # Keep only the newest GCC installation — clang selects the highest version
   # via --gcc-toolchain anyway.  Older installations from the compat container
   # carry broken symlinks and waste space.
   best_gcc_ver=
   best_gcc_path=
   while IFS= read -r -d '' d; do
      v=\$(basename \"\$d\")
      if test -z \"\$best_gcc_ver\" || \
         test \"\$(printf '%s\\n%s' \"\$best_gcc_ver\" \"\$v\" | sort -V | tail -n1)\" = \"\$v\"; then
         best_gcc_ver=\"\$v\"
         best_gcc_path=\"\$d\"
      fi
   done < <(find '${appdir_in_container}/usr/lib/gcc' -mindepth 2 -maxdepth 2 -type d -print0)
   if test -n \"\$best_gcc_ver\"; then
      while IFS= read -r -d '' d; do
         test \"\$d\" = \"\$best_gcc_path\" && continue
         rm -rf \"\$d\"
      done < <(find '${appdir_in_container}/usr/lib/gcc' -mindepth 2 -maxdepth 2 -type d -print0)
      find '${appdir_in_container}/usr/lib/gcc' -mindepth 1 -maxdepth 1 -type d -empty -delete
      for hdr_root in '${appdir_in_container}/usr/include/c++' \
                      '${appdir_in_container}/usr/include/x86_64-linux-gnu/c++'; do
         test -d \"\$hdr_root\" || continue
         while IFS= read -r -d '' d; do
            v=\$(basename \"\$d\")
            test \"\$v\" = \"\$best_gcc_ver\" && continue
            rm -rf \"\$d\"
         done < <(find \"\$hdr_root\" -mindepth 1 -maxdepth 1 -type d -print0)
      done
      echo \"--- Kept GCC \$best_gcc_ver; removed older installations\"
   fi
   # Remove any absolute symlinks under the staged GCC/lib32 trees — they
   # would escape the AppDir and resolve on the host (or break).
   while IFS= read -r -d '' lnk; do
      tgt=\$(readlink \"\$lnk\")
      case \"\$tgt\" in
         /*) rm -f \"\$lnk\" ;;
      esac
   done < <(find '${appdir_in_container}/usr/lib/gcc' '${appdir_in_container}/usr/lib32' -type l -print0 2>/dev/null)
fi

# GLIBC audit
if test -n '${max_glibc}'; then
   echo '--- Running GLIBC audit (max ${max_glibc})'
   bash /src/etc/scripts/appimage_check_glibc.sh '${appdir_in_container}/usr' '${max_glibc}'
   # Note: lib32/ is a compilation sysroot only; its glibc files are never loaded at runtime.
fi

# Package with appimagetool
echo '--- Creating AppImage'
appimage_args=( appimagetool -n )
if test -n '${runtime_file}'; then
   appimage_args+=( --runtime-file '${runtime_file}' )
fi
if test -f '${appdir_in_container}/.appimageignore'; then
   appimage_args+=( --exclude-file '${appdir_in_container}/.appimageignore' )
fi
appimage_path='${appdir_in_container}/${appimage_name}.AppImage'
rm -rf \"\${appimage_path}\"
env APPIMAGE_EXTRACT_AND_RUN=1 APPIMAGE_ALLOW_UNSUPPORTED_FILESYSTEMS=1 ARCH='${appimage_arch}' \"\${appimage_args[@]}\" '${appdir_in_container}' \"\${appimage_path}\"
echo '--- AppImage created: ${appdir_in_container}/${appimage_name}.AppImage'"
)

"${runtime}" "${container_args[@]}" "${image}" "${container_command[@]}"
