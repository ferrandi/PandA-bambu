#!python

import argparse
import os
import stat
import re

from array import *
from collections import OrderedDict
import xml.etree.ElementTree as ET

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

tests = {"": 0}
tests.clear()
testsuites = ET.parse(in_file).getroot()
print('Testsuites containing {} failed tests'.format(testsuites.attrib['failures']))
for testsuite in testsuites:
    for testcase in testsuite:
        if any(child.tag == 'failure' for child in testcase):
            test_line = testcase.attrib['name']
            if len(old_path):
                test_line = test_line.replace(old_path, root_path)
            tests[test_line] = tests.get(test_line, 0) + 1

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
script_dir="$(dirname $(readlink -e $0))"
out_name="TEST_NAME"
python ${script_dir}/../etc/scripts/test_panda.py --tool=bambu \\
    -l${out_name}_list -o output_${out_name} -b$script_dir --name="${out_name}" \\
    --timeout=120m --junitdir=test-report_${out_name} "$@"
'''
    test_runner = test_runner.replace("TEST_NAME", out_file)
    runner_script = out_file + "_test-runner.sh"
    t = open(runner_script, "w")
    t.write(test_runner)
    t.close()
    st = os.stat(runner_script)
    os.chmod(runner_script, st.st_mode | stat.S_IEXEC)
