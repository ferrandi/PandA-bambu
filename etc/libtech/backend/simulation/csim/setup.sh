#!/usr/bin/env bash

CSIMFLAGS=" -DBAMBU_CSIM"

case "$(bambu_results /application/sources@cflags)" in
  *-m32*) CSIMFLAGS+=" -m32" ;;
  *-mx32*) CSIMFLAGS+=" -mx32" ;;
  *) CSIMFLAGS+=" -m64" ;;
esac

CSIMFLAGS+=" -fno-unwind-tables -fno-stack-protector -fomit-frame-pointer -w -fno-builtin -fno-strict-aliasing"

CSIM_SRCS=("${BAMBU_HLS}/share/panda/libmdpi/mdpi.c" "../mdpi_csim.c" "../mdpi_pp.c")

for src in $(bambu_results /application/outputs/file) $(bambu_results /application/outputs/testbench);
do
   src="${BAMBU_HLS_OUTDIR}/${src}"
   case ${src} in
      *.v | *.sv) ;
      *.vhd | *.vhdl) ;;
      *.c | *.cpp) CSIM_SRCS+=("${src}") ;;
      *) echo "Unknown source file type: ${src}" 1>&2; exit -1 ;;
   esac
done


${CC} -pipe ${CSIMFLAGS} -o bambu_csim ${CSIM_SRCS[@]}


BAMBU_IPC_SIM_CMD="${SWD}/bambu_csim 2>&1 | tee ${SWD}/simulation.log; exit \${PIPESTATUS[0]};"
