#!/usr/bin/env bash

VERILATOR_VERSION="$({ verilator --version 2>/dev/null || echo 0; } | sed -E 's/Verilator ([0-9]+).*/\1/')"

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
ln -sf "${BAMBU_HLS_OUTDIR}/HLS_output" "${WORK_DIR}/HLS_output" || true

OPT="-fstrict-aliasing"
OPT+=" -DCLOCK_PORT_NAME=$(bambu_results /application/top_module@clock_name)"

VPPFLAGS="--cc --exe --Mdir ${WORK_DIR}"
VPPFLAGS+=" -Wno-fatal -Wno-lint"
VPPFLAGS+=" -O3 --unroll-count 10000 --output-split-cfuncs 3000 --output-split-ctrace 3000"
VPPFLAGS+=" -sv ${VFLAGS}"
if [ "x$(bambu_results /application/backend@vcd)" != "x" ]; then
  VPPFLAGS+=" --trace --trace-underscore"
  if ! test verilator --l2-name v 2>&1 | head -n1 | grep -i 'Invalid Option' ; then
    VPPFLAGS+=" --l2-name bambu_testbench"
  fi
  OPT+=" -DVCD_OUT_FILENAME=\\\"$(bambu_results /application/backend@vcd)\\\""
else
  VPPFLAGS+=" --x-assign fast --x-initial fast --noassert"
fi
if test ${VERILATOR_VERSION} -ge 5; then
  VPPFLAGS+=" --no-timing"
fi

VPPTHREADS="$(bambu_results /application/backend@parallel)"
if [ "${VPPTHREADS}" != "1" ]; then
  if test ${VERILATOR_VERSION} -ge 4; then
    VPPFLAGS+=" --threads ${VPPTHREADS}"
  fi
fi

VPPFLAGS+=" --top-module bambu_testbench"

verilator ${VPPFLAGS} "${SWD}/bambu_testbench.cpp" \
  $(bambu_results /application/outputs/file) \
  $(bambu_results /application/outputs/testbench) \
  "${SWD}/libmdpi.so"
if [ $? -ne 0 ]; then 
  exit $?;
fi

make -C "${WORK_DIR}" -j "${VPPTHREADS}" OPT="${OPT}" -f Vbambu_testbench.mk Vbambu_testbench

BAMBU_IPC_SIM_CMD="${WORK_DIR}/Vbambu_testbench 2>&1 | tee ${SWD}/simulation.log; exit \${PIPESTATUS[0]};"
