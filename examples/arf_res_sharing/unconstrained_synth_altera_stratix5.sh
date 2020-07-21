#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH
#!/bin/bash
mkdir -p unconstrained_synth_altera_stratix5
cd unconstrained_synth_altera_stratix5
echo "# Quartus II synthesis and ICARUS simulation"
timeout 2h bambu -v4 $root_dir/module.c --generate-tb=$root_dir/test.xml --simulator=ICARUS --device-name=5SGXEA7N2F45C1 --evaluation --experimental-setup=BAMBU --generate-interface=WB4 --cprf=0.9 --skip-pipe-parameter=1
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..



