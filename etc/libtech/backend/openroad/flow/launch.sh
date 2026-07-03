#!/usr/bin/env bash
SWD="$(dirname "$(readlink -e "$0")")"
OUT_LVL="$(bambu_results /application@verbosity)"
ORFS_DEFAULT_PINNED_DIGEST="a2bb042b6ea0811b4a63b6be41dcac5dc49e72f8"
ORFS_REQUIRED_DIGEST="${OPENROAD_ORFS_PINNED_DIGEST:-${ORFS_DEFAULT_PINNED_DIGEST}}"
ORFS_REPO_URL="https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts.git"
ORFS_DEFAULT_MODULES_TO_KEEP_AND_RETIME="mul_node_FU widen_mul_node_FU ui_widen_mul_node_FU ui_mul_node_FU"

detect_host_cores()
{
   nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1
}

resolve_num_cores()
{
   : "${ENABLE_PARALLEL:=$(bambu_results /application/backend@parallel)}"
   export ENABLE_PARALLEL

   if [[ -z "${NUM_CORES:-}" ]]; then
      if [[ "${ENABLE_PARALLEL}" =~ ^[0-9]+$ ]]; then
         if [[ "${ENABLE_PARALLEL}" -gt 1 ]]; then
            NUM_CORES="${ENABLE_PARALLEL}"
         else
            NUM_CORES=1
         fi
      elif [[ "${ENABLE_PARALLEL}" == "true" || "${ENABLE_PARALLEL}" == "TRUE" ]]; then
         NUM_CORES="$(detect_host_cores)"
      else
         NUM_CORES=1
      fi
   fi
   export NUM_CORES
}

append_asap7_fakeram_assets_config()
{
   local config_mk="$1"

   case "${PLATFORM}" in
      asap7|asap7-*)
         cat >> "${config_mk}" <<'EOF'
export ADDITIONAL_LEFS         = $(PLATFORM_DIR)/lef/fakeram7_64x25.lef \
                                 $(PLATFORM_DIR)/lef/fakeram7_64x28.lef \
                                 $(PLATFORM_DIR)/lef/fakeram7_256x32.lef \
                                 $(PLATFORM_DIR)/lef/fakeram7_128x64.lef \
                                 $(PLATFORM_DIR)/lef/fakeram7_64x256.lef \
                                 $(PLATFORM_DIR)/lef/fakeram7_256x256.lef
export ADDITIONAL_LIBS         = $(PLATFORM_DIR)/lib/NLDM/fakeram7_64x25.lib \
                                 $(PLATFORM_DIR)/lib/NLDM/fakeram7_64x28.lib \
                                 $(PLATFORM_DIR)/lib/NLDM/fakeram7_256x32.lib \
                                 $(PLATFORM_DIR)/lib/NLDM/fakeram7_128x64.lib \
                                 $(PLATFORM_DIR)/lib/NLDM/fakeram7_64x256.lib \
                                 $(PLATFORM_DIR)/lib/NLDM/fakeram7_256x256.lib
EOF
         ;;
   esac
}

