#!/bin/bash
# Offline checks of the shared simulation logging wrapper: they exercise the
# logging of the simulator and of the system testbench. The checks compile the
# record_output copier of the distribution, so they need bash, a C++ compiler
# and python3 for the check driver only: the product flow they check needs
# neither the system tee nor python3. No simulator and no Bambu invocation.
script_dir="$(dirname "$(readlink -e "$0")")"

python3 -B "$script_dir/bambu_simulation_logging_test/test_simulation_logging.py" -v
exit $?
