#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--simulator=MODELSIM --evaluation=AREA" \
   -ltaste_list -o output_taste -b$script_dir --table=TASTEoutput.tex --name="TASTE" "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
