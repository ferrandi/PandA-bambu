BACKEND_PATHS=()
BACKEND_PATHS+=("/opt/mentor")
BACKEND_PATHS+=("/opt/synopsys/vcs")
BACKEND_PATHS+=("/opt/altera" "/opt/intelFPGA")
BACKEND_PATHS+=("/opt/diamond" "/usr/local/diamond")
BACKEND_PATHS+=("/opt/NanoXplore")
BACKEND_PATHS+=("/opt/Xilinx/Vivado" "/opt/Xilinx/Vitis")

_bambu_all="${BAMBU_HLS_BACKEND_PATH:-}:$(IFS=":"; printf '%s' "${BACKEND_PATHS[*]}"):${PATH:-}"
_bambu_entries=()
while [ -n "$_bambu_all" ]; do
  case "$_bambu_all" in
    *:*)
      _p="${_bambu_all%%:*}"
      _bambu_all="${_bambu_all#*:}"
      ;;
    *)
      _p="$_bambu_all"
      _bambu_all=""
      ;;
  esac
  if [ -n "$_p" ] && [ -d "$_p" ]; then
    _dup=""
    for _e in "${_bambu_entries[@]}"; do
      if [ "${_e}" = "${_p}" ]; then
        _dup="1"
        break
      fi
    done
    if [ -z "${_dup}" ]; then
      _bambu_entries+=("${_p}")
    fi
  fi
done
unset _bambu_all _e _dup

_bambu_backend=""
for _p in "${_bambu_entries[@]}"; do
  _bambu_backend+="${_bambu_backend:+:}${_p}"
done
unset _bambu_entries _p

export BAMBU_HLS_BACKEND_PATH="${_bambu_backend}"
unset _bambu_backend
export BAMBU_HLS="$(dirname $(readlink -e ${BASH_SOURCE[0]:-${(%):-%x}}))"
if [ -n "${PATH}" ]; then
  case ":${PATH}:" in
    *":${BAMBU_HLS}/bin:"*) ;;
    *) export PATH=${BAMBU_HLS}/bin:$PATH ;;
  esac
else
  export PATH=${BAMBU_HLS}/bin
fi

if [ -e "${BAMBU_HLS}/compilers/settings.sh" ]; then
  source ${BAMBU_HLS}/compilers/settings.sh
fi
