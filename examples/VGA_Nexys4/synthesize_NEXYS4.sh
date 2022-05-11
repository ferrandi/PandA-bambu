#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
rm -rf VGA_Nexys4_synth
mkdir -p VGA_Nexys4_synth
cd VGA_Nexys4_synth
timeout 2h bambu $root_dir/vgatest.c $root_dir/constraints_STD.xml $root_dir/IPs.xml \
   --backend-sdc-extensions=$root_dir/Nexys4_Master.sdc --speculative-sdc-scheduling \
   -O3 --device-name=xc7a100t-1csg324-VVD --clock-period=10 \
   --C-no-parse=$root_dir/plot.c,$root_dir/leds_ctrl.c,$root_dir/btn_ctrl.c,$root_dir/sw_ctrl.c,$root_dir/delay.c \
   --file-input-data=$root_dir/plot.v,$root_dir/leds_ctrl.v,$root_dir/btn_ctrl.v,$root_dir/sw_ctrl.v,$root_dir/delay.v \
   --evaluation=PERIOD,AREA,FREQUENCY,CLOCK_SLACK,REGISTERS,DSPS,BRAMS --simulator=MODELSIM --print-dot "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
