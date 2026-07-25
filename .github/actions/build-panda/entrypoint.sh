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
ACTION_MODE="${ACTION_MODE:-build}"
REGRESSION_RESULTS_DIR="${WORKSPACE_DIR}/.ci-regression-results"
REGRESSION_EVIDENCE_DIR="${WORKSPACE_DIR}/.ci-regression-evidence"
CCACHE_RELATIVE_DIR="${CCACHE_DIR_INPUT:-.ccache}"
if [[ "${CCACHE_RELATIVE_DIR}" = /* || "${CCACHE_RELATIVE_DIR}" =~ (^|/)\.\.(/|$) ]]; then
   echo "::error::ccache-dir must be a workspace-relative path without parent traversal."
   exit 2
fi
CCACHE_DIR="${WORKSPACE_DIR}/${CCACHE_RELATIVE_DIR}"
APPIMAGE_NAME="bambu"
APPIMAGE_ENABLED=false
APPIMAGE_RUNTIME_FILE=""
CONTAINER_START_EPOCH="$(date +%s)"
CCACHE_REPORTED=false
CURRENT_STAGE="initialization"
BUILD_EXIT_STATUS="not-run"
KILL_DETECTED="no"
MEMORY_AVAILABLE_BEFORE_KIB=""
MEMORY_AVAILABLE_AFTER_KIB=""
OOM_COUNT_BEFORE=""
OOM_KILL_COUNT_BEFORE=""
MEMORY_MONITOR_PID=""
MEMORY_SAMPLES_FILE="${WORKSPACE_DIR}/.ci-telemetry/jobs-${J:-unknown}/memory-samples.tsv"
BUILD_ERROR_LOG="${WORKSPACE_DIR}/.ci-telemetry/jobs-${J:-unknown}/build-stderr.log"
PEAK_BUILD_RSS_KIB=""
PEAK_BUILD_CGROUP_KIB=""
BUILD_TELEMETRY_REPORTED=false
BUILD_START_EPOCH=""
BUILD_SECONDS_REPORTED=false

function set_output {
   printf '%s=%s\n' "$1" "$2" >> "${GITHUB_OUTPUT}"
}


function memory_available_kib {
   awk "/^MemAvailable:/ { print \$2; exit }" /proc/meminfo
}

function cgroup_event {
   local key="$1"
   if test -r /sys/fs/cgroup/memory.events; then
      awk -v key="${key}" "\$1 == key { print \$2; found=1 } END { if (!found) print 0 }" /sys/fs/cgroup/memory.events
   else
      printf "\n"
   fi
}

function cgroup_memory_current_bytes {
   if test -r /sys/fs/cgroup/memory.current; then
      cat /sys/fs/cgroup/memory.current
   elif test -r /sys/fs/cgroup/memory/memory.usage_in_bytes; then
      cat /sys/fs/cgroup/memory/memory.usage_in_bytes
   else
      printf "0\n"
   fi
}

function monitor_build_memory {
   set +e
   local rss_kib cgroup_bytes
   while :; do
      rss_kib="$(awk "\$1 == \"VmRSS:\" { sum += \$2 } END { printf \"%d\\n\", sum + 0 }" /proc/[0-9]*/status 2>/dev/null)"
      [[ "${rss_kib}" =~ ^[0-9]+$ ]] || rss_kib=0
      cgroup_bytes="$(cgroup_memory_current_bytes)"
      [[ "${cgroup_bytes}" =~ ^[0-9]+$ ]] || cgroup_bytes=0
      printf "%s\t%s\n" "${rss_kib}" "${cgroup_bytes}" >> "${MEMORY_SAMPLES_FILE}"
      sleep 0.25
   done
}

function start_build_telemetry {
   mkdir -p "$(dirname "${MEMORY_SAMPLES_FILE}")"
   : > "${MEMORY_SAMPLES_FILE}"
   : > "${BUILD_ERROR_LOG}"
   MEMORY_AVAILABLE_BEFORE_KIB="$(memory_available_kib)"
   OOM_COUNT_BEFORE="$(cgroup_event oom)"
   OOM_KILL_COUNT_BEFORE="$(cgroup_event oom_kill)"
   BUILD_START_EPOCH="$(date +%s)"
   set_output memory-available-before-kib "${MEMORY_AVAILABLE_BEFORE_KIB}"
   set_output build-exit-status running
   set_output failure-stage "${CURRENT_STAGE}"
   monitor_build_memory &
   MEMORY_MONITOR_PID=$!
}

