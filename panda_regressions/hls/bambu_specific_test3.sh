#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=O3-none --hls-div=none -O3 -lm --evaluation=TOTAL_CYCLES,CYCLES --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC49" \
   --args="--configuration-name=O3-as   --hls-div=as   -O3 -lm --evaluation=TOTAL_CYCLES,CYCLES --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC49" \
   --args="--configuration-name=O3-nr1  --hls-div=nr1  -O3 -lm --evaluation=TOTAL_CYCLES,CYCLES --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC49" \
   --args="--configuration-name=O3-nr2  --hls-div=nr2  -O3 -lm --evaluation=TOTAL_CYCLES,CYCLES --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC49" \
   --args="--configuration-name=O3-NR   --hls-div=NR   -O3 -lm --evaluation=TOTAL_CYCLES,CYCLES --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC49" \
   -lbambu_specific_test3_list -o output_BST3 -b$(dirname $0) --table=BST3output.tex --name="BambuSpecificTest3" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
