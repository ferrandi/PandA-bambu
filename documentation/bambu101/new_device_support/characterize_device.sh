#!/bin/bash
abs_script=$(readlink -e $0)
root_dir=$(dirname $abs_script)/../..
python3 $root_dir/etc/scripts/characterize.py --technology-files=$root_dir/etc/libtech/C_HLS_IPs.xml,$root_dir/etc/libtech/C_MEM_IPs.xml,$root_dir/etc/libtech/C_PROFILING_IPs.xml,$root_dir/etc/libtech/C_VEC_IPs.xml,$root_dir/etc/libtech/C_FP_IPs.xml,$root_dir/etc/libtech/C_STD_IPs.xml "$@"