function stop_memory_monitor {
   if [[ "${MEMORY_MONITOR_PID}" =~ ^[0-9]+$ ]]; then
      kill "${MEMORY_MONITOR_PID}" 2>/dev/null || true
      wait "${MEMORY_MONITOR_PID}" 2>/dev/null || true
      MEMORY_MONITOR_PID=""
   fi
}

function finish_build_telemetry {
   if ${BUILD_TELEMETRY_REPORTED}; then
      return
   fi
   stop_memory_monitor
   BUILD_TELEMETRY_REPORTED=true
   MEMORY_AVAILABLE_AFTER_KIB="$(memory_available_kib)"
   if test -s "${MEMORY_SAMPLES_FILE}"; then
      PEAK_BUILD_RSS_KIB="$(awk "\$1 > max { max=\$1 } END { print max + 0 }" "${MEMORY_SAMPLES_FILE}")"
      PEAK_BUILD_CGROUP_KIB="$(awk "\$2 > max { max=\$2 } END { printf \"%.0f\\n\", max / 1024 }" "${MEMORY_SAMPLES_FILE}")"
   fi
   local oom_after oom_kill_after
   oom_after="$(cgroup_event oom)"
   oom_kill_after="$(cgroup_event oom_kill)"
   OOM_DETECTED=unknown
   if [[ "${OOM_COUNT_BEFORE}" =~ ^[0-9]+$ && "${OOM_KILL_COUNT_BEFORE}" =~ ^[0-9]+$ &&
         "${oom_after}" =~ ^[0-9]+$ && "${oom_kill_after}" =~ ^[0-9]+$ ]]; then
      OOM_DETECTED=no
      if test "${oom_after}" -gt "${OOM_COUNT_BEFORE}" || test "${oom_kill_after}" -gt "${OOM_KILL_COUNT_BEFORE}"; then
         OOM_DETECTED=yes
      fi
   fi
   if grep -Eiq "out of memory|oom-kill|cannot allocate memory|killed signal terminated program" "${BUILD_ERROR_LOG}"; then
      OOM_DETECTED=yes
   fi
   if [[ "${BUILD_EXIT_STATUS}" =~ ^[0-9]+$ ]] && test "${BUILD_EXIT_STATUS}" -gt 128; then
      KILL_DETECTED=yes
   elif grep -Eiq "fatal error: killed|killed signal terminated program|(^|[^[:alpha:]])killed([^[:alpha:]]|$)" "${BUILD_ERROR_LOG}"; then
      KILL_DETECTED=yes
   fi
   set_output peak-build-rss-kib "${PEAK_BUILD_RSS_KIB}"
   set_output peak-build-cgroup-kib "${PEAK_BUILD_CGROUP_KIB}"
   set_output memory-available-after-kib "${MEMORY_AVAILABLE_AFTER_KIB}"
   set_output oom-detected "${OOM_DETECTED}"
   set_output kill-detected "${KILL_DETECTED}"
}

function report_runtime_telemetry {
   local status="$1" failure_stage
   if [[ "${BUILD_START_EPOCH}" =~ ^[0-9]+$ ]] && ! ${BUILD_SECONDS_REPORTED}; then
      set_output build-seconds "$(( $(date +%s) - BUILD_START_EPOCH ))"
   fi
   if [[ "${BUILD_START_EPOCH}" =~ ^[0-9]+$ ]]; then
      finish_build_telemetry
   fi
   if test "${status}" -eq 0; then
      failure_stage=none
   else
      failure_stage="${CURRENT_STAGE}"
   fi
   set_output action-exit-status "${status}"
   set_output failure-stage "${failure_stage}"
   echo "peak_build_rss_kib=${PEAK_BUILD_RSS_KIB}"
   echo "peak_build_cgroup_kib=${PEAK_BUILD_CGROUP_KIB}"
   echo "memory_available_before_kib=${MEMORY_AVAILABLE_BEFORE_KIB:-0}"
   echo "memory_available_after_kib=${MEMORY_AVAILABLE_AFTER_KIB:-0}"
   echo "build_exit_status=${BUILD_EXIT_STATUS}"
   echo "action_exit_status=${status}"
   echo "failure_stage=${failure_stage}"
   echo "oom_detected=${OOM_DETECTED:-unknown}"
   echo "kill_detected=${KILL_DETECTED}"
}

