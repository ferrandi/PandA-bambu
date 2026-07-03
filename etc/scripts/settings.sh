BACKEND_PATHS=()
BACKEND_PATHS+=("/opt/mentor")
BACKEND_PATHS+=("/opt/synopsys/vcs")
BACKEND_PATHS+=("/opt/altera" "/opt/intelFPGA")
BACKEND_PATHS+=("/opt/diamond" "/usr/local/diamond")
BACKEND_PATHS+=("/opt/NanoXplore")
BACKEND_PATHS+=("/opt/Xilinx/Vivado" "/opt/Xilinx/Vitis")
IFS=":"
export BAMBU_HLS_BACKEND_PATH="${BACKEND_PATHS[*]}:${PATH}"
export BAMBU_HLS="$(dirname $(readlink -e ${BASH_SOURCE[0]:-${(%):-%x}}))"
if [ -n "${PATH}" ]; then
  export PATH=${BAMBU_HLS}/bin:$PATH
else
  export PATH=${BAMBU_HLS}/bin
fi

if [ -e "${BAMBU_HLS}/compilers/settings.sh" ]; then
  source ${BAMBU_HLS}/compilers/settings.sh
fi
