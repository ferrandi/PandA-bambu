#!/usr/bin/python

import argparse
import sys
import os
import stat
import re

from array import *
from collections import OrderedDict

parser = argparse.ArgumentParser(
    description="Aggregate failed test from test_panda.py log", fromfile_prefix_chars="@")
parser.add_argument('-l', "--log", help="The path of log file", required=True)
parser.add_argument(
    '-o', "--output", help="The output file to fill with aggregated failed tests", default="aggregated_failures")
parser.add_argument(
    "--testrunner", help="Require to setup a test runner for aggregated failed tests", action='store_true')
parser.add_argument(
    "--replace", help="Comma separated old,new paths to be replaced in test path", default=",")

args = parser.parse_args()
in_file = args.log
out_file = args.output
test_required = args.testrunner
old_path = args.replace.split(',')[0].rstrip('/')
root_path = args.replace.split(',')[1].rstrip('/')

print('Out file is {}'.format(out_file))
print('In file is {}'.format(in_file))
if len(old_path) != 0:
    if len(root_path) == 0:
        parser.error("Argument --replace not correctly set")
    print('Old path to be replaced is {}'.format(old_path))
    print('New root directory for tests is {}'.format(root_path))

t = open(in_file, "r")
lines = t.readlines()
t.close()
tests = {"": 0}
tests.clear()
t = open(out_file, "w")
for line in lines:
    match = re.search(r"FAILURE[^\/]*(.*)", line)
    if match:
        t.write(line)
        test_line = ' '.join(OrderedDict.fromkeys(match.group(1).split()))
        if len(old_path) != 0:
            test_line = test_line.replace(old_path, root_path)
        tests[test_line] = tests.get(test_line, 0) + 1

t.close()

if test_required:
    unnamed_counter = 0
    t = open(out_file + "_list", "w")
    for test in tests.keys():
        if len(test) == 0:
            continue
        bench_suffix = ''
        conf_name = ' --configuration-name='
        compiler = re.search(
            r"--compiler=I386_(\S*)(?!.*--compiler=)", test)
        if compiler:
            bench_suffix += '_' + compiler.group(compiler.lastindex)
            conf_name += compiler.group(compiler.lastindex)
        else:
            conf_name += 'GCC49'
        if re.search(r"-wH", test):
            bench_suffix += '_w'
        if re.search(r"--channels-type=MEM_ACC_NN", test):
            bench_suffix += '_NN'
        if re.search(r"--do-not-expose-globals", test):
            bench_suffix += '_dneg'
        if re.search(r"--speculative-sdc-scheduling", test):
            bench_suffix += '_sdc'
        period = re.search(r"--clock-period=([0-9]*)", test)
        if period:
            bench_suffix += '_c' + period.group(1)
        device = re.search(r"--device\S*=([^T]\S*)", test)
        if device:
            bench_suffix += '_' + device.group(1).replace(',', '')
        if re.search(r"--configuration-name", test):
            conf_name = ''
        else:
            opt = re.search(r"\s-(O[0-9])\s", test)
            if opt:
                conf_name += '_' + opt.group(1)

        test_line = ""
        if re.search(r"--benchmark-name", test):
            test_line = re.sub(r"--benchmark-name=(\S*)",
                               r"--benchmark-name=\1" + bench_suffix, test) + conf_name + "\n"
        else:
            unnamed_counter += 1
            test_line = test + ' --benchmark-name=' + \
                re.search(r"\/[^\s,]+\/]*([^\s,]+\.[^\s,]+)", test).group(1) + \
                bench_suffix + '_' + str(unnamed_counter) + conf_name + "\n"

        if re.search(r"^(?!.*" + root_path.replace('/', '\\/') + ").*", test_line) == None:
            t.write(test_line)
    t.close()
    test_runner = '''#!/bin/bash
$(dirname $0)/../etc/scripts/test_panda.py --tool=bambu -lTEST_NAME_list -o output_TEST_NAME -b$(dirname $0) --name="TEST_NAME" --timeout=120m --junitdir=test-report_TEST_NAME $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
'''
    test_runner = test_runner.replace("TEST_NAME", out_file)
    runner_script = out_file + "_test-runner.sh"
    t = open(runner_script, "w")
    t.write(test_runner)
    t.close()
    st = os.stat(runner_script)
    os.chmod(runner_script, st.st_mode | stat.S_IEXEC)
