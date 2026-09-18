#!/usr/bin/env bash
set -euo pipefail

install_prefix="${1:-}"

if test -z "${install_prefix}" || test ! -d "${install_prefix}"; then
   echo "ERROR: Usage: $0 <appimage-usr-prefix>" >&2
   exit 1
fi

if test ! -d "${install_prefix}/compilers"; then
   echo "INFO: No compiler bundles under ${install_prefix}; skipping clang AppImage wrapping"
   exit 0
fi

runtime_libdir="${install_prefix}/lib/panda"

if test -d "${runtime_libdir}"; then
   if test -e "${runtime_libdir}/libstdc++.so.6" && test ! -e "${runtime_libdir}/libstdc++.so"; then
      ln -sfn libstdc++.so.6 "${runtime_libdir}/libstdc++.so"
   fi
   if test -e "${runtime_libdir}/libgcc_s.so.1" && test ! -e "${runtime_libdir}/libgcc_s.so"; then
      ln -sfn libgcc_s.so.1 "${runtime_libdir}/libgcc_s.so"
   fi
fi

wrap_compiler() {
   local tool_path="$1"
   local real_path="${tool_path}.real"

   if test ! -e "${real_path}"; then
      mv "${tool_path}" "${real_path}"
   fi

   cat > "${tool_path}" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

script_path="$(readlink -f "${BASH_SOURCE[0]}")"
tool_real="${script_path}.real"
script_dir="$(dirname "${script_path}")"
compiler_root="$(cd "${script_dir}/.." && pwd -P)"
install_prefix="$(cd "${compiler_root}/../.." && pwd -P)"
app_root="$(cd "${install_prefix}/.." && pwd -P)"

extra_args=()
library_path_entries=()

# Always use the bundled GCC toolchain so C++ headers (<iostream> etc.)
# resolve from the AppImage rather than depending on a host GCC installation.
if test -d "${install_prefix}/lib/gcc"; then
   extra_args+=(--gcc-toolchain="${install_prefix}")
fi

append_library_path() {
   local candidate="$1"
   test -d "${candidate}" || return 0
   library_path_entries+=("${candidate}")
}

# Provide linker-visible C++ runtime directories from the AppImage so
# clang++ can resolve -lstdc++ even when the host lacks libstdc++-devel.
append_library_path "${install_prefix}/lib/panda"

# For -m32/-mx32 also set --sysroot to the bundled 32-bit sysroot so the
# linker finds crt*.o, libgcc, and libc startup files inside the AppImage.
for arg in "$@"; do
   case "${arg}" in
      -m32|-mx32)
         append_library_path "${install_prefix}/lib32"
         if test -d "${install_prefix}/lib32" && test -d "${app_root}/lib32"; then
            extra_args+=(--sysroot="${app_root}")
         fi
         break
         ;;
   esac
done

if test "${#library_path_entries[@]}" -gt 0; then
   appimage_library_path="$(IFS=:; printf '%s' "${library_path_entries[*]}")"
   if test -n "${LIBRARY_PATH:-}"; then
      export LIBRARY_PATH="${appimage_library_path}:${LIBRARY_PATH}"
   else
      export LIBRARY_PATH="${appimage_library_path}"
   fi
fi

exec "${tool_real}" "${extra_args[@]}" "$@"
EOF

   chmod +x "${tool_path}"
}

wrapped=0
for compiler_dir in "${install_prefix}"/compilers/clang-*; do
   test -d "${compiler_dir}/bin" || continue

   for tool_path in "${compiler_dir}"/bin/clang-[0-9]* \
                    "${compiler_dir}"/bin/clang++-[0-9]* \
                    "${compiler_dir}"/bin/clang-cpp-[0-9]*; do
      test -e "${tool_path}" || continue
      case "$(basename "${tool_path}")" in
         *.real*) continue ;;
      esac
      wrap_compiler "${tool_path}"
      wrapped=1
   done
done

if test "${wrapped}" -eq 1; then
   echo "INFO: Wrapped AppImage clang compiler entry points under ${install_prefix}/compilers"
else
   echo "INFO: No versioned clang compiler entry points found under ${install_prefix}/compilers"
fi
