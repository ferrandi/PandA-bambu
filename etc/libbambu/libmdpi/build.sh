#!/bin/bash
#####################################################################
# The script requires the following parameters:
#     - linker command line containing user-provided testbench 
#       object file, top function wrapper object file and output 
#       flag for the shared object file (any additional options too)
#     - CFLAGS environment variable with include path to svdpi.h, 
#       -m flag, and -D<ACTIVE_SIM>
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

if [ -z "$CFLAGS" ]; then
   echo "CFLAGS not provided"
   exit 1
fi

if [ -z "$CC" ]; then
   echo "Compiler not provided, fallback to gcc"
   CC="gcc"
fi

if [ -z "$LD" ]; then
   echo "Linker not provided, fallback to compiler $CC"
   LD="$CC"
fi

# Retrieve -m option if present
m_option="$(grep -oE '(-m[0-9]+)' <<< ${CFLAGS})"
if [ -z "${m_option}" ]; then
   echo "-m option not provided, default will be used $($CC -dumpmachine)"
fi
CFLAGS="$(sed -E "s/--std=[^[:space:]]+[[:space:]]*//g" <<< ${CFLAGS})"
CFLAGS+=" -fPIC -fexceptions -fnon-call-exceptions -O2"
case "$CC" in
   */gcc*) ;&
   */xsc*)
   unsupported_flags=( "-fexcess-precision=standard" )
   echo "Removing unsupported C flags: ${unsupported_flags[*]}"
   for flag in ${unsupported_flags[@]}
   do
      CFLAGS="${CFLAGS/$flag}"
   done
   ;;
   *) ;;
esac
if [ "$CC" = "xsc" ]; then
   echo "Xilinx XCS compiler detected"
   CFLAGS="${CFLAGS/(/\\(}"
   CFLAGS="${CFLAGS/)/\\)}"
   IFS=' ' read -r -a CFLAGS <<< "${CFLAGS}"
   CFLAGS="${CFLAGS[@]/#/-gcc_compile_options=}"
fi

objs=($(echo "$@" | grep -oE "[^ ]+\.o"))
for obj in "${objs[@]}"
do
   echo "Redefine symbols in '$obj'"
   objcopy --redefine-sym main=m_cosim_main --redefine-sym exit=__m_exit  --redefine-sym abort=__m_abort $obj
   args="${args/$obj}"
done

srcs=(
   "${script_dir}/mdpi_cosim.cpp"
   "${script_dir}/mdpi_user.cpp"
   "${script_dir}/mdpi_wrapper.cpp"
   "${script_dir}/mdpi.cpp"
   "${script_dir}/segvcatch/segvcatch.cpp"
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

LDFLAGS="$(grep -oE '( -l|-W,l)\w+' <<< ${CFLAGS}) -lm"
if [ "$LD" = "xsc" ]; then
   IFS=' ' read -r -a LDFLAGS <<< "${LDFLAGS}"
   LDFLAGS="${LDFLAGS[@]/#/-gcc_link_options=}"
   LDFLAGS+=" -shared -work work -gcc_link_options=-lpthread -gcc_link_options=-lstdc++"
   objs=( "${objs[@]/#/-i }" )
else
   LDFLAGS+=" ${m_option} -shared -fPIC -Bsymbolic -Wl,-z,defs -lpthread -lstdc++"
fi

$LD ${objs[*]} ${LDFLAGS} ${args}
rm -rf ${build_dir}
