#!/usr/bin/env bash
# Source-only ModelSim lookup shared by Bambu's launcher and regression gates.
# Keep the PATH-first/backend-root fallback order aligned with launch.sh users.

bambu_modelsim_resolve() {
  local candidate backend_path root
  local -a roots=() search_roots=()

  BAMBU_MODELSIM_PATH=""
  BAMBU_MODELSIM_BIN=""
  BAMBU_MODELSIM_ROOT=""
  BAMBU_MODELSIM_SOURCE=""

  # Match the production launcher: a simulator already on PATH always wins.
  if candidate=$(command -v vsim 2>/dev/null) && [[ -n $candidate ]]; then
    BAMBU_MODELSIM_SOURCE=PATH
  else
    backend_path=${BAMBU_HLS_BACKEND_PATH:-}
    if [[ -z $backend_path ]]; then
      echo "ModelSim vsim not found on PATH and BAMBU_HLS_BACKEND_PATH is empty" >&2
      return 1
    fi

    IFS=: read -r -a roots <<< "$backend_path"
    for root in "${roots[@]}"; do
      # Empty components must not make find search the current directory.
      [[ -n $root && -d $root ]] && search_roots+=("$root")
    done
    if ((${#search_roots[@]} == 0)); then
      echo "ModelSim vsim not found: BAMBU_HLS_BACKEND_PATH has no existing roots" >&2
      return 1
    fi

    # Deliberately keep find's first match, as Bambu does. Do not sort versions.
    # find may report a traversal error in an earlier root after already
    # printing a valid match from a later root. Preserve stdout in that case,
    # as the original production launcher did; an empty result still fails
    # closed below.
    candidate=$(find -L "${search_roots[@]}" \( -type f -o -type l \) \
      -path '*/bin/vsim' -print -quit 2>/dev/null || true)
    if [[ -z $candidate ]]; then
      echo "ModelSim vsim not found in BAMBU_HLS_BACKEND_PATH roots" >&2
      return 1
    fi
    BAMBU_MODELSIM_SOURCE=BAMBU_HLS_BACKEND_PATH
  fi

  if [[ ! -x $candidate ]]; then
    echo "ModelSim vsim candidate is not executable: $candidate" >&2
    return 1
  fi

  [[ $candidate == /* ]] || candidate="$PWD/$candidate"

  # Preserve the invocation path and basename: vsim can be a dispatcher
  # symlink to vco, and the executable name selects the simulator mode.
  BAMBU_MODELSIM_PATH=$candidate
  BAMBU_MODELSIM_BIN=${candidate%/*}
  BAMBU_MODELSIM_ROOT=$(dirname -- "$BAMBU_MODELSIM_BIN")
}
