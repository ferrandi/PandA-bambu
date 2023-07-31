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

if [ -z "$LD" ]; then
   echo "Linker not provided, fallback to compiler $CC"
   LD="$CC"
fi

# Retrieve -m option if present
m_option="$(grep -oE '(-mx?[0-9]+)' <<< ${CFLAGS})"
if [ -z "${m_option}" ]; then
   echo "-m option not provided, default will be used $($CC -dumpmachine)"
fi
includes="$(grep -oE '((-I|-isystem) ?[^ ]+)' <<< ${CFLAGS} | tr '\n' ' ')"
defines="$(grep -oE '(-D(\\.|[^ ])+)' <<< ${CFLAGS} | tr '\n' ' ')"
CFLAGS="${m_option} -fPIC -funroll-loops -O2 ${includes} ${defines}"

objs=($(echo "$@" | grep -oE "[^ ]+\.o"))
for obj in "${objs[@]}"
do
   echo "Redefine symbols in '$obj'"
   objcopy --redefine-sym main=m_cosim_main --redefine-sym exit=__m_exit --redefine-sym abort=__m_abort --redefine-sym __assert_fail=__m_assert_fail $obj
   args="${args/$obj}"
done

srcs=(
   "${script_dir}/mdpi_cosim.cpp"
   "${script_dir}/mdpi_user.cpp"
   "${script_dir}/mdpi_wrapper.cpp"
   "${script_dir}/mdpi.cpp"
   )
build_dir="$(mktemp -d /tmp/build_XXXX)"
for file in "${srcs[@]}"
do
   if [ "$CC" = "xsc" ]; then
      $CC -c -work work -i $file ${CFLAGS}
   else
      fileo="${build_dir}/$(basename ${file//.cpp/.o})"
      $CC -c -o ${fileo} $file ${CFLAGS}
      objs+=("${fileo}")
   fi
done

LDFLAGS="-lpthread -lstdc++ -lm"
if [ "$LD" = "xsc" ]; then
   IFS=' ' read -r -a LDFLAGS <<< "${LDFLAGS}"
   LDFLAGS="-shared -work work ${LDFLAGS[@]/#/-gcc_link_options=}"
   objs=( "${objs[@]/#/-i }" )
else
   LDFLAGS="${m_option} -shared -fPIC -Bsymbolic -Wl,-z,defs ${LDFLAGS}"
fi

$LD ${objs[*]} ${args} ${LDFLAGS}
rm -rf ${build_dir}
