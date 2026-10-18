#!/bin/bash
# Offline checks of the shared simulation logging wrapper: they exercise the
# logging of the simulator and of the system testbench, so they need bash and
# python3 but no simulator and no Bambu invocation.
script_dir="$(dirname "$(readlink -e "$0")")"

python3 -B "$script_dir/bambu_simulation_logging_test/test_simulation_logging.py" -v
exit $?
