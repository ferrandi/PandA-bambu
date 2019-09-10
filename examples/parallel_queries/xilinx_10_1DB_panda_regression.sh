#!/bin/bash
abs_script=`readlink -e $0`
dir_script=`dirname $abs_script`
COMMONARGS="--compiler=I386_GCC49 --std=c99 --experimental-set=BAMBU -O3 -v3 -fno-delete-null-pointer-checks -fopenmp --pragma-parse --mem-delay-read=20 --mem-delay-write=20 --channels-type=MEM_ACC_11 --memory-allocation-policy=NO_BRAM --no-iob --device-name=xc7vx690t-3ffg1930-VVD  --top-fname=test --top-rtldesign-name=search --clock-period=10 -DMAX_VERTEX_NUMBER=26455 -DMAX_EDGE_NUMBER=100573"
VHDL=no
for arg in "$@"
do
   if test "$arg" = "-c=-wH"; then
      VHDL=yes
   fi
done
if test "$VHDL" = yes; then
   $dir_script/xilinx_10_1DB.sh $@
   return_value=$?
   if test $return_value != 0; then
      exit $return_value
   fi
   NAME=`basename $0 .sh`
   $dir_script/../../../../etc/scripts/test_panda.py --spider-style="../lib/latex_format_bambu_results_xilinx.xml" --tool=bambu -l`dirname $0`/list_1DB -ooutput_$NAME --commonargs="$COMMONARGS --simulate" \
      --args="--configuration-name=2W-4CH -DN_THREADS=2 --num-threads=2 --memory-banks-number=4 --channels-number=4"\
      -t1440m --table=$NAME.tex --benchmarks_root=`dirname $0` --name="$NAME" $@
   return_value=$?
   if test $return_value != 0; then
      exit $return_value
   fi
else
   $dir_script/xilinx_10_1DB_simulate.sh $@
   return_value=$?
   if test $return_value != 0; then
      exit $return_value
   fi
fi
exit 0