function ccache_stat {
   ccache --print-stats | awk -v key="$1" '$1 == key { print $2 }'
}

function report_ccache {
   if ${CCACHE_REPORTED} || ! command -v ccache >/dev/null 2>&1; then
      return
   fi
   CCACHE_REPORTED=true
   echo "::group::Final ccache statistics"
   ccache --show-stats
   local direct_hits preprocessed_hits hits misses cacheable size_kib hit_rate
   direct_hits="$(ccache_stat direct_cache_hit)"
   preprocessed_hits="$(ccache_stat preprocessed_cache_hit)"
   misses="$(ccache_stat cache_miss)"
   size_kib="$(ccache_stat cache_size_kibibyte)"
   hits=$((direct_hits + preprocessed_hits))
   cacheable=$((hits + misses))
   hit_rate="$(awk -v hits="${hits}" -v total="${cacheable}" 'BEGIN { if (total == 0) print "0.00"; else printf "%.2f", 100 * hits / total }')"
   set_output ccache-cacheable-calls "${cacheable}"
   set_output ccache-hits "${hits}"
   set_output ccache-misses "${misses}"
   set_output ccache-hit-rate "${hit_rate}"
   set_output ccache-size-kibibyte "${size_kib}"
   echo "ccache_cacheable_calls=${cacheable}"
   echo "ccache_hits=${hits}"
   echo "ccache_misses=${misses}"
   echo "ccache_hit_rate=${hit_rate}"
   echo "ccache_size_kibibyte=${size_kib}"
   echo "::endgroup::"
}


function cleanup {
   local status=$?
   trap - EXIT
   stop_memory_monitor || true
   report_runtime_telemetry "${status}" || true
   report_ccache || true
   echo "::endgroup::"
   exit ${status}
}
trap cleanup EXIT

if [[ "${ACTION_START_EPOCH:-}" =~ ^[0-9]+$ ]] && test "${CONTAINER_START_EPOCH}" -ge "${ACTION_START_EPOCH}"; then
   set_output container-setup-seconds "$((CONTAINER_START_EPOCH - ACTION_START_EPOCH))"
else
   set_output container-setup-seconds 0
fi
set_output cosimulation-seconds 0
set_output configure-exit-status not-run
set_output frontend-resolution-seconds ""
set_output frontend-resolution-exit-status not-run
set_output plugin-build-seconds ""
set_output plugin-build-exit-status not-run
set_output project-build-seconds ""
set_output project-build-exit-status not-run
set_output installation-exit-status not-run
set_output cosimulation-exit-status not-run
set_output selected-frontend ""
set_output clang-version ""
set_output clangxx-version ""
set_output llvm-version ""
set_output cmake-version ""
set_output gcc-version ""
set_output gxx-version ""
set_output ccache-version ""
set_output verilator-version ""

set_output peak-build-rss-kib ""
set_output peak-build-cgroup-kib ""
set_output memory-available-before-kib ""
set_output memory-available-after-kib ""
set_output build-exit-status not-run
set_output action-exit-status running
set_output failure-stage initialization
set_output oom-detected unknown
set_output kill-detected unknown
set_output regression-results-dir "${REGRESSION_RESULTS_DIR#${WORKSPACE_DIR}/}"
set_output regression-evidence-dir "${REGRESSION_EVIDENCE_DIR#${WORKSPACE_DIR}/}"
set_output regression-suite-seconds ""
set_output regression-suite-exit-status not-run
set_output regression-suite-outcome not-run
set_output regression-task-count 0
set_output regression-passed-count 0
set_output regression-failed-count 0

case "${ACTION_MODE}" in
   build|fast-regressions) ;;
   *)
      echo "::error::Unsupported build-panda action mode: ${ACTION_MODE}"
      exit 2
      ;;
esac

echo "::group::Initialize workspace"
if test -d "${COMPILERS_DIR}"; then
   echo "Pre-initialized AppImage dist directory found"
   if test -e "${COMPILERS_DIR}/settings.sh"; then source "${COMPILERS_DIR}/settings.sh"; fi
fi

