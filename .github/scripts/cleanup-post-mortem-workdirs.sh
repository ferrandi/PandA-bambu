#!/usr/bin/env bash
set -u

found=0

remove_dir() {
   local path="$1"
   local kind="$2"

   if [[ -z "${path}" || "${path}" == "." || "${path}" == "/" ]]; then
      printf 'Skipping unsafe cleanup path %s\n' "${path:-<empty>}"
      return
   fi

   if [[ -d "${path}" ]]; then
      printf 'Removing %s %s\n' "${kind}" "${path}"
      rm -rf -- "${path}"
      found=1
   fi
}

for path in out_*
do
   remove_dir "${path}" "post-mortem workdir"
done

if [[ "$#" -eq 0 ]]; then
   set -- panda_dist
fi

for path in "$@"
do
   remove_dir "${path}" "cleanup directory"
done

if [[ "${found}" -eq 0 ]]; then
   echo "No cleanup directories to remove."
fi
