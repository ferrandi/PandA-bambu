#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH
rm -rf led_example_synth
mkdir -p led_example_synth
cd led_example_synth
timeout 2h bambu -O3 --print-dot -v4 --device-name=xc7a100t-1csg324-VVD $root_dir/led_example.c --backend-sdc-extensions=$root_dir/Nexys4_Master.sdc --clock-period=10 $root_dir/constraints_STD.xml $root_dir/IPs.xml --C-no-parse=$root_dir/sw_ctrl.c,$root_dir/leds_ctrl.c,$root_dir/btn_ctrl.c,$root_dir/sevensegments_ctrl.c --file-input-data=$root_dir/leds_ctrl.v,$root_dir/sw_ctrl.v,$root_dir/btn_ctrl.v,$root_dir/sevensegments_ctrl.v --evaluation=PERIOD,AREA,FREQUENCY,CLOCK_SLACK,REGISTERS,DSPS,BRAMS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

