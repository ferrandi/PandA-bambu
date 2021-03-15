#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH
mkdir -p VGA_synth
cd VGA_synth
timeout 2h bambu -O3 --print-dot --evaluation=AREA,FREQUENCY,CLOCK_SLACK,REGISTERS,DSPS,BRAMS -v4 --target-file=$root_dir/DE1-characterization-file.xml $root_dir/delay.c $root_dir/vgatest.c --backend-script-extensions=$root_dir/DE1_pin_assignments.qsf --backend-sdc-extensions=$root_dir/DE1_design.sdc --clock-period=20 $root_dir/constraints_STD.xml $root_dir/PLOT_IPs.xml  --C-no-parse=$root_dir/plot.c,$root_dir/leds_ctrl.c --file-input-data=$root_dir/leds_ctrl.v,$root_dir/plot.v 2>&1 | tee log.txt 
/opt/altera/13.0sp1/quartus/bin/quartus_pgm -c USB-Blaster -m jtag -o "p;main_minimal_interface.sof"

