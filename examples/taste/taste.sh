#!/bin/bash
`dirname $0`/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--simulator=MODELSIM --evaluation=AREA" \
   -ltaste_list -o output_taste -b`dirname $0` --table=TASTEoutput.tex --name="TASTE" "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
