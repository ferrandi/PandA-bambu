#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"

techlibs="$(find "${script_dir}" -name 'C_*_IPs.xml' | paste -sd,)"

python3 $script_dir/../../etc/scripts/characterize.py \
   --technology-files="${techlibs}" "$@"
