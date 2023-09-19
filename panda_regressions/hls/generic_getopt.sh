#!/bin/bash
export LC_ALL=$(locale -a|grep -ix 'c.utf-\?8' || echo C)

showHelp() {
    use_string="Usage:"
    legend=$'\n'
    if [[ "$ggo_require_compiler" == "1" ]]; then
        use_string+=" -c=--compiler=<compiler>"
        legend+=$'   --compiler     Select frontend compiler for bambu\n'
    fi
    if [[ "$ggo_require_device" == "1" ]]; then
        use_string+=" -c=--device-name=<device>"
        legend+=$'   --device-name  Select target device\n'
    fi
    if [[ "$ggo_require_period" == "1" ]]; then
        use_string+=" -c=--clock-period=<period>"
        legend+=$'   --clock-period Select target clock period\n'
    fi
python3     use_string+=" [test_panda.py args]"
    
    echo $use_string
    echo $legend
}

compiler=""
device=""
period=""
ARGS="$@"

for arg in $ARGS
do
    if [[ "$arg" = -c=--compiler=* ]]; then
        compiler="$(sed 's/-c=--compiler=I386_//g' <<<$arg)"
    elif [[ "$arg" = -c=--device-name=* ]]; then
        device="$(sed 's/-c=--device-name=//g' <<<$arg)"
    elif [[ "$arg" = -c=--clock-period=* ]]; then
        period="$(sed 's/-c=--clock-period=//g' <<<$arg)"
    fi
done

if [[ -z "$compiler" && "$ggo_require_compiler" == "1" ]]; then
    showHelp
    exit -1
fi
if [[ -z "$device" && "$ggo_require_device" == "1" ]]; then
    showHelp
    exit -1
fi
if [[ -z "$period" && "$ggo_require_period" == "1" ]]; then
    showHelp
    exit -1
fi
