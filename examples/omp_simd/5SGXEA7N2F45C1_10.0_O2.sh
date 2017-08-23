
ARGS="--compiler=I386_GCC49 --device-name=5SGXEA7N2F45C1 --evaluation -fwhole-program -fno-delete-null-pointer-checks --clock-period=10 --experimental-setup=BAMBU-BALANCED-MP --no-iob"
script=`readlink -e $0`
root_dir=`dirname $script`
NAME=`basename $0 .sh`
DIRNAME=${root_dir##*/}
$root_dir/../../etc/scripts/test_panda.py --spider-style="../lib/latex_format_bambu_results_xilinx.xml" --tool=bambu -l$root_dir/list --args="$ARGS" -t120m --benchmarks_root=$root_dir -ooutput_${DIRNAME}_$NAME --name=${DIRNAME}_$NAME --table=${DIRNAME}_$NAME.tex $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0

