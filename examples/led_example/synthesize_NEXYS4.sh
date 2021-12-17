#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
rm -rf led_example_synth
mkdir -p led_example_synth
cd led_example_synth
timeout 2h bambu $root_dir/led_example.c $root_dir/constraints_STD.xml $root_dir/IPs.xml --top-fname=led_example \
   --backend-sdc-extensions=$root_dir/Nexys4_Master.sdc -O3 --device-name=xc7a100t-1csg324-VVD --clock-period=10 \
   --C-no-parse=$root_dir/sw_ctrl.c,$root_dir/leds_ctrl.c,$root_dir/btn_ctrl.c,$root_dir/sevensegments_ctrl.c \
   --file-input-data=$root_dir/leds_ctrl.v,$root_dir/sw_ctrl.v,$root_dir/btn_ctrl.v,$root_dir/sevensegments_ctrl.v \
   --simulator=MODELSIM --evaluation=PERIOD,AREA,FREQUENCY,CLOCK_SLACK,REGISTERS,DSPS,BRAMS --print-dot "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

