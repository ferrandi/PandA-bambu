#!/usr/bin/env bash
set -euo pipefail

scan_root="${1:-}"
max_glibc="${2:-}"

if test -z "${scan_root}" || test ! -d "${scan_root}"; then
   echo "ERROR: Usage: $0 <appimage-usr-root> [max_glibc_version]" >&2
   exit 1
fi

extract_versions() {
   local elf_file="$1"
   { readelf --version-info "${elf_file}" 2> /dev/null || true; } \
      | sed -n 's/.*Name: \(GLIBC_[0-9.]*\).*/\1/p' \
      | sort -Vu
}

version_gt() {
   test "$(printf '%s\n%s\n' "$1" "$2" | sort -V | tail -1)" = "$1" && test "$1" != "$2"
}

worst_seen=""
violations=0

while IFS= read -r -d '' elf_file; do
   versions="$(extract_versions "${elf_file}")"
   if test -z "${versions}"; then
      continue
   fi
   highest="$(printf '%s\n' "${versions}" | tail -1)"
   highest="${highest#GLIBC_}"
   if test -z "${worst_seen}" || version_gt "${highest}" "${worst_seen}"; then
      worst_seen="${highest}"
   fi
   if test -n "${max_glibc}" && version_gt "${highest}" "${max_glibc}"; then
      echo "ERROR: ${elf_file} requires GLIBC_${highest} (max allowed: GLIBC_${max_glibc})" >&2
      violations=1
   fi
done < <(find "${scan_root}" -type f -print0)

if test -n "${worst_seen}"; then
   echo "INFO: Highest GLIBC requirement under ${scan_root}: GLIBC_${worst_seen}"
else
   echo "INFO: No ELF objects with GLIBC version requirements found under ${scan_root}"
fi

if test "${violations}" -ne 0; then
   exit 1
fi
