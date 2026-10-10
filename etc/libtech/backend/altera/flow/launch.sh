#!/usr/bin/env bash
SWD="$(dirname $(readlink -e $0))"

if ! command -v quartus_sh 2>&1 > /dev/null; then
  QUARTUS_ROOT=`dirname $(find -L ${BAMBU_HLS_BACKEND_PATH//:/ } \( -type f -o -type l \) -path "*/quartus/bin/quartus_sh" 2> /dev/null | sort -V | tail -n1) 2> /dev/null`
  if [ -z "${QUARTUS_ROOT}" ]; then
    echo "Intel Quartus tool not found"
    exit -1
  fi
  QUARTUS_BIN="${QUARTUS_ROOT}"
  QUARTUS_ROOT=`readlink -e ${QUARTUS_ROOT}/../..`
  echo "Intel Quartus root: ${QUARTUS_ROOT}"
fi

(
  set -e
  export PATH="${QUARTUS_BIN}:${PATH}"
  if [ $(quartus_sh --version | grep Version | sed -E 's/Version ([0-9]+).*/\1/') -lt 14 ]; then 
    if quartus_sh --help | grep -q '\-\-64bit'; then
        QUARTUS_FLAGS="--64bit"
    fi
  fi
  quartus_sh ${QUARTUS_FLAGS} -t "${SWD}/quartus_setup.tcl"
  quartus_sh ${QUARTUS_FLAGS} -t "${SWD}/quartus_flow.tcl"
  quartus_sta ${QUARTUS_FLAGS} -t "${SWD}/quartus_sta.tcl"
  quartus_pow ${QUARTUS_FLAGS} -f \
    "$(bambu_results /application/top_module@name).qpf" -c "$(bambu_results /application/top_module@name).qsf" \
    --estimate_power=on --use_vectorless_estimation=on --output_saf=result.saf --output_epe=result.epe --vcd_filter_glitches=on
)
