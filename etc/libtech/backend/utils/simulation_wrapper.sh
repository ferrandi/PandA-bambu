#!/usr/bin/env bash
#######################################################################
#
# simulation_wrapper.sh <simulator_specific_setup.sh>
#
# Available targets:
#  - testbench     : compiles the whole testbench (valid only when TB_SRCS is set)
#  - dyn_driver    : compiles the MDPI driver as a dynamic shared object
#  - static_driver : compiles the MDPI driver as a static library
#
#######################################################################
set -e

sim_elapsed_time=$(date +%s.%N)
function exit_time {
  local et=$(echo "$(date +%s.%N) - $sim_elapsed_time" | bc)
  echo "Sim: Elapsed time: $et"
}
trap exit_time EXIT

: ${OUT_LVL:="$(bambu_results /application@verbosity)"}

CC="$(bambu_results /application/sources@compiler)"
CFLAGS=`echo $(bambu_results /application/sources@cflags) | envsubst`
CFLAGS+=" -fwrapv -flax-vector-conversions -msse2 -fno-strict-aliasing"
CFLAGS+=" -D__builtin_bambu_time_start\(\)= -D__builtin_bambu_time_stop\(\)= -D__BAMBU_SIM__"

case "${CFLAGS}" in
  *-m32*) BEH_CFLAGS+=" -D__M32" ;;
  *-mx32*) BEH_CFLAGS+=" -D__MX32" ;;
  *) BEH_CFLAGS+=" -D__M64" ;;
esac
BEH_CFLAGS+=" -isystem ${BAMBU_HLS}/include/panda -I../../ -D__M_OUT_LVL=${OUT_LVL} -DBAMBU_CONCURRENT_COSIM -O2"


if [ -z "${SYS_ELF}" ]; then
  SYS_ELF="$(bambu_results /application/testbench@elf)"
fi

SIM_DIR="HLS_output/simulation"
if [ -z "${TARGET}" ]; then
  if [ "${SYS_ELF}" == "HLS_output/simulation/testbench" ]; then
    TARGET="testbench"
  else
    TARGET="dyn_driver"
  fi
fi

MAKE_TARGETS=("${TARGET}")
MAKE_VERBOSITY=0
if [ "${PANDA_CI_RUNTIME_LINKAGE_EVIDENCE:-0}" = "1" ]; then
  MAKE_VERBOSITY=1
  if [ "${TARGET}" = "testbench" ]; then
    # Retain the driver evidence without changing the selected executable.
    MAKE_TARGETS+=(dyn_driver)
  fi
fi

make -f "${BAMBU_HLS}/share/panda/libmdpi/Makefile.mk" \
  SIM_DIR="${SIM_DIR}" BEH_DIR="$(dirname $1)" \
  TOP_FNAME="$(bambu_results /application/testbench@symbol)" \
  MTOP_FNAME="$(bambu_results /application/testbench@m_symbol)" \
  MPPTOP_FNAME="$(bambu_results /application/testbench@m_pp_symbol)" \
  CC="${CC}" \
  BEH_CC="${BEH_CC}" \
  CFLAGS="${CFLAGS}" \
  BEH_CFLAGS="${BEH_CFLAGS}" \
  TB_CFLAGS="$(bambu_results /application/testbench@cflags)" \
  SRCS="$(bambu_results /application/sources/file)" \
  PP_SRC="" \
  TB_SRCS="$(bambu_results /application/testbench/file)" \
  V="${MAKE_VERBOSITY}" \
  -j "$(bambu_results /application/backend@parallel)" "${MAKE_TARGETS[@]}"


#######################################################################
###  Simulator specific setup
#######################################################################

echo "Sim: Environment setup"
. "$1"
shift

if [ -z "${BAMBU_IPC_SIM_CMD}" ]; then
   echo "Sim: ERROR: BAMBU_IPC_SIM_CMD not set by simulator setup" 1>&2; exit -1
fi
export BAMBU_IPC_SIM_CMD
if [ ${OUT_LVL} -gt 2 ]; then echo "Sim: BAMBU_IPC_SIM_CMD=\"${BAMBU_IPC_SIM_CMD}\""; fi


#######################################################################
###  Launch simulation
#######################################################################
convert_results() {
  awk 'NR==1{n=split($0,x,/,/);for(i=1;i<=n;i++){gsub(/^[ \t\r\n]+|[ \t\r\n]+$/,"",x[i]);if(x[i]=="")continue;split(x[i],p,/\|/);a=p[1];b=p[2];if(b=="A"||b=="X"){r[++c]=b}else{v=(b-a)/2;r[++c]=(v==int(v)?int(v):v)}}}
  NR==2{rv=$0;gsub(/^[ \t\r\n]+|[ \t\r\n]+$/,"",rv);if(rv=="")rv="A"}
  END{if(rv=="")rv="A";printf"<application><timing><simulation return_value=\"%s\">",rv;for(i=1;i<=c;i++)printf"<run>%s</run>",r[i];print"</simulation></timing></application>"}' "$1"
}

if [ -f "${SYS_ELF}" ] && [ "${TARGET}" != "static_driver" ]; then
  function get_class { readelf -h $1 2> /dev/null | grep Class: | sed -E 's/.*Class:\s*(\w+)/\1/'; }
  sys_elf_class=`get_class ${SYS_ELF}`
  driver_elf_class=`get_class ${SIM_DIR}/libmdpi_driver.so`
  if [ "${TARGET}" != "testbench" ]; then
    if [ "${sys_elf_class}" != "${driver_elf_class}" ]; then
      echo "Sim: ERROR: Wrong system application ELF class: ${sys_elf_class} != ${driver_elf_class}" 1>&2; exit -1;
    fi
    TB_PRELOAD="${SIM_DIR}/libmdpi_driver.so"
  fi
  SYS_LOG="${SIM_DIR}/$(basename ${SYS_ELF}).log"
  echo "Sim: Launch user testbench: LD_PRELOAD=\"${TB_PRELOAD}:${LD_PRELOAD}\" ${SYS_ELF} $@"
  (LD_PRELOAD="${TB_PRELOAD}:$LD_PRELOAD" ${SYS_ELF} "$@" 2>&1 | tee "${SYS_LOG}"; exit ${PIPESTATUS[0]})
  convert_results bambu_time_simulation.txt > "${SWD}/bambu_results.xml"
fi
