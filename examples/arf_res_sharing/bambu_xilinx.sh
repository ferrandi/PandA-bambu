#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

$root_dir/constrained_synth_xilinx_zynq_vvd.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/unconstrained_synth_xilinx_zynq_vvd.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/constrained_synth_xilinx_vc707_vvd.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/unconstrained_synth_xilinx_vc707_vvd.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/constrained_synth_xilinx_vx690t_vvd.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/unconstrained_synth_xilinx_vx690t_vvd.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/constrained_synth_xilinx_artix7_vvd.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/unconstrained_synth_xilinx_artix7_vvd.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/constrained_synth_xilinx_zynq_ise.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/unconstrained_synth_xilinx_zynq_ise.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/constrained_synth_xilinx_vx7330_ise.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/unconstrained_synth_xilinx_vx7330_ise.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/constrained_synth_xilinx_v6_ise.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/unconstrained_synth_xilinx_v6_ise.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/constrained_synth_xilinx_v550_ise.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/unconstrained_synth_xilinx_v550_ise.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/constrained_synth_xilinx_v5330_ise.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/unconstrained_synth_xilinx_v5330_ise.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/constrained_synth_xilinx_v5110_ise.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/unconstrained_synth_xilinx_v5110_ise.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi


