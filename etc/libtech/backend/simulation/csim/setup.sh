#!/usr/bin/env bash

CSIMFLAGS=" -DBAMBU_CSIM"

case "$(bambu_results /application/sources@cflags)" in
  *-m32*) CSIMFLAGS+=" -m32" ;;
  *-mx32*) CSIMFLAGS+=" -mx32" ;;
  *) CSIMFLAGS+=" -m64" ;;
esac

CSIMFLAGS+=" -isystem ${BAMBU_HLS}/include/panda"
CSIMFLAGS+=" -fno-unwind-tables -fno-stack-protector -fomit-frame-pointer -w -fno-builtin -fno-strict-aliasing"

# mdpi.c provides the HDL co-simulation handlers (m_read/m_write/m_state);
# mdpi_csim.c provides the C-simulation ones. Linking both gives duplicate
# symbols, so CSIM must use only the csim variant.
CSIM_SRCS=("${BAMBU_HLS}/share/panda/libmdpi/mdpi_csim.c" "${BAMBU_HLS}/share/panda/libmdpi/mdpi_pp.c")

for src in $(bambu_results /application/outputs/file) $(bambu_results /application/outputs/testbench);
do
   src="${BAMBU_HLS_OUTDIR}/${src}"
   case ${src} in
      *.v | *.sv) ;;
      *.vhd | *.vhdl) ;;
      *.c | *.cpp) CSIM_SRCS+=("${src}") ;;
      *) echo "Unknown source file type: ${src}" 1>&2; exit -1 ;;
   esac
done


# CC is the compiler recorded for the design sources, so it is often a C++ driver
# (e.g. clang++-16). A C++ driver compiles ".c" files as C++, which breaks the
# generated C testbench (void* assignments in the mdpi interface macros). Select
# the language per file extension instead of trusting the driver name.
CSIM_OBJS=()
obj_idx=0
for src in "${CSIM_SRCS[@]}";
do
   case ${src} in
      *.c) src_lang="c" ;;
      *) src_lang="c++" ;;
   esac
   ${CC} -pipe ${CSIMFLAGS} -x "${src_lang}" -c "${src}" -o "${SWD}/csim_obj_${obj_idx}.o" || exit -1
   CSIM_OBJS+=("${SWD}/csim_obj_${obj_idx}.o")
   obj_idx=$((obj_idx + 1))
done

${CC} -pipe ${CSIMFLAGS} -o "${SWD}/bambu_csim" "${CSIM_OBJS[@]}" || exit -1


BAMBU_IPC_SIM_CMD="run_logged \"${SWD}/simulation.log\" \"${SWD}/bambu_csim\""
