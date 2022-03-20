#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
mkdir -p VGA_synth
cd VGA_synth
timeout 2h bambu $root_dir/vgatest.c $root_dir/delay.c $root_dir/constraints_STD.xml $root_dir/PLOT_IPs.xml \
   --target-file=$root_dir/DE1-characterization-file.xml \
   --backend-script-extensions=$root_dir/DE1_pin_assignments.qsf --backend-sdc-extensions=$root_dir/DE1_design.sdc \
   --clock-period=20 -O3 \
   --C-no-parse=$root_dir/plot.c,$root_dir/leds_ctrl.c -DC_SIMULATION \
   --file-input-data=$root_dir/leds_ctrl.v,$root_dir/plot.v \
   --simulator=MODELSIM --print-dot --evaluation "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