export PATH
export CCACHE_DIR
export CCACHE_BASEDIR="${WORKSPACE_DIR}"
export CCACHE_NOHASHDIR=true
export CCACHE_CONFIGPATH="${CCACHE_DIR}/ccache.conf"
umask 0022
mkdir -p "${CCACHE_DIR}"
ccache --set-config="cache_dir=${CCACHE_DIR}"
ccache --set-config=max_size=5G
ccache --set-config=compression=true
ccache --set-config=compiler_check=content
ccache --set-config="base_dir=${WORKSPACE_DIR}"
ccache --set-config=hash_dir=false
ccache --set-config=absolute_paths_in_stderr=false
ccache --set-config=umask=0022
ccache --show-config
echo "Initial persisted ccache statistics:"
ccache --show-stats
ccache --zero-stats
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
set_output clang-version "$(clang-16 --version 2>&1 | sed -n '1p')"
set_output clangxx-version "$(clang++-16 --version 2>&1 | sed -n '1p')"
set_output llvm-version "$(llvm-config-16 --version 2>&1 | sed -n '1p')"
set_output cmake-version "$(cmake --version 2>&1 | sed -n '1p')"
set_output gcc-version "$(gcc --version 2>&1 | sed -n '1p')"
set_output gxx-version "$(g++ --version 2>&1 | sed -n '1p')"
set_output ccache-version "$(ccache --version 2>&1 | sed -n '1p')"
set_output verilator-version "$(verilator --version 2>&1 | sed -n '1p')"
clang-16 --version
clang++-16 --version
clang-cpp-16 --version
llvm-config-16 --version
llvm-link-16 --version
llvm-ar-16 --version
opt-16 --version
verilator --version
echo "::endgroup::"

if test "${ACTION_MODE}" = "fast-regressions"; then
   CURRENT_STAGE="fast-regressions"
   set_output failure-stage "${CURRENT_STAGE}"
   set_output dist-dir "${DIST_DIR#${WORKSPACE_DIR}/}"
   set_output regression-suite-outcome unknown

   echo "::group::Verify retained PandA build for fast regressions"
   CONFIG_HEADERS_DIR="${BUILD_DIR}/config_headers"
   FRONTEND_COMPILER="$(sed -n 's/^#define LIBBAMBU_COMPILER "\(.*\)"/\1/p' "${CONFIG_HEADERS_DIR}/config_LIBBAMBU_COMPILER.hpp")"
   CLANG_PLUGIN_SUBDIR="$(sed -n 's/^#define LIBBAMBU_COMPILER_DIR "\(.*\)"/\1/p' "${CONFIG_HEADERS_DIR}/config_LIBBAMBU_COMPILER_DIR.hpp")"
   if test "${FRONTEND_COMPILER}" != "I386_CLANG16" || test -z "${CLANG_PLUGIN_SUBDIR}"; then
      echo "::error::The retained build does not select the required I386_CLANG16 frontend."
      exit 1
   fi
   CLANG_PLUGIN_DIR="${BUILD_DIR}/${CLANG_PLUGIN_SUBDIR}"
   # The installed compiler configuration still embeds these build-tree paths;
   # this mode deliberately reuses the same workspace as the build invocation.
   for required_path in \
      "${DIST_DIR}/bin/bambu" \
      "${DIST_DIR}/settings.sh" \
      "${CLANG_PLUGIN_DIR}/ASTAnalyzer.so" \
      "${CLANG_PLUGIN_DIR}/customSROA.so" \
      "${CLANG_PLUGIN_DIR}/expandMemOps.so" \
      "${CLANG_PLUGIN_DIR}/dumpBambuIrSSA.so"; do
      if test ! -e "${required_path}"; then
         echo "::error file=${required_path}::Required retained build input is missing."
         exit 1
      fi
      ls -l "${required_path}"
   done
   set_output selected-frontend "${FRONTEND_COMPILER}"
   echo "::endgroup::"

   mkdir -p "${REGRESSION_RESULTS_DIR}" "${REGRESSION_EVIDENCE_DIR}"
   echo "::group::Run GitHub-hosted fast regressions"
   set_output regression-suite-exit-status running
   set_output regression-suite-outcome running
   regression_start_epoch="$(date +%s)"
   set +e
   (
      set -e
      # shellcheck disable=SC1091 -- generated by the PandA installation
      source "${DIST_DIR}/settings.sh"
      export PYTHONPATH="${WORKSPACE_DIR}/.github/scripts${PYTHONPATH:+:${PYTHONPATH}}"
      python3 -m ci_results run-regressions \
         --repository "${WORKSPACE_DIR}" \
         --bambu "${DIST_DIR}/bin/bambu" \
         --results-directory "${REGRESSION_RESULTS_DIR}" \
         --evidence-directory "${REGRESSION_EVIDENCE_DIR}" \
         --compiler I386_CLANG16 \
         --parallel-backend "${J:-2}" \
         --timeout-seconds 300
   )
   regression_status=$?
   set -e
   regression_seconds="$(( $(date +%s) - regression_start_epoch ))"
   suite_outcome=unknown
   suite_exit_status="${regression_status}"
   suite_duration_seconds="${regression_seconds}"
   suite_task_count=0
   suite_passed_count=0
   suite_failed_count=0
   suite_json="${REGRESSION_RESULTS_DIR}/suite.json"
   suite_values_file="$(mktemp)"

   if test -f "${suite_json}"; then
      set +e
      python3 - "${suite_json}" > "${suite_values_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as stream:
    suite = json.load(stream)
