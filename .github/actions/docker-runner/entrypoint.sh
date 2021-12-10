#!/bin/bash
set -e

working_dir=$1
script=$2
shift
shift

cd $working_dir

./$script $@
