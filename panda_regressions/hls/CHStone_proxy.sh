#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=GCC49-O3-noproxy --simulate -O3 -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --memory-ctrl-type=D00 --experimental-setup=BAMBU --disable-function-proxy" \
   --args="--configuration-name=GCC49-O3-noinline --simulate -O3 -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --memory-ctrl-type=D00 --experimental-setup=BAMBU -fno-inline -fgcse-after-reload -fno-ipa-cp -fno-ipa-cp-clone" \
   -o output_CHStone_proxy -b$script_dir --table=CHStone_proxy.tex --name="CHStone_proxy" -lCHStone_list "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
