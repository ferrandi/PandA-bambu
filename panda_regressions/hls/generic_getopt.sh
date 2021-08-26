#!/bin/bash

showHelp() {
cat << EOF  
Usage: -c=--compiler=<compiler_id> [test_panda.py args]

   --compiler  Select frontend compiler for bambu

EOF
}

COMPILER=""
REPORT_DIR=""
ARGS=$@

for arg in $ARGS
do
    if [[ "$arg" = -c=--compiler=* ]]; then
        COMPILER="$(sed 's/-c=--compiler=I386_//g' <<<$arg)"
    elif [[ "$arg" = --junitdir=* ]]; then
        REPORT_DIR="$(sed 's/--junitdir=//g' <<<$arg)/"
    fi
done

if [[ -z "$COMPILER" && "$ggo_require_compiler" == "1" ]]; then
    showHelp
    exit -1
fi
