#!/usr/bin/env bash
# Fix RPATH of all ELF shared libraries and executables inside an AppImage
# staging root so they find their dependencies at runtime.
#
# Usage: appimage_fix_rpath.sh <appimage-usr-prefix>
#
# The script:
#   1. Scans <prefix>/lib/panda/ for shared objects and sets their RPATH
#   2. Scans <prefix>/bin/ for ELF executables and ensures their RPATH is set
#   3. Uses patchelf if available, falls back to chrpath
set -euo pipefail

install_prefix="$1"
panda_libdir="${install_prefix}/lib/panda"

# Locate RPATH tool
RPATH_TOOL=""
if command -v patchelf > /dev/null 2>&1; then
   RPATH_TOOL="patchelf"
elif command -v chrpath > /dev/null 2>&1; then
   RPATH_TOOL="chrpath"
else
   echo "WARNING: Neither patchelf nor chrpath found; skipping RPATH fixup"
   exit 0
fi

set_rpath() {
   local file="$1"
   local rpath="$2"
   if [ "${RPATH_TOOL}" = "patchelf" ]; then
      patchelf --set-rpath "${rpath}" "${file}" 2>/dev/null || true
   elif [ "${RPATH_TOOL}" = "chrpath" ]; then
      chrpath -r "${rpath}" "${file}" 2>/dev/null || true
   fi
}

is_elf() {
   file -b "$1" 2>/dev/null | grep -q 'ELF'
}

echo "INFO: Fixing RPATH in ${install_prefix} using ${RPATH_TOOL}"

# Fix shared libraries in lib/panda/
if test -d "${panda_libdir}"; then
   for lib in "${panda_libdir}"/*.so*; do
      test -f "${lib}" || continue
      test -L "${lib}" && continue
      is_elf "${lib}" || continue
      set_rpath "${lib}" '$ORIGIN'
   done
   echo "INFO: Fixed RPATH for libraries in ${panda_libdir}"
fi

# Fix executables in bin/
if test -d "${install_prefix}/bin"; then
   for exe in "${install_prefix}/bin"/*; do
      test -f "${exe}" || continue
      test -L "${exe}" && continue
      is_elf "${exe}" || continue
      set_rpath "${exe}" '$ORIGIN/../lib/panda'
   done
   echo "INFO: Fixed RPATH for executables in ${install_prefix}/bin"
fi

# Fix shared libraries in lib/ (non-panda, e.g. from flattened compilers)
if test -d "${install_prefix}/lib"; then
   for lib in "${install_prefix}/lib"/*.so*; do
      test -f "${lib}" || continue
      test -L "${lib}" && continue
      is_elf "${lib}" || continue
      set_rpath "${lib}" '$ORIGIN'
   done
fi

echo "INFO: RPATH fixup complete"
