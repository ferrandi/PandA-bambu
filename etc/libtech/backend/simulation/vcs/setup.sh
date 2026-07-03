#!/usr/bin/env bash

VFLAGS=""
case "$(bambu_results /application/sources@cflags)" in
  *-m32*) VFLAGS="+define+__M32" ;;
  *-mx32*) VFLAGS="+define+__MX32" ;;
  *) VFLAGS="+define+__M64" ;;
esac
if [ "x$(bambu_results /application/backend@vcd)" != "x" ]; then
  VFLAGS+=" +define+GENERATE_VCD"
fi
if $(bambu_results /application/backend@discrepancy) ; then
  VFLAGS+=" +define+GENERATE_VCD_DISCREPANCY"
fi
VFLAGS+=" +define+__BAMBU_SIM__"

for inc in $(bambu_results /application/outputs/include)
do
  VFLAGS+=" +incdir+${inc}"
done

WORK_DIR="${SWD}/work"
mkdir -p "${WORK_DIR}"
if [ -d "${WORK_DIR}" ]; then
  echo "WORK  > default" > synopsys_sim.setup
  echo "default : ${WORK_DIR}" >> synopsys_sim.setup
fi

COMMON_FLAGS="-full64 -nc"
case "$(bambu_results /application/sources@cflags)" in
  *-m32* | *-mx32*)
    COMMON_FLAGS="-nc"
    if [ "${VCS_TARGET_ARCH}" = "linux64" ] || [ "${VCS_TARGET_ARCH}" = "suse64" ]; then
      COMMON_FLAGS="-full64 -nc"
    fi
    ;;
esac
if $(bambu_results /application/backend@assert) ; then
  COMMON_FLAGS+=" -psl"
fi

SV_FLAGS="${VFLAGS} -sverilog ${COMMON_FLAGS}"
VHD_FLAGS="-vhdl08 ${COMMON_FLAGS}"

for src in $(bambu_results /application/outputs/file) $(bambu_results /application/outputs/testbench);
do
   case ${src} in
      *.v | *.sv) vlogan ${SV_FLAGS} "${src}" ;;
      *.vhd | *.vhdl) vhdlan ${VHD_FLAGS} "${src}" ;;
      *) echo "Unknown source file type: ${src}" 1>&2; exit -1 ;;
   esac
done

BAMBU_IPC_SIM_CMD="vcs ${COMMON_FLAGS} work.clocked_bambu_testbench +vpi ${SWD}/libmdpi.so -R"
if $(bambu_results /application/backend@assert) ; then
  BAMBU_IPC_SIM_CMD+=" -check_all -psl +lint=all -debug_access+all -deraceclockdata -sn=+rdr"
fi
BAMBU_IPC_SIM_CMD+=" 2>&1 | tee ${SWD}/simulation.log; exit \${PIPESTATUS[0]};"