for key in (
    "outcome",
    "exit_status",
    "duration_seconds",
    "task_count",
    "passed_count",
    "failed_count",
):
    print(suite[key])
PY
      suite_parse_status=$?
      set -e
      if test "${suite_parse_status}" -eq 0; then
         mapfile -t suite_values < "${suite_values_file}"
         suite_outcome="${suite_values[0]}"
         suite_exit_status="${suite_values[1]}"
         suite_duration_seconds="${suite_values[2]}"
         suite_task_count="${suite_values[3]}"
         suite_passed_count="${suite_values[4]}"
         suite_failed_count="${suite_values[5]}"
      else
         echo "::error file=${suite_json}::Unable to parse the fast-regression suite result."
         if test "${regression_status}" -eq 0; then
            regression_status=1
            suite_exit_status=1
         fi
      fi
   else
      echo "::error file=${suite_json}::The fast-regression runner did not produce suite.json."
      if test "${regression_status}" -eq 0; then
         regression_status=1
         suite_exit_status=1
      fi
   fi
   rm -f "${suite_values_file}"
   set_output regression-suite-seconds "${suite_duration_seconds}"
   set_output regression-suite-exit-status "${suite_exit_status}"
   set_output regression-suite-outcome "${suite_outcome}"
   set_output regression-task-count "${suite_task_count}"
   set_output regression-passed-count "${suite_passed_count}"
   set_output regression-failed-count "${suite_failed_count}"
   printf 'Fast regressions: outcome=%s exit_status=%s duration_seconds=%s tasks=%s passed=%s failed=%s\n' \
      "${suite_outcome}" "${suite_exit_status}" "${suite_duration_seconds}" \
      "${suite_task_count}" "${suite_passed_count}" "${suite_failed_count}"
   echo "::endgroup::"

   if test "${regression_status}" -eq 0; then
      CURRENT_STAGE="complete"
      set_output failure-stage none
   fi
   exit "${regression_status}"
fi

CURRENT_STAGE="configure"
set_output failure-stage "${CURRENT_STAGE}"
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
   "-DCMAKE_C_COMPILER_LAUNCHER=ccache"
   "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
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

configure_start_epoch="$(date +%s)"
set +e
cmake -S "${WORKSPACE_DIR}" -B "${BUILD_DIR}" "${CMAKE_ARGS[@]}"
configure_status=$?
set -e
configure_seconds="$(( $(date +%s) - configure_start_epoch ))"
set_output configure-seconds "${configure_seconds}"
set_output configure-exit-status "${configure_status}"
echo "configure_seconds=${configure_seconds}"
echo "::endgroup::"
if test "${configure_status}" -ne 0; then
   exit "${configure_status}"
fi

