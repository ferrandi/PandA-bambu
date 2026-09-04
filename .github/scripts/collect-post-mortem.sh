#!/usr/bin/env bash
set -u

report_dir="${1:?report directory is required}"
job_name="${2:-unknown-job}"
command_line="${3:-}"
dist_dir="${4:-panda_dist}"

post_mortem_dir="${report_dir}/post-mortem"
mkdir -p "${post_mortem_dir}"

manifest="${post_mortem_dir}/manifest.txt"
{
   echo "job=${job_name}"
   echo "command=${command_line}"
   echo "github-sha=${GITHUB_SHA:-}"
   echo "github-ref=${GITHUB_REF:-}"
   echo "github-run-id=${GITHUB_RUN_ID:-}"
   echo "runner-name=${RUNNER_NAME:-}"
   echo "runner-os=${RUNNER_OS:-}"
   echo "timestamp=$(date --iso-8601=seconds)"
   echo
   echo "bambu:"
   if [[ -x "${dist_dir}/bin/bambu" ]]; then
      "${dist_dir}/bin/bambu" --version 2>&1 | head -40 || true
   else
      echo "missing ${dist_dir}/bin/bambu"
   fi
   echo
   echo "failed-cases:"
} > "${manifest}"

found=0

copy_external_log_refs() {
   local case_copy="$1"
   local external_dir="${case_copy}/post_mortem_external_logs"
   local copied=0
   local log_file ref target
   local -A copied_refs=()

   while IFS= read -r -d '' log_file
   do
      while IFS= read -r ref
      do
         [[ -n "${ref}" ]] || continue
         [[ -f "${ref}" ]] || continue
         [[ -z "${copied_refs[${ref}]+x}" ]] || continue
         copied_refs["${ref}"]=1
         mkdir -p "${external_dir}"
         target="${external_dir}/$(basename "${ref}")"
         cp -a "${ref}" "${target}"
         if [[ "${copied}" -eq 0 ]]; then
            {
               echo
               echo "external-log-files:"
            } >> "${manifest}"
         fi
         printf '%s -> %s\n' "${ref}" "${target#${post_mortem_dir}/}" >> "${manifest}"
         copied=1
      done < <(sed -nE 's@.*(/tmp/[[:alnum:]_.+/@-]+).*@\1@p' "${log_file}" | sort -u)
   done < <(
      find "${case_copy}" -type f \( \
         -name '*.log' -o \
         -name execution.log -o \
         -name failure.log -o \
         -name timeout.log -o \
         -name __stdouterr -o \
         -name transcript -o \
         -name backend_output \
      \) -print0
   )
}

while IFS= read -r -d '' marker
do
   case_dir="$(dirname "${marker}")"
   rel_case="${case_dir#./}"
   case_copy="${post_mortem_dir}/${rel_case}"
   mkdir -p "$(dirname "${case_copy}")"
   # Keep the full testcase workdir: logs alone are not enough to replay failed simulations.
   cp -a "${case_dir}" "${case_copy}"
   printf '%s\n' "${rel_case}" >> "${manifest}"
   copy_external_log_refs "${case_copy}"
   found=1
done < <(
   find . \
      -path "./${report_dir}" -prune -o \
      -path "./${dist_dir}" -prune -o \
      -path ./.git -prune -o \
      \( -name failure.log -o -name timeout.log \) -print0
)

if [[ "${found}" -eq 0 ]]; then
   echo "No failure.log or timeout.log marker found." >> "${manifest}"
   echo "Known execution logs and return values:" >> "${manifest}"
   find . \
      -path "./${report_dir}" -prune -o \
      -path "./${dist_dir}" -prune -o \
      -path ./.git -prune -o \
      -type f \( -name execution.log -o -name return_value \) -print \
      >> "${manifest}" 2>/dev/null || true
fi
