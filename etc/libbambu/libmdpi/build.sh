#!/bin/bash
#####################################################################
# The script requires the following parameters:
#     - linker command line containing user-provided testbench 
#       object file, top function wrapper object file and output 
#       flag for the shared object file (any additional options too)
#     - CFLAGS environment variable with include path to svdpi.h, 
#       -m flag, and -D<ACTIVE_SIM> (additional flags will be discarded)
# i.e.: build.sh user_tb.o wrapper.o -o libtb.so
# Optional arguments:
#     - CC environment variable is used to specify the compiler
#
# The script generates a shared object containing the 
# DPI-C imports necessary for the Verilog co-simulation
#
#####################################################################
script_dir="$(dirname $(readlink -e $0))"
args="$@"

if [ -z "$CC" ]; then
   echo "Compiler not provided, fallback to gcc"
   CC="gcc"
fi

# Retrieve -m option if present
m_option="$(grep -oE '(-mx?[0-9]+)' <<< ${CFLAGS})"
if [ -z "${m_option}" ]; then
   echo "-m option not provided, default will be used $($CC -dumpmachine)"
fi
includes="$(grep -oE '((-I|-isystem) ?[^ ]+)' <<< ${CFLAGS} | tr '\n' ' ')"
defines="$(grep -oE '(-D(\\.|[^ ])+)' <<< ${CFLAGS} | tr '\n' ' ')"
CFLAGS="${m_option} -fPIC -funroll-loops -O2 ${includes} ${defines}"
# Forward LD_LIBRARY_PATH overrides
IFS=':' read -r -a LD_LIBS <<< "${LD_LIBRARY_PATH}"
LDFLAGS="-lpthread -lstdc++ -lm ${LD_LIBS[@]/#/-L}"
mdpi_src="${script_dir}/mdpi.cpp"
build_dir="$(dirname $(echo "$@" | sed -E 's/.*-o ([^ ]+).*/\1/'))"

if [ "$CC" = "xsc" ]; then
   IFS=' ' read -r -a LDFLAGS <<< "${LDFLAGS}"
   LDFLAGS="-shared -work work ${LDFLAGS[@]/#/-gcc_link_options=}"
   objs=($(echo "$@" | grep -oE "[^ ]+\.o"))
   for obj in "${objs[@]}"
   do
      args="${args/$obj}"
   done
   objs=( "${objs[@]/#/-i }" )
   $CC -c -work work -i ${mdpi_src} ${CFLAGS}
   $CC ${objs[*]} ${args} ${LDFLAGS}
else
   LDFLAGS="${m_option} -shared -fPIC -Bsymbolic -Wl,-z,defs ${LDFLAGS}"
   mdpi_obj="${build_dir}/$(basename ${mdpi_src//.cpp/.o})"
   $CC -c -o ${mdpi_obj} ${mdpi_src} ${CFLAGS}
   $CC ${args} ${mdpi_obj} ${LDFLAGS}
fi