CURRENT_STAGE="frontend-resolution"
set_output failure-stage "${CURRENT_STAGE}"
echo "::group::Resolve selected Clang frontend"
frontend_resolution_start_epoch="$(date +%s)"
CONFIG_HEADERS_DIR="${BUILD_DIR}/config_headers"
frontend_resolution_status=0
set +e
FRONTEND_COMPILER="$(sed -n 's/^#define LIBBAMBU_COMPILER "\(.*\)"/\1/p' "${CONFIG_HEADERS_DIR}/config_LIBBAMBU_COMPILER.hpp")"
frontend_compiler_status=$?
CLANG_PLUGIN_SUBDIR="$(sed -n 's/^#define LIBBAMBU_COMPILER_DIR "\(.*\)"/\1/p' "${CONFIG_HEADERS_DIR}/config_LIBBAMBU_COMPILER_DIR.hpp")"
clang_plugin_subdir_status=$?
set -e
if test "${frontend_compiler_status}" -ne 0; then
   frontend_resolution_status="${frontend_compiler_status}"
elif test "${clang_plugin_subdir_status}" -ne 0; then
   frontend_resolution_status="${clang_plugin_subdir_status}"
elif test -z "${FRONTEND_COMPILER}" || test -z "${CLANG_PLUGIN_SUBDIR}"; then
   frontend_resolution_status=1
fi
frontend_resolution_seconds="$(( $(date +%s) - frontend_resolution_start_epoch ))"
set_output frontend-resolution-seconds "${frontend_resolution_seconds}"
set_output frontend-resolution-exit-status "${frontend_resolution_status}"
if test "${frontend_resolution_status}" -ne 0; then
   echo "::error::Unable to resolve the selected frontend compiler and plugin directory."
   exit "${frontend_resolution_status}"
fi
FRONTEND_VERSION="${FRONTEND_COMPILER#I386_CLANG}"
CLANG_PLUGIN_DIR="${BUILD_DIR}/${CLANG_PLUGIN_SUBDIR}"
set_output selected-frontend "${FRONTEND_COMPILER}"
echo "Selected frontend: ${FRONTEND_COMPILER}"
echo "Expected plugin directory: ${CLANG_PLUGIN_DIR}"
echo "::endgroup::"

CURRENT_STAGE="plugin-build"
set_output failure-stage "${CURRENT_STAGE}"
start_build_telemetry
plugin_build_start_epoch="$(date +%s)"

PLUGIN_TARGETS=(
   "clang_plugin_${FRONTEND_VERSION}_ast"
   "clang_plugin_${FRONTEND_VERSION}_customsroa"
   "clang_plugin_${FRONTEND_VERSION}_expandmemops"
   "clang_plugin_${FRONTEND_VERSION}_ssa"
)
echo "::group::Build selected PandA Clang plugins"
set +e
cmake --build "${BUILD_DIR}" --target "${PLUGIN_TARGETS[@]}" --parallel "${J:-1}" 2> >(tee -a "${BUILD_ERROR_LOG}" >&2)
BUILD_EXIT_STATUS=$?
set -e
set_output build-exit-status "${BUILD_EXIT_STATUS}"
if test "${BUILD_EXIT_STATUS}" -ne 0; then
   plugin_build_seconds="$(( $(date +%s) - plugin_build_start_epoch ))"
   set_output plugin-build-seconds "${plugin_build_seconds}"
   set_output plugin-build-exit-status "${BUILD_EXIT_STATUS}"
   finish_build_telemetry
   exit "${BUILD_EXIT_STATUS}"
fi
echo "::endgroup::"

echo "::group::Verify PandA Clang plugins in ${CLANG_PLUGIN_DIR}"
for plugin in ASTAnalyzer.so customSROA.so expandMemOps.so dumpBambuIrSSA.so; do
   plugin_path="${CLANG_PLUGIN_DIR}/${plugin}"
   if test ! -f "${plugin_path}"; then
      echo "::error file=${plugin_path}::Required PandA Clang plugin is missing."
      plugin_build_seconds="$(( $(date +%s) - plugin_build_start_epoch ))"
      set_output plugin-build-seconds "${plugin_build_seconds}"
      set_output plugin-build-exit-status 1
      exit 1
   fi
   set +e
   file "${plugin_path}"
   plugin_file_status=$?
   set -e
   if test "${plugin_file_status}" -ne 0; then
      plugin_build_seconds="$(( $(date +%s) - plugin_build_start_epoch ))"
      set_output plugin-build-seconds "${plugin_build_seconds}"
      set_output plugin-build-exit-status "${plugin_file_status}"
      exit "${plugin_file_status}"
   fi
   set +e
   ldd "${plugin_path}" | tee "${BUILD_DIR}/${plugin}.ldd.txt"
   plugin_ldd_status=$?
   set -e
   if test "${plugin_ldd_status}" -ne 0; then
      plugin_build_seconds="$(( $(date +%s) - plugin_build_start_epoch ))"
      set_output plugin-build-seconds "${plugin_build_seconds}"
      set_output plugin-build-exit-status "${plugin_ldd_status}"
      exit "${plugin_ldd_status}"
   fi
   if grep -q "not found" "${BUILD_DIR}/${plugin}.ldd.txt"; then
      echo "::error file=${plugin_path}::PandA Clang plugin has unresolved shared-library dependencies."
      plugin_build_seconds="$(( $(date +%s) - plugin_build_start_epoch ))"
      set_output plugin-build-seconds "${plugin_build_seconds}"
      set_output plugin-build-exit-status 1
      exit 1
   fi
