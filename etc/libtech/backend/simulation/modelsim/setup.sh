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

OPT_FLAGS="-O5"

SV_FLAGS="${VFLAGS} ${OPT_FLAGS}"
if $(bambu_results /application/backend@assert) ; then
  SV_FLAGS+=" -lint -fsmsingle -hazards -pedanticerrors -fsmverbose w"
fi
SV_FLAGS+=" -sv"

VHD_FLAGS="${OPT_FLAGS} -2008"
if $(bambu_results /application/backend@assert) ; then
  VHD_FLAGS+=" -lint -check_synthesis -fsmsingle -fsmverbose w"
fi

WORK_DIR="${SWD}/work"
if [ -d "${WORK_DIR}" ]; then
  vdel -all -lib "${WORK_DIR}" || true
  rm -rf "${WORK_DIR}"
fi

vlib "${WORK_DIR}"
vmap work "${WORK_DIR}"
sed -i 's/; AssertionFailAction = 1/AssertionFailAction = 2/g' modelsim.ini

for src in $(bambu_results /application/outputs/file) $(bambu_results /application/outputs/testbench) "${BAMBU_HLS}/share/panda/libmdpi/mdpi.c";
do
   case ${src} in
      *.v | *.sv) vlog ${SV_FLAGS} -work work "${src}" ;;
      *.vhd | *.vhdl) vcom ${VHD_FLAGS} -work work "${src}" ;;
      *.c | *.cpp) vlog -ccflags "${BEH_CFLAGS}" ${OPT_FLAGS} -sv -work work "${src}" ;;
      *) echo "Unknown source file type: ${src}" 1>&2; exit -1 ;;
   esac
done

BAMBU_IPC_SIM_CMD="vsim ${VFLAGS} -noautoldlibpath"
if $(bambu_results /application/backend@assert) ; then
  BAMBU_IPC_SIM_CMD+=" -pedanticerrors -assertdebug"
  OPT_FLAGS="+acc -hazards ${OPT_FLAGS}"
fi
BAMBU_IPC_SIM_CMD+=" -c"
if [ -n "${OPT_FLAGS}" ]; then
  BAMBU_IPC_SIM_CMD+=" -voptargs=\"${OPT_FLAGS}\""
fi
BAMBU_IPC_SIM_CMD+=" -do \"set StdArithNoWarnings 1; set StdNumNoWarnings 1; set NumericStdNoWarnings 1; onerror {quit -f -code 1;}; run -all; exit -f;\""
BAMBU_IPC_SIM_CMD+=" work.clocked_bambu_testbench 2>&1 | tee ${SWD}/simulation.log; exit \${PIPESTATUS[0]};"