validate_orfs_pinned_digest()
{
   local orfs_dir="$1"
   local current_digest=""
   local dirty_status=""
   local tree_state="clean"

   if [[ ! "${ORFS_REQUIRED_DIGEST}" =~ ^[0-9a-fA-F]{40}$ ]]; then
      cat >&2 <<EOF
ERROR: invalid ORFS digest in OPENROAD_ORFS_PINNED_DIGEST.
Current value: ${ORFS_REQUIRED_DIGEST}
Please set OPENROAD_ORFS_PINNED_DIGEST to a full 40-hex git commit digest.
EOF
      exit 1
   fi

   if ! command -v git >/dev/null 2>&1; then
      cat >&2 <<EOF
ERROR: git is required to validate the pinned OpenROAD-flow-scripts digest.
Please install git and retry.
Required ORFS digest: ${ORFS_REQUIRED_DIGEST}
EOF
      exit 1
   fi

   if [[ ! -d "${orfs_dir}/.git" ]]; then
      cat >&2 <<EOF
ERROR: '${orfs_dir}' is not a git clone of OpenROAD-flow-scripts.
Bambu requires a git clone to validate the pinned ORFS digest.
Required ORFS digest: ${ORFS_REQUIRED_DIGEST}

Recovery (recommended):
  git clone --no-checkout --depth 1 ${ORFS_REPO_URL} /path/to/OpenROAD-flow-scripts
  git -C /path/to/OpenROAD-flow-scripts fetch --depth 1 origin ${ORFS_REQUIRED_DIGEST}
  git -C /path/to/OpenROAD-flow-scripts checkout --detach ${ORFS_REQUIRED_DIGEST}
  export OPENROAD_FLOW_DATA_INSTALLDIR=/path/to/OpenROAD-flow-scripts
EOF
      exit 1
   fi

   current_digest="$(git -C "${orfs_dir}" rev-parse HEAD 2>/dev/null || true)"
   if [[ -z "${current_digest}" ]]; then
      cat >&2 <<EOF
ERROR: unable to read ORFS git digest from '${orfs_dir}'.
Please verify that OPENROAD_FLOW_DATA_INSTALLDIR points to a valid ORFS git clone.
Required ORFS digest: ${ORFS_REQUIRED_DIGEST}
EOF
      exit 1
   fi

   dirty_status="$(git -C "${orfs_dir}" status --porcelain --untracked-files=no 2>/dev/null || true)"
   if [[ -n "${dirty_status}" ]]; then
      tree_state="dirty"
   fi

   if [[ "${current_digest}" != "${ORFS_REQUIRED_DIGEST}" || "${tree_state}" != "clean" ]]; then
      cat >&2 <<EOF
ERROR: OpenROAD-flow-scripts digest validation failed.
Expected ORFS digest: ${ORFS_REQUIRED_DIGEST}
Found ORFS digest:    ${current_digest}
Working tree state:   ${tree_state}

Please use the exact pinned ORFS revision and a clean tracked-file tree.

Recovery from an existing clone:
  git -C "${orfs_dir}" fetch --depth 1 origin ${ORFS_REQUIRED_DIGEST}
  git -C "${orfs_dir}" checkout --detach ${ORFS_REQUIRED_DIGEST}
  git -C "${orfs_dir}" reset --hard ${ORFS_REQUIRED_DIGEST}

Recovery from scratch:
  git clone --no-checkout --depth 1 ${ORFS_REPO_URL} /path/to/OpenROAD-flow-scripts
  git -C /path/to/OpenROAD-flow-scripts fetch --depth 1 origin ${ORFS_REQUIRED_DIGEST}
  git -C /path/to/OpenROAD-flow-scripts checkout --detach ${ORFS_REQUIRED_DIGEST}
  export OPENROAD_FLOW_DATA_INSTALLDIR=/path/to/OpenROAD-flow-scripts
EOF
      exit 1
   fi
}

is_simple_orfs_module_list()
{
   local raw="$1"
   local token=""

   if [[ -z "${raw}" ]]; then
      return 1
   fi

   for token in ${raw}; do
      if [[ "${token}" == -* ]]; then
         return 1
      fi
      if [[ ! "${token}" =~ ^[[:alnum:]_.$\\]+$ ]]; then
         return 1
      fi
   done

   return 0
}