done
echo "::endgroup::"
plugin_build_seconds="$(( $(date +%s) - plugin_build_start_epoch ))"
set_output plugin-build-seconds "${plugin_build_seconds}"
set_output plugin-build-exit-status 0

CURRENT_STAGE="project-build"
set_output failure-stage "${CURRENT_STAGE}"
project_build_start_epoch="$(date +%s)"
echo "::group::Build PandA project"
set +e
cmake --build "${BUILD_DIR}" --parallel "${J:-1}" 2> >(tee -a "${BUILD_ERROR_LOG}" >&2)
BUILD_EXIT_STATUS=$?
set -e
set_output build-exit-status "${BUILD_EXIT_STATUS}"
project_build_seconds="$(( $(date +%s) - project_build_start_epoch ))"
set_output project-build-seconds "${project_build_seconds}"
set_output project-build-exit-status "${BUILD_EXIT_STATUS}"
finish_build_telemetry
if test "${BUILD_EXIT_STATUS}" -ne 0; then
   exit "${BUILD_EXIT_STATUS}"
fi
echo "::endgroup::"
build_seconds="$(( $(date +%s) - BUILD_START_EPOCH ))"
set_output build-seconds "${build_seconds}"
BUILD_SECONDS_REPORTED=true
echo "build_seconds=${build_seconds}"

if test -e "${BUILD_DIR}/compile_commands.json"; then
   echo "::group::Export Compilation Database"
   COMPILATION_DB="${WORKSPACE_DIR}/compilation_db"
   mkdir -p "${COMPILATION_DB}/build"
   cp -r "${BUILD_DIR}/config_headers" "${COMPILATION_DB}/build/"
   mv "${BUILD_DIR}/compile_commands.json" "${COMPILATION_DB}/build/"
   echo "compilation-db=${COMPILATION_DB#${WORKSPACE_DIR}/}" >> ${GITHUB_OUTPUT}
   echo "::endgroup::"
fi

CURRENT_STAGE="installation"
set_output failure-stage "${CURRENT_STAGE}"
echo "::group::Package PandA distribution"
install_start_epoch="$(date +%s)"
set +e
cmake --install "${BUILD_DIR}" --strip
installation_status=$?
set -e
install_seconds="$(( $(date +%s) - install_start_epoch ))"
set_output install-seconds "${install_seconds}"
set_output installation-exit-status "${installation_status}"
echo "install_seconds=${install_seconds}"
if test "${installation_status}" -ne 0; then
   exit "${installation_status}"
fi

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
set_output dist-dir "${DIST_DIR#${WORKSPACE_DIR}/}"

# TODO: The installed distribution currently embeds build-tree plugin paths.
# Install the plugins into panda_dist and make plugin discovery relative to the
# installation prefix before treating the distribution as relocatable.
CURRENT_STAGE="cosimulation"
set_output failure-stage "${CURRENT_STAGE}"
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
   set_output cosimulation-seconds "${smoke_elapsed_seconds}"
   set_output cosimulation-exit-status "${smoke_status}"
   printf 'Bambu XML Verilator co-simulation runtime: %s seconds\n' "${smoke_elapsed_seconds}" |
      tee "${SYNTHESIS_OUTPUT_DIR}/runtime.txt"
   echo "::endgroup::"
   if test "${smoke_status}" -ne 0; then
      exit "${smoke_status}"
   fi
fi

CURRENT_STAGE="complete"
set_output failure-stage none
set_output dist-dir "${DIST_DIR#${WORKSPACE_DIR}/}"
