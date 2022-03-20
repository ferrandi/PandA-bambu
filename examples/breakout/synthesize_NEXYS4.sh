#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"

rm -rf breakout_synth
mkdir -p breakout_synth
cd breakout_synth
timeout 2h bambu $script_dir/main.c --top-fname=main \
   --compiler=I386_CLANG11 -Os --device-name=xc7a100t-1csg324-VVD --clock-period=10 \
   --backend-sdc-extensions=$script_dir/Nexys4_Master.sdc $script_dir/constraints_STD.xml $script_dir/IPs.xml \
   --C-no-parse=$script_dir/plot.c,$script_dir/sevensegments_ctrl.c,$script_dir/btn_ctrl.c,$script_dir/get_ticks.c \
   --file-input-data=$script_dir/plot.v,$script_dir/sevensegments_ctrl.v,$script_dir/btn_ctrl.v,$script_dir/get_ticks.v \
   --evaluation=PERIOD,AREA,FREQUENCY,CLOCK_SLACK,REGISTERS,DSPS,BRAMS --speculative-sdc-scheduling --print-dot "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
