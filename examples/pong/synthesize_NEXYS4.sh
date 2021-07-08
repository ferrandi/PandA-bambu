#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
rm -rf pong_synth
mkdir -p pong_synth
cd pong_synth
timeout 2h bambu --compiler=I386_CLANG11 -Os --print-dot --device-name=xc7a100t-1csg324-VVD $root_dir/main.c  --backend-sdc-extensions=$root_dir/Nexys4_Master.sdc --clock-period=10 $root_dir/constraints_STD.xml $root_dir/IPs.xml --C-no-parse=$root_dir/plot.c,$root_dir/sevensegments_ctrl.c,$root_dir/btn_ctrl.c,$root_dir/get_ticks.c --file-input-data=$root_dir/plot.v,$root_dir/sevensegments_ctrl.v,$root_dir/btn_ctrl.v,$root_dir/get_ticks.v --evaluation=PERIOD,AREA,FREQUENCY,CLOCK_SLACK,REGISTERS,DSPS,BRAMS --top-fname=main --speculative-sdc-scheduling "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

