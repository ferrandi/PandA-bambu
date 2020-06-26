#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH
rm -rf VGA_Nexys4_synth
mkdir -p VGA_Nexys4_synth
cd VGA_Nexys4_synth
timeout 2h bambu -O3 --print-dot -v4 --device-name=xc7a100t-1csg324-VVD $root_dir/vgatest.c --backend-sdc-extensions=$root_dir/Nexys4_Master.sdc --clock-period=10 $root_dir/constraints_STD.xml $root_dir/IPs.xml --C-no-parse=$root_dir/plot.c,$root_dir/leds_ctrl.c,$root_dir/btn_ctrl.c,$root_dir/sw_ctrl.c,$root_dir/delay.c --file-input-data=$root_dir/plot.v,$root_dir/leds_ctrl.v,$root_dir/btn_ctrl.v,$root_dir/sw_ctrl.v,$root_dir/delay.v --evaluation=PERIOD,AREA,FREQUENCY,CLOCK_SLACK,REGISTERS,DSPS,BRAMS -fwhole-program --speculative-sdc-scheduling
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