resolve_orfs_module_names()
{
   local rtlil_file="$1"
   local optional_keep="$2"
   local required_keep="$3"
   local optional_retime="$4"
   local required_retime="$5"

   python3 - "${rtlil_file}" "${optional_keep}" "${required_keep}" "${optional_retime}" "${required_retime}" <<'PY'
import pathlib
import sys

rtlil_path = pathlib.Path(sys.argv[1])
optional_keep = sys.argv[2].split()
required_keep = sys.argv[3].split()
optional_retime = sys.argv[4].split()
required_retime = sys.argv[5].split()

if not rtlil_path.is_file():
   raise SystemExit(f"ERROR: canonicalized RTLIL not found: {rtlil_path}")

module_names = []
seen = set()
for line in rtlil_path.read_text(encoding="utf-8").splitlines():
   if not line.startswith("module "):
      continue
   tokens = line.split()
   if len(tokens) < 2:
      continue
   name = tokens[1]
   if name not in seen:
      seen.add(name)
      module_names.append(name)

def resolve_module_names(requested_name: str, *, required: bool) -> list[str]:
   if requested_name in seen:
      return [requested_name]

   escaped_name = "\\" + requested_name
   if escaped_name in seen:
      return [escaped_name]

   suffix = "\\" + requested_name
   matches = [module_name for module_name in module_names if module_name.endswith(suffix)]
   if matches:
      return matches
   if required:
      raise SystemExit(
         f"ERROR: unable to resolve ORFS module name '{requested_name}' in {rtlil_path}."
      )
   return []

keep_modules = []
seen_keep = set()
for requested_name, required in [(name, False) for name in optional_keep] + [(name, True) for name in required_keep]:
   for resolved in resolve_module_names(requested_name, required=required):
      if resolved not in seen_keep:
         seen_keep.add(resolved)
         keep_modules.append(resolved)

retime_modules = []
seen_retime = set()
for requested_name, required in [(name, False) for name in optional_retime] + [(name, True) for name in required_retime]:
   for resolved in resolve_module_names(requested_name, required=required):
      if resolved not in seen_retime:
         seen_retime.add(resolved)
         retime_modules.append(resolved)

# Tcl list parsing in 'foreach module $::env(SYNTH_KEEP_MODULES)' consumes one
# backslash level, so preserve RTLIL escaped identifiers by doubling '\'.
keep_value = " ".join(module_name.replace("\\", "\\\\") for module_name in keep_modules)
retime_value = " ".join(module_name.replace("\\", "\\\\") for module_name in retime_modules)

sys.stdout.write(keep_value)
sys.stdout.write("\0")
sys.stdout.write(retime_value)
PY
}

create_custom_orfs_synth_script()
{
   local source_synth_script="$1"
   local destination_synth_script="$2"

   python3 - "${source_synth_script}" "${destination_synth_script}" <<'PY'
import pathlib
import sys

source = pathlib.Path(sys.argv[1])
destination = pathlib.Path(sys.argv[2])

if not source.is_file():
   raise SystemExit(f"ERROR: ORFS synth.tcl not found: {source}")

text = source.read_text(encoding="utf-8")
old = """if { [env_var_exists_and_non_empty SYNTH_RETIME_MODULES] } {\n  select $::env(SYNTH_RETIME_MODULES)\n  opt -fast -full\n  memory_map\n  opt -full\n  techmap\n  abc -dff -script $::env(SCRIPTS_DIR)/abc_retime.script\n  select -clear\n}\n"""
new = """if { [env_var_exists_and_non_empty SYNTH_RETIME_MODULES] } {\n  foreach module $::env(SYNTH_RETIME_MODULES) {\n    select -module $module\n    opt -fast -full\n    memory_map\n    opt -full\n    techmap\n    abc -dff -script $::env(SCRIPTS_DIR)/abc_retime.script\n    select -clear\n  }\n}\n"""
if old not in text:
   raise SystemExit("ERROR: ORFS synth.tcl retime block not found while preparing custom synth script.")

destination.parent.mkdir(parents=True, exist_ok=True)
destination.write_text(text.replace(old, new, 1), encoding="utf-8")
PY
}

