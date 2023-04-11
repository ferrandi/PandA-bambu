#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu \
             --args="--simulate -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --device-name=xc7z020,-1,clg484,VVD --experimental-setup=BAMBU --configuration-name=BAMBU"\
             --args="--simulate -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --device-name=xc7z020,-1,clg484,VVD --experimental-setup=BAMBU-AREA --configuration-name=BAMBU-AREA"\
             --args="--simulate -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --device-name=xc7z020,-1,clg484,VVD --experimental-setup=BAMBU-AREA-MP --configuration-name=BAMBU-AREA-MP"\
             --args="--simulate -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --device-name=xc7z020,-1,clg484,VVD --experimental-setup=BAMBU-BALANCED --configuration-name=BALANCED"\
             --args="--simulate -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --device-name=xc7z020,-1,clg484,VVD --experimental-setup=BAMBU-BALANCED-MP --configuration-name=BAMBU-BALANCED-MP"\
             --args="--simulate -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --device-name=xc7z020,-1,clg484,VVD --experimental-setup=BAMBU-PERFORMANCE-MP --configuration-name=BAMBU-PERFORMANCE-MP"\
             --args="--simulate -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --device-name=xc7z020,-1,clg484,VVD --experimental-setup=BAMBU-PERFORMANCE --configuration-name=BAMBU-PERFORMANCE"\
             --args="--simulate -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --device-name=xc7z020,-1,clg484,VVD --experimental-setup=BAMBU092 --configuration-name=BAMBU092"\
             -lCHStone_experimental_setups_list -o output_CHStoneExperimentalSetups -b$script_dir --table=CHStoneExperimentalSetups.tex --name="CHStoneExperimentalSetups" "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
