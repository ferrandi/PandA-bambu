#!/usr/bin/env bash

VFLAGS=""
case "$(bambu_results /application/sources@cflags)" in
  *-m32*) VFLAGS="-define __M32" ;;
  *-mx32*) VFLAGS="-define __MX32" ;;
  *) VFLAGS="-define __M64" ;;
esac
if [ "x$(bambu_results /application/backend@vcd)" != "x" ]; then
  VFLAGS+=" -define GENERATE_VCD"
fi
if $(bambu_results /application/backend@discrepancy) ; then
  VFLAGS+=" -define GENERATE_VCD_DISCREPANCY"
fi
VFLAGS+=" -define __BAMBU_SIM__"

for inc in $(bambu_results /application/outputs/include)
do
  VFLAGS+=" -i ${inc}"
done

PRJ_FILE="${SWD}/bambuhls.prj"
echo "" > "${PRJ_FILE}"
for src in $(bambu_results /application/outputs/file) $(bambu_results /application/outputs/testbench);
do
   src="${BAMBU_HLS_OUTDIR}/${src}"
   case ${src} in
      *.v | *.sv) echo "SV work ${src}" >> "${PRJ_FILE}" ;;
      *.vhd | *.vhdl) echo "VHDL work ${src}" >> "${PRJ_FILE}" ;;
      *) echo "Unknown source file type: ${src}" 1>&2; exit -1 ;;
   esac
done

BAMBU_IPC_SIM_CMD="rm -rf xsim.* xelab.*;"
BAMBU_IPC_SIM_CMD+=" xelab -sv_root ${SWD} -sv_lib libmdpi ${VFLAGS} -prj ${PRJ_FILE}"
if $(bambu_results /application/backend@assert) ; then
  BAMBU_IPC_SIM_CMD+=" --debug all --rangecheck -O2"
else
  BAMBU_IPC_SIM_CMD+=" --debug off -O3"
fi
BAMBU_IPC_SIM_CMD+=" -L work -L unifast_ver -L unisims_ver -L unimacro_ver -L secureip"
BAMBU_IPC_SIM_CMD+=" --snapshot bambuhlstb_behav work.clocked_bambu_testbench -nolog -stat -R"
BAMBU_IPC_SIM_CMD+=" 2>&1 | tee ${SWD}/simulation.log; exit \${PIPESTATUS[0]};"