run_orfs_backend()
{
   WORK_DIR="${CURR_WORKDIR:-$(pwd)}"
   WORK_DIR_CONTAINER="${OPENROAD_DOCKER_WORK_DIR_CONTAINER:-/work}"
   if [[ "${WORK_DIR_CONTAINER}" == "/" ]]; then
      echo "ERROR: OPENROAD_DOCKER_WORK_DIR_CONTAINER cannot be '/'." >&2
      echo "Use an absolute path such as /work or /tmp/openroad-work." >&2
      exit 1
   fi
   WORK_DIR_CONTAINER="${WORK_DIR_CONTAINER%/}"
   if [[ -z "${WORK_DIR_CONTAINER}" || "${WORK_DIR_CONTAINER}" != /* ]]; then
      cat >&2 <<EOF
ERROR: OPENROAD_DOCKER_WORK_DIR_CONTAINER must be an absolute path.
Current value: ${OPENROAD_DOCKER_WORK_DIR_CONTAINER:-${WORK_DIR_CONTAINER}}

Please set OPENROAD_DOCKER_WORK_DIR_CONTAINER to an absolute container path.
Examples:
  export OPENROAD_DOCKER_WORK_DIR_CONTAINER=/work
  export OPENROAD_DOCKER_WORK_DIR_CONTAINER=/tmp/openroad-work
EOF
      exit 1
   fi
   local use_docker="${OPENROAD_USE_DOCKER:-1}"
   local orfs_mode="image"

   if [[ -n "${OPENROAD_FLOW_DATA_INSTALLDIR:-}" ]]; then
      orfs_mode="repo"
      if [[ "${OPENROAD_FLOW_DATA_INSTALLDIR}" != /* ]]; then
         cat >&2 <<EOF
ERROR: OPENROAD_FLOW_DATA_INSTALLDIR must be an absolute path.
Current value: ${OPENROAD_FLOW_DATA_INSTALLDIR}

Please set OPENROAD_FLOW_DATA_INSTALLDIR to an absolute directory path.
Example:
  export OPENROAD_FLOW_DATA_INSTALLDIR=/path/to/OpenROAD-flow-scripts

If you currently have a relative path, convert it to absolute first, e.g.:
  export OPENROAD_FLOW_DATA_INSTALLDIR="\$(realpath "${OPENROAD_FLOW_DATA_INSTALLDIR}")"
EOF
         exit 1
      fi
      if [[ ! -d "${OPENROAD_FLOW_DATA_INSTALLDIR}" ]]; then
         echo "ERROR: OPENROAD_FLOW_DATA_INSTALLDIR is not a directory: ${OPENROAD_FLOW_DATA_INSTALLDIR}" >&2
         exit 1
      fi
      if [[ ! -d "${OPENROAD_FLOW_DATA_INSTALLDIR}/flow" ]]; then
         echo "ERROR: '${OPENROAD_FLOW_DATA_INSTALLDIR}/flow' not found." >&2
         exit 1
      fi
      validate_orfs_pinned_digest "${OPENROAD_FLOW_DATA_INSTALLDIR}"
   elif [[ "${use_docker}" -eq 0 ]]; then
      cat >&2 <<'EOF'
ERROR: OPENROAD_FLOW_DATA_INSTALLDIR is required when OPENROAD_USE_DOCKER=0.
Non-docker mode needs a local clone of OpenROAD-flow-scripts.

Set OPENROAD_FLOW_DATA_INSTALLDIR to the directory containing a clone of:
  https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts.git
EOF
      exit 1
   fi

   local verilog_files_host=""
   local verilog_files_container=""
   local source_file=""
   for source_file in ${VERILOG_FILES}; do
      local abs_source="${source_file}"
      if [[ "${abs_source}" != /* ]]; then
         abs_source="${WORK_DIR}/${abs_source}"
      fi
      verilog_files_host+="${abs_source} "
      if [[ "${abs_source}" == "${WORK_DIR}"* ]]; then
         verilog_files_container+="${WORK_DIR_CONTAINER}${abs_source#"${WORK_DIR}"} "
      else
         verilog_files_container+="${abs_source} "
      fi
   done

   local config_verilog_files="${verilog_files_host}"
   local config_sdc_file="${SDC_FILE}"
   if [[ "${use_docker}" -eq 1 ]]; then
      config_verilog_files="${verilog_files_container}"
      if [[ "${SDC_FILE}" == "${WORK_DIR}"* ]]; then
         config_sdc_file="${WORK_DIR_CONTAINER}${SDC_FILE#"${WORK_DIR}"}"
      fi
   fi

   local use_fixed_floorplan=0
   if [[ -n "${DIE_AREA:-}" || -n "${CORE_AREA:-}" ]]; then
      if [[ -z "${DIE_AREA:-}" || -z "${CORE_AREA:-}" ]]; then
         echo "ERROR: DIE_AREA and CORE_AREA must be set together when using fixed floorplan." >&2
         echo "Set both DIE_AREA and CORE_AREA, or unset both and use CORE_UTILIZATION." >&2
         exit 1
      fi
      use_fixed_floorplan=1
      # Force fixed floorplan path by disabling utilization-based initialization.
      unset CORE_UTILIZATION
   fi

   local config_mk="${WORK_DIR}/config.mk"
   local config_mk_container="${config_mk}"
   if [[ "${config_mk}" == "${WORK_DIR}"* ]]; then
      config_mk_container="${WORK_DIR_CONTAINER}${config_mk#"${WORK_DIR}"}"
   fi
   cat > "${config_mk}" <<EOF
export PLATFORM               = ${PLATFORM}
export DESIGN_NAME            = ${DESIGN_NAME}
export VERILOG_FILES          = ${config_verilog_files}
export SDC_FILE               = ${config_sdc_file}
export SKIP_LAST_GASP ?= ${SKIP_LAST_GASP:-1}
EOF
   if [[ "${use_fixed_floorplan}" -eq 1 ]]; then
      echo "export DIE_AREA               = ${DIE_AREA}" >> "${config_mk}"
      echo "export CORE_AREA              = ${CORE_AREA}" >> "${config_mk}"
   else
      if [[ -z "${CORE_UTILIZATION:-}" ]]; then
         echo "ERROR: CORE_UTILIZATION is not set." >&2
         echo "Set CORE_UTILIZATION for auto floorplan, or set both DIE_AREA and CORE_AREA for fixed floorplan." >&2
         exit 1
      fi
      local core_utilization="${CORE_UTILIZATION}"
      core_utilization="$(echo "${core_utilization}" | sed -E "s/^[[:space:]]+//; s/[[:space:]]+$//; s/^\"(.*)\"$/\\1/; s/^'(.*)'$/\\1/")"
      if [[ -z "${core_utilization}" ]]; then
         echo "ERROR: CORE_UTILIZATION resolves to an empty value after quote trimming." >&2
         echo "Provide a numeric value, e.g. CORE_UTILIZATION=10" >&2
         exit 1
      fi
      echo "export CORE_UTILIZATION       = ${core_utilization}" >> "${config_mk}"
   fi
   if [[ -n "${FP_CORE_SPACE:-}" ]]; then
      echo "export FP_CORE_SPACE          = ${FP_CORE_SPACE}" >> "${config_mk}"
   fi
   if [[ -n "${CELL_PAD_IN_SITES_GLOBAL_PLACEMENT:-}" ]]; then
      echo "export CELL_PAD_IN_SITES_GLOBAL_PLACEMENT = ${CELL_PAD_IN_SITES_GLOBAL_PLACEMENT}" >> "${config_mk}"
   fi
   if [[ -n "${CELL_PAD_IN_SITES_DETAIL_PLACEMENT:-}" ]]; then
      echo "export CELL_PAD_IN_SITES_DETAIL_PLACEMENT = ${CELL_PAD_IN_SITES_DETAIL_PLACEMENT}" >> "${config_mk}"
   fi
   if [[ -n "${PLACE_DENSITY_LB_ADDON:-}" ]]; then
      echo "export PLACE_DENSITY_LB_ADDON = ${PLACE_DENSITY_LB_ADDON}" >> "${config_mk}"
   fi
   if [[ -n "${PLACE_DENSITY:-}" ]]; then
      echo "export PLACE_DENSITY          = ${PLACE_DENSITY}" >> "${config_mk}"
   fi
   append_asap7_fakeram_assets_config "${config_mk}"

   local flow_makefile=""
   local docker_image="${OR_IMAGE:-openroad/orfs:v3.0-4639-ga2bb042b6}"
   local flow_variant="${FLOW_VARIANT:-base}"
   local results_dir_host="${WORK_DIR}/results/${PLATFORM}/${DESIGN_NAME}/${flow_variant}"
   local canonicalize_target_host="${results_dir_host}/1_1_yosys_canonicalize.rtlil"
   local canonicalize_target="${canonicalize_target_host}"
   local user_requested_keep_modules="${SYNTH_KEEP_MODULES:-}"
   local user_requested_retime_modules="${SYNTH_RETIME_MODULES:-}"
   local requested_keep_modules="${user_requested_keep_modules}"
   local requested_retime_modules="${user_requested_retime_modules}"
   local resolved_keep_modules="${requested_keep_modules}"
   local resolved_retime_modules="${requested_retime_modules}"
   local default_keep_modules_to_resolve=""
   local default_retime_modules_to_resolve=""
   local keep_to_resolve=""
   local retime_to_resolve=""
   local resolve_keep_modules=0
   local resolve_retime_modules=0
   local custom_synth_script_host="${WORK_DIR}/orfs_scripts/synth_multi_retime.tcl"
   local custom_synth_script="${custom_synth_script_host}"
   local source_synth_script_host=""

   if [[ "${orfs_mode}" == "repo" ]]; then
      flow_makefile="${OPENROAD_FLOW_DATA_INSTALLDIR}/flow/Makefile"
      source_synth_script_host="${OPENROAD_FLOW_DATA_INSTALLDIR}/flow/scripts/synth.tcl"
   else
      flow_makefile="/OpenROAD-flow-scripts/flow/Makefile"
   fi

   if [[ "${use_docker}" -eq 1 ]]; then
      canonicalize_target="${WORK_DIR_CONTAINER}${canonicalize_target_host#"${WORK_DIR}"}"
      custom_synth_script="${WORK_DIR_CONTAINER}${custom_synth_script_host#"${WORK_DIR}"}"
   fi

   run_orfs_make_cmd()
   {
      local -a make_targets=("$@")
      if [[ "${use_docker}" -eq 1 ]]; then
         local -a docker_args=(
            "--rm"
            -e FLOW_HOME=/OpenROAD-flow-scripts/flow/
            -e "NUM_CORES=${NUM_CORES}"
            -e "WORK_HOME=${WORK_DIR_CONTAINER}"
            -e "DESIGN_CONFIG=${config_mk_container}"
            -w "/OpenROAD-flow-scripts/flow"
            -v "${WORK_DIR}:${WORK_DIR_CONTAINER}:Z"
         )
         if [[ "${orfs_mode}" == "repo" ]]; then
            docker_args+=(-v "${OPENROAD_FLOW_DATA_INSTALLDIR}/flow:/OpenROAD-flow-scripts/flow:Z")
         fi
         local userns_remap
         userns_remap="$(docker info --format '{{.SecurityOptions}}' 2>/dev/null | grep -i userns || true)"
         if [[ -n "${userns_remap}" ]]; then
            echo "WARNING: Docker User Namespaces enabled on this machine, make sure mapped users have permissions to read/write the output directory."
         else
            docker_args+=(-u "$(id -u):$(id -g)")
         fi
         local env_assignment=""
         for env_assignment in "${orfs_extra_env_vars[@]}"; do
            docker_args+=(-e "${env_assignment}")
         done
         local make_cmd="make -f /OpenROAD-flow-scripts/flow/Makefile"
         local make_target=""
         for make_target in "${make_targets[@]}"; do
            printf -v make_cmd '%s %q' "${make_cmd}" "${make_target}"
         done
         docker run "${docker_args[@]}" "${docker_image}" \
            bash -lc "set -e; if [[ -f ../env.sh ]]; then . ../env.sh; elif [[ -f ../setup-env.sh ]]; then . ../setup-env.sh; fi; ${make_cmd}"
      else
         local -a host_make_cmd=(
            make
            "NUM_CORES=${NUM_CORES}"
            "WORK_HOME=${WORK_DIR}"
            "DESIGN_CONFIG=${config_mk}"
            -f "${flow_makefile}"
         )
         if [[ ${#make_targets[@]} -ne 0 ]]; then
            host_make_cmd+=("${make_targets[@]}")
         fi
         (
            set -e
            cd "$(dirname "${flow_makefile}")"
            if [[ -f ../env.sh ]]; then
               . ../env.sh
            elif [[ -f ../setup-env.sh ]]; then
               . ../setup-env.sh
            fi
            env "${orfs_extra_env_vars[@]}" "${host_make_cmd[@]}"
         )
      fi
   }

   local -a orfs_extra_env_vars=()
   local simple_user_keep_modules=0
   local simple_user_retime_modules=0
   if is_simple_orfs_module_list "${user_requested_keep_modules}"; then
      simple_user_keep_modules=1
   fi
   if is_simple_orfs_module_list "${user_requested_retime_modules}"; then
      simple_user_retime_modules=1
   fi

   if [[ -z "${user_requested_keep_modules}" || "${simple_user_keep_modules}" -eq 1 ]]; then
      default_keep_modules_to_resolve="${ORFS_DEFAULT_MODULES_TO_KEEP_AND_RETIME}"
      requested_keep_modules="${default_keep_modules_to_resolve}${user_requested_keep_modules:+ ${user_requested_keep_modules}}"
      resolved_keep_modules="${requested_keep_modules}"
   fi
   if [[ -z "${user_requested_retime_modules}" || "${simple_user_retime_modules}" -eq 1 ]]; then
      default_retime_modules_to_resolve="${ORFS_DEFAULT_MODULES_TO_KEEP_AND_RETIME}"
      requested_retime_modules="${default_retime_modules_to_resolve}${user_requested_retime_modules:+ ${user_requested_retime_modules}}"
      resolved_retime_modules="${requested_retime_modules}"
   fi

   if is_simple_orfs_module_list "${requested_keep_modules}"; then
      resolve_keep_modules=1
      if [[ -n "${user_requested_keep_modules}" ]]; then
         keep_to_resolve="${user_requested_keep_modules}"
      fi
   fi
   if is_simple_orfs_module_list "${requested_retime_modules}"; then
      resolve_retime_modules=1
      if [[ -n "${user_requested_retime_modules}" ]]; then
         retime_to_resolve="${user_requested_retime_modules}"
      fi
   fi

   if [[ "${resolve_keep_modules}" -eq 1 || "${resolve_retime_modules}" -eq 1 ]]; then
      run_orfs_make_cmd "${canonicalize_target}"
      local -a resolved_module_vars=()
      mapfile -d '' -t resolved_module_vars < <(
         resolve_orfs_module_names "${canonicalize_target_host}" "${default_keep_modules_to_resolve}" \
            "${keep_to_resolve}" "${default_retime_modules_to_resolve}" "${retime_to_resolve}"
      )
      if [[ "${resolve_keep_modules}" -eq 1 ]]; then
         resolved_keep_modules="${resolved_module_vars[0]}"
         echo "Resolved SYNTH_KEEP_MODULES to elaborated Yosys modules: ${resolved_keep_modules}"
      fi
      if [[ "${resolve_retime_modules}" -eq 1 ]]; then
         resolved_retime_modules="${resolved_module_vars[1]}"
         echo "Resolved SYNTH_RETIME_MODULES to elaborated Yosys modules: ${resolved_retime_modules}"
      fi
   fi

   if [[ -n "${resolved_keep_modules}" ]]; then
      orfs_extra_env_vars+=("SYNTH_KEEP_MODULES=${resolved_keep_modules}")
   fi
   if [[ -n "${resolved_retime_modules}" ]]; then
      orfs_extra_env_vars+=("SYNTH_RETIME_MODULES=${resolved_retime_modules}")
      if [[ -z "${source_synth_script_host}" ]]; then
         source_synth_script_host="${WORK_DIR}/orfs_scripts/original_synth.tcl"
         mkdir -p "$(dirname "${source_synth_script_host}")"
         docker run --rm "${docker_image}" \
            bash -lc 'cat /OpenROAD-flow-scripts/flow/scripts/synth.tcl' > "${source_synth_script_host}"
      fi
      create_custom_orfs_synth_script "${source_synth_script_host}" "${custom_synth_script_host}"
      orfs_extra_env_vars+=("SYNTH_SCRIPT=${custom_synth_script}")
   fi

   run_orfs_make_cmd
   run_orfs_make_cmd metadata-generate

   local metadata_json="${WORK_DIR}/reports/${PLATFORM}/${DESIGN_NAME}/${FLOW_VARIANT}/metadata.json"
   local results_sdc="${WORK_DIR}/results/${PLATFORM}/${DESIGN_NAME}/${FLOW_VARIANT}/2_floorplan.sdc"
   local out_xml="${SWD}/$(bambu_results /application/backend@bambu_results)"
   python3 - "${metadata_json}" "${results_sdc}" "${out_xml}" <<'PY'
import json
import pathlib
import re
import sys
import xml.etree.ElementTree as ET

def as_float(value):
   if isinstance(value, (int, float)):
      return float(value)
   if isinstance(value, str):
      m = re.search(r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?", value.strip())
      if m:
         try:
            return float(m.group(0))
         except ValueError:
            return None
   return None

def first_numeric(data, keys):
   for key in keys:
      if key in data:
         val = as_float(data[key])
         if val is not None:
            return val
   return None

def period_from_metadata(data):
   clocks = data.get("constraints__clocks__details")
   if not isinstance(clocks, list):
      return None
   for clk in clocks:
      if isinstance(clk, str):
         m = re.search(r":\s*([-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?)", clk)
         if m:
            return float(m.group(1))
   return None

def requirement_from_sdc(sdc_path):
   try:
      content = sdc_path.read_text(encoding="utf-8")
   except OSError:
      return None
   m = re.search(r"create_clock[\s\S]*?-period\s+([-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?)", content)
   if m:
      return float(m.group(1))
   normalized = re.sub(r"\\\s*\n", " ", content)
   num_re = r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?"
   vals = []
   for line in normalized.splitlines():
      if "set_max_delay" in line:
         numbers = re.findall(num_re, line)
         if numbers:
            vals.append(float(numbers[-1]))
   return min(vals) if vals else None

metadata_path = pathlib.Path(sys.argv[1])
sdc_path = pathlib.Path(sys.argv[2])
out_xml = pathlib.Path(sys.argv[3])
if not metadata_path.is_file():
   print(f"ERROR: metadata file not found: {metadata_path}", file=sys.stderr)
   sys.exit(1)

data = json.loads(metadata_path.read_text(encoding="utf-8"))
design_area = first_numeric(data, ["finish__design__instance__area", "finish__design__instance__area__stdcell"])
tot_power = first_numeric(data, ["finish__power__total", "finish__power__internal__total"])
setup_ws = first_numeric(data, ["finish__timing__setup__ws", "finish__timing__setup__wns"])
timing_requirement = period_from_metadata(data) or requirement_from_sdc(sdc_path)
if design_area is None or tot_power is None or setup_ws is None or timing_requirement is None:
   print("ERROR: missing ORFS metrics for backend XML generation", file=sys.stderr)
   sys.exit(1)

design_delay_ps = timing_requirement - setup_ws
design_delay_ns = design_delay_ps / 1000.0
clock_slack_ns = setup_ws / 1000.0
frequency_mhz = 1000.0 / design_delay_ns if design_delay_ns else 0.0
out_xml.parent.mkdir(parents=True, exist_ok=True)
root = ET.Element("application")
ET.SubElement(root, "resources",
              AREA=f"{design_area:g}",
              BRAMS="0",
              DRAMS="0",
              CLOCK_SLACK=f"{clock_slack_ns:g}",
              DSPS="0",
              FREQUENCY=f"{frequency_mhz:g}",
              PERIOD=f"{design_delay_ns:g}",
              REGISTERS="0",
              POWER=f"{tot_power:g}",
              DELAY=f"{design_delay_ns:g}",
              SLACK=f"{clock_slack_ns:g}")
ET.ElementTree(root).write(out_xml, encoding="utf-8", xml_declaration=True)
PY
}

export DESIGN_NAME="$(bambu_results /application/top_module@name)"
: "${DESIGN_NICKNAME:=${DESIGN_NAME}}"
export DESIGN_NICKNAME
export SDC_FILE="${SWD}/constraints.sdc"
clock_constraint_ps="$(bambu_results /application/target@period_ps)"
if [[ -z "${clock_constraint_ps}" ]]; then
   clock_constraint_ps="$(bambu_results /application/target@period)"
fi

if $(bambu_results /application/top_module@combinational); then
   echo "set_max_delay ${clock_constraint_ps} -from [all_inputs] -to [all_outputs]" > "${SDC_FILE}"
else
   echo "create_clock $(bambu_results /application/top_module@clock_name) -period ${clock_constraint_ps}" > "${SDC_FILE}"
fi
sdc_ext_file="$(bambu_results /application/target@sdc_ext_file)"
if [[ -n "${sdc_ext_file}" ]]; then
   echo "source ${sdc_ext_file}" >> "${SDC_FILE}"
fi

WORK_DIR="${CURR_WORKDIR:-$(pwd)}"
export VERILOG_FILES=""
for src in $(bambu_results /application/outputs/file); do
   if [[ "${src}" = /* ]]; then
      VERILOG_FILES+="${src} "
   else
      VERILOG_FILES+="${WORK_DIR}/${src} "
   fi
done
: "${FLOW_VARIANT:=base}"
export FLOW_VARIANT

resolve_num_cores
run_orfs_backend
exit 0
