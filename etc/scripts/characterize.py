#!python

import argparse
import logging
import os
import shlex
import shutil
import signal
import subprocess
import sys
import threading

# The absolute path of current script
abs_script = os.path.dirname(os.path.abspath(sys.argv[0]))

# The absolute path of panda
abs_panda = os.path.abspath(os.path.join(abs_script, "../../"))


def GetChildren(parent_pid):
    # Return children of a process
    ret = set()
    ps_command = subprocess.Popen(
        "ps -o pid --ppid %d --noheaders" % parent_pid, shell=True, stdout=subprocess.PIPE)
    ps_output = ps_command.stdout.read()
    ps_command.wait()
    for pid_str in ps_output.split("\n")[:-1]:
        ret.add(int(pid_str))
    return ret


def kill_proc_tree(pid):
    # Kill a process than kill its children
    children = GetChildren(pid)
    os.kill(pid, signal.SIGKILL)
    for child in children:
        kill_proc_tree(child)


def execute_characterization(device_fu_list, thread_index):
    # Process characterations in list
    global passed_characterization
    global total_characterization
    global line_index
    global children
    global killing
    lines = open(device_fu_list).readlines()
    with lock:
        local_index = line_index
        line_index += 1
    while local_index < len(lines) and not killing:
        local_args = lines[local_index].replace('PANDA_ROOT', abs_panda)
        cwd = ComputeDirectory(local_args)
        failed_output_file_name = os.path.join(cwd, "eucalyptus_failed_output")
        if os.path.exists(failed_output_file_name):
            os.remove(failed_output_file_name)
        tool_return_value_file_name = os.path.join(
            cwd, "eucalyptus_return_value")
        if args.restart and os.path.exists(os.path.join(cwd, "eucalyptus_return_value")):
            tool_return_value_file = open(tool_return_value_file_name, "r")
            return_value = tool_return_value_file.read()
            tool_return_value_file.close()
            if return_value == "0":
                with lock:
                    total_characterization += 1
                    passed_characterization += 1
                    logging.info("   SKIPPING --- OVERALL: " + str(passed_characterization) + " passed, " + str(total_characterization-passed_characterization) +
                                 " failed, " + str(len(lines)-total_characterization) + " queued --- " + local_args.replace("\\", ""))
                    local_index = line_index
                    line_index += 1
                continue
        output_file_name = os.path.join(cwd, "eucalyptus_execution_output")
        output_file = open(output_file_name, "w")
        if local_args[0] == "\"":
            local_args = local_args[1:-1]
        local_command = "ulimit " + args.ulimit + \
            "; exec timeout --foreground " + args.timeout + " " + args.eucalyptus
        local_command = local_command + " " + local_args
        output_file.write("#" * 80 + "\n")
        output_file.write("cd " + cwd + "; ")
        output_file.write(local_command + "\n")
        output_file.write("#" * 80 + "\n")
        output_file.flush()
        return_value = -1
        with lock_creation_destruction:
            if not killing:
                children[thread_index] = subprocess.Popen(
                    local_command, stderr=output_file, stdout=output_file, cwd=cwd, shell=True, executable="/bin/bash")
        try:
            return_value = children[thread_index].wait()
        except:
            pass
        with lock_creation_destruction:
            if return_value != 0 and args.stop:
                killing = True
                for local_thread_index in range(args.j):
                    if children[local_thread_index] != None:
                        if children[local_thread_index].poll() == None:
                            try:
                                kill_proc_tree(
                                    children[local_thread_index].pid)
                            except OSError:
                                pass
        os.fsync(output_file.fileno())
        output_file.close()
        tool_return_value_file = open(tool_return_value_file_name, "w")
        tool_return_value_file.write(str(return_value))
        tool_return_value_file.close()
        args_file = open(os.path.join(cwd, "args"), "w")
        args_file.write(local_args)
        args_file.close()
        if not killing or (return_value != -9 and return_value != 0):
            if return_value != 0:
                shutil.copy(output_file_name, str(os.path.join(
                    os.path.dirname(output_file_name), "eucalyptus_failed_output")))
            with lock:
                total_characterization += 1
                if return_value == 0:
                    passed_characterization += 1
                    logging.info("   SUCCESS --- OVERALL: " + str(passed_characterization) + " passed, " + str(total_characterization-passed_characterization) +
                                 " failed, " + str(len(lines)-total_characterization) + " queued --- " + local_args.replace("\\", ""))
                    if not args.no_clean:
                        for sub in os.listdir(cwd):
                            if os.path.isdir(os.path.join(cwd, sub)):
                                shutil.rmtree(os.path.join(cwd, sub))
                            else:
                                if sub != "eucalyptus_failed_output" and sub != "eucalyptus_return_value" and sub != "eucalyptus_execution_output" and sub != "characterization.xml" and sub != "args":
                                    os.remove(os.path.join(cwd, sub))
                elif return_value == 124:
                    logging.info("   FAILURE (Timeout) --- OVERALL: " + str(passed_characterization) + " passed, " + str(total_characterization -
                                 passed_characterization) + " failed, " + str(len(lines)-total_characterization) + " queued --- " + local_args.replace("\\", ""))
                elif return_value == 153:
                    logging.info("   FAILURE (File size limit exceeded) --- OVERALL: " + str(passed_characterization) + " passed, " + str(total_characterization -
                                 passed_characterization) + " failed, " + str(len(lines)-total_characterization) + " queued --- " + local_args.replace("\\", ""))
                else:
                    logging.info("   FAILURE --- OVERALL: " + str(passed_characterization) + " passed, " + str(total_characterization-passed_characterization) +
                                 " failed, " + str(len(lines)-total_characterization) + " queued --- " + local_args.replace("\\", ""))
            with lock:
                local_index = line_index
                line_index += 1


def ComputeDirectory(line):
    # Computing relative path
    device_name = ""
    component_name = ""
    tokens = shlex.split(line)
    for token in tokens:
        if token.find("--target-datafile") != -1:
            device_file = token[len("--target-datafile="):]
            device_name = os.path.basename(device_file)[:-len("-seed.xml")]
        if token.find("--characterize") != -1:
            if token.find(',') != -1:
                component_name = token[len("--characterize="):token.find(',')]
            else:
                component_name = token[len("--characterize="):]
    new_dir = os.path.join(out_path, device_name, component_name)
    return new_dir


def CollectResults(directory):
    # Collecting results
    # Skip if this is a leaf directory
    if os.path.exists(os.path.join(directory, "eucalyptus_return_value")) or os.listdir(directory) == []:
        return
    subdirs = [s for s in sorted(os.listdir(directory)) if os.path.isdir(
        os.path.join(directory, s)) and s != "panda-temp" and s != "HLS_output"]
    for subdir in subdirs:
        CollectResults(os.path.join(directory, subdir))
    tool_failed_output = open(os.path.join(
        directory, "eucalyptus_failed_output"), "w")
    for subdir in subdirs:
        if os.path.exists(os.path.join(directory, subdir, "eucalyptus_failed_output")):
            tool_failed_output.write(
                open(os.path.join(directory, subdir, "eucalyptus_failed_output")).read())
            if os.path.exists(os.path.join(directory, subdir, "eucalyptus_execution_output")):
                tool_failed_output.write("\n")
                tool_failed_output.write("\n")
                tool_failed_output.write("\n")
    tool_failed_output.close()
    if os.path.exists(os.path.join(directory, "eucalyptus_failed_output")):
        if os.stat(os.path.join(directory, "eucalyptus_failed_output")).st_size == 0:
            os.remove(os.path.join(directory, "eucalyptus_failed_output"))
    report_file = open(os.path.join(directory, "report"), "w")
    for subdir in subdirs:
        if os.path.exists(os.path.join(directory, subdir, "eucalyptus_return_value")):
            return_value_file_name = os.path.join(
                directory, subdir, "eucalyptus_return_value")
            return_value_file = open(return_value_file_name)
            return_value = return_value_file.read()
            return_value_file.close()
            args_file = open(os.path.join(directory, subdir, "args"))
            command_args = args_file.readlines()[0]
            args_file.close()
            if return_value == "0":
                tool_results_file_name = os.path.join(
                    directory, subdir, "eucalyptus_results")
                if os.path.exists(tool_results_file_name):
                    report_file.write(
                        "SUCCESS (" + open(tool_results_file_name).read() + " cycles) " + command_args.replace("\\", ""))
                else:
                    report_file.write(
                        "SUCCESS: " + command_args.replace("\\", ""))
            else:
                if return_value == "124":
                    report_file.write("FAILURE(Timeout): " +
                                      command_args.replace("\\", ""))
                else:
                    report_file.write(
                        "FAILURE: " + command_args.replace("\\", ""))
            report_file.write("\n")
        elif os.path.exists(os.path.join(directory, subdir, "report")):
            local_report_file = open(os.path.join(directory, subdir, "report"))
            report_file.write(local_report_file.read())
            local_report_file.close()
    report_file.close()


def find_file(name, path):
    # Find a file in apth
    for root, dirs, files in os.walk(path):
        if name in files:
            return os.path.join(root, name)


def positive_integer(value):
    pos_int = int(value)
    if pos_int <= 0:
        raise argparse.ArgumentTypeError(
            "%s must be a positive integer" % value)
    return pos_int


class StoreOrUpdateMin(argparse.Action):
    first_parsed = True

    def __call__(self, parser, namespace, values, option_string=None):
        if self.first_parsed == True:
            self.first_parsed = False
            setattr(namespace, self.dest, values)
        else:
            setattr(namespace, self.dest, min(namespace.j, values))


parser = argparse.ArgumentParser(
    description="Characterize device", fromfile_prefix_chars='@')
parser.add_argument('-c', "--commonargs",
                    help="A set of arguments to be passed to eucalyptus", nargs='*', action='append')
parser.add_argument("--devices", help="The devices to be characterized")
parser.add_argument("--technology-files",
                    help="The technology files containing the components")
parser.add_argument(
    "--update", help="The components whose characterations have to be updated", default="all")
parser.add_argument(
    "--list-only", help="Only generate the list of characterizations (default=\"device_fu_list\").", default="")
parser.add_argument(
    "--from-list", help="Perform characterization of components from previously generated list", default="")
parser.add_argument('-j', help="The number of jobs which execute the characterizations (default=\"1\")",
                    default=int(os.getenv('J', "1")), type=positive_integer, action=StoreOrUpdateMin)
parser.add_argument(
    '-o', "--output", help="The directory where output files we be put (default=\"output\")", default="output")
parser.add_argument('-s', "--spiderargs",
                    help="A set of arguments to be passed to spider", nargs='*', action='append')
parser.add_argument(
    '-t', "--timeout", help="Timeout for tool execution (default=60m)", default="1440m")
parser.add_argument("--fix", help="Correct the characterization times",
                    default=False, action="store_true")
parser.add_argument("--no-clean", help="Do not clean produced files",
                    default=False, action="store_true")
parser.add_argument("--restart", help="Restart last execution (default=false)",
                    default=False, action="store_true")
parser.add_argument("--stop", help="Stop the execution on first error (default=false)",
                    default=False, action="store_true")
parser.add_argument("--mail", help="Send a mail with the result")
parser.add_argument("--eucalyptus", help="The eucalyptus executable (default=eucalyptus)",
                    default="eucalyptus")
parser.add_argument("--spider", help="The spider executable (default=spider)",
                    default="spider")
parser.add_argument("--ulimit", help="The ulimit options",
                    default="-f 8388608 -s 32768-v 16777216 ")

args = parser.parse_args()
logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')

# Check spider executable
spider = ""
if os.path.isfile(args.spider) and os.access(args.spider, os.X_OK):
    spider = args.spider
else:
    # Check spider in the path
    for path in os.environ["PATH"].split(os.pathsep):
        exe_file = os.path.join(path, "spider")
        if os.path.isfile(exe_file) and os.access(exe_file, os.X_OK):
            spider = exe_file
if spider == "":
    logging.error("spider not found")
    sys.exit(1)
logging.info("Spider found: " + spider)

# Check eucalyptus executable
eucalyptus = ""
if os.path.isfile(args.eucalyptus) and os.access(args.eucalyptus, os.X_OK):
    eucalyptus = args.eucalyptus
else:
    # Check eucalyptus in the path
    for path in os.environ["PATH"].split(os.pathsep):
        exe_file = os.path.join(path, "eucalyptus")
        if os.path.isfile(exe_file) and os.access(exe_file, os.X_OK):
            eucalyptus = exe_file
if eucalyptus == "":
    logging.error("eucalyptus not found")
    sys.exit(1)
logging.info("Eucalyptus found: " + eucalyptus)

# Check mutt executable
if args.mail != None:
    # Check mutt in the path
    for path in os.environ["PATH"].split(os.pathsep):
        exe_file = os.path.join(path, "mutt")
        if os.path.isfile(exe_file) and os.access(exe_file, os.X_OK):
            mutt = exe_file
    if mutt == None:
        logging.error("mutt not found")
        sys.exit(1)


# Create the folder and enter in it
out_path = os.path.abspath(args.output)

# Check if output directory exists, if yes abort
if os.path.exists(args.output) and not args.restart:
    logging.error("Output directory " + args.output +
                  " already exists. Please remove it or specify a different one with -o")
    sys.exit(1)

device_fu_list_name = os.path.join(out_path, "device_fu_list")
if args.list_only != "":
    device_fu_list_name = args.list_only

# Check if device FUs list exists, if yes abort
if os.path.exists(device_fu_list_name) and not args.restart:
    logging.error("Device FUs list " + device_fu_list_name +
                  " already exists. Please remove it.")
    sys.exit(1)

if args.from_list != "":
    device_fu_list_name = args.from_list
    if not os.path.exists(device_fu_list_name):
        logging.error("Device FUs list " + device_fu_list_name + " not found.")
        sys.exit(1)

if args.restart:
    if not os.path.exists(out_path):
        args.restart = False
if not args.restart:
    os.mkdir(out_path)

if args.from_list == "":
    # Check technology files
    if args.fix:
        empty_technology_file = open(os.path.join(out_path, "empty.xml"), "w")
        empty_technology_file.write("<?xml version=\"1.0\"?>")
        empty_technology_file.write("<technology/>")
        empty_technology_file.close()
        args.technology_files = os.path.join(out_path, "empty.xml")
    else:
        if args.technology_files == None:
            logging.error("Missing technology files")
            sys.exit(1)

    # Check devices
    if args.devices == None:
        logging.error("Missing devices")
        sys.exit(1)

    device_files = []

    # Computing device file
    for device in args.devices.split(","):
        vendor_dirs_root = os.path.join(abs_panda, "etc/devices")
        device_file = ""
        for root, dirs, files in os.walk(vendor_dirs_root):
            for name in files:
                if name.endswith(device + "-seed.xml"):
                    device_file = os.path.join(root, name)
        if device_file == "":
            logging.error("seed file for " + device + " not found")
            sys.exit(1)
        device_files.append(device_file)

    device_fu_list = open(device_fu_list_name, "w")

    # Create device_fu_list
    for device in device_files:
        spider_args = ' '.join(
            map(os.path.abspath, args.technology_files.split(','))) + " /tmp/fu_list"
        spider_command = [spider]
        spider_command.append(device)
        spider_command.append("--components=" + args.update)
        spider_command.extend(shlex.split(spider_args))
        logging.info("Executing " + ' '.join(spider_command))
        return_value = subprocess.call(spider_command)
        fus = open("/tmp/fu_list").readlines()
        os.remove("/tmp/fu_list")

        for line in fus:
            arg_line = "--target-datafile=" + device + " --characterize=" + line.rstrip()
            if args.commonargs != None and len(args.commonargs) > 0:
                for commonarg in args.commonargs:
                    arg_line += " " + commonarg[0]
            device_fu_list.write(arg_line.replace(
                abs_panda, 'PANDA_ROOT') + '\n')

    device_fu_list.close()

if args.list_only != "":
    sys.exit(0)

# Create directories
if not args.restart:
    for device in args.devices.split(","):
        os.makedirs(os.path.join(out_path, device))
    lines = open(device_fu_list_name).readlines()
    for line in lines:
        new_dir = ComputeDirectory(line)
        logging.info("      Creating directory " + new_dir)
        os.makedirs(new_dir)

# Create threads
logging.info("   Launching tool")
lock = threading.RLock()
lock_creation_destruction = threading.RLock()
passed_characterization = 0
total_characterization = 0
line_index = 0
threads = []
children = [None] * args.j
killing = False
for thread_index in range(args.j):
    threads.insert(thread_index, threading.Thread(
        target=execute_characterization, args=(device_fu_list_name, thread_index)))
    threads[thread_index].daemon = True
    threads[thread_index].start()

try:
    # Wait threads
    for thread_index in range(args.j):
        while threads[thread_index].isAlive():
            threads[thread_index].join(100)
except KeyboardInterrupt:
    logging.error("SIGINT received")
    sys.exit(1)

# Collect results
CollectResults(out_path)

# Generate technology files
if not killing:
    for device in args.devices.split(","):
        if os.path.isdir(os.path.join(out_path, device)):
            # In this point device is the folder containing all the characterations of a device
            local_args = ""
            for characterization in os.listdir(os.path.join(out_path, device)):
                if os.path.exists(os.path.join(out_path, device, characterization, "characterization.xml")):
                    local_args = local_args + " " + \
                        os.path.join(out_path, device,
                                     characterization, "characterization.xml")
            local_command = [spider]
            # If this is not the first characterization, add the previous
            old_characterization = find_file(
                device + ".xml", os.path.join(abs_panda, "etc/devices"))
            if old_characterization != None:
                local_args = local_args + " " + old_characterization
            seed_file = find_file(device + "-seed.xml",
                                  os.path.join(abs_panda, "etc/devices"))
            if seed_file != None:
                local_args = local_args + " " + seed_file
            local_args = local_args + " " + device + ".xml"
            if args.spiderargs != None and len(args.spiderargs) > 0:
                for spiderarg in args.spiderargs:
                    local_args += " " + spiderarg[0]
            local_command.extend(shlex.split(local_args))
            logging.info("Executing " + ' '.join(local_command))
            return_value = subprocess.call(local_command)
if args.mail != None:
    outcome = ""
    if args.stop:
        if killing:
            outcome = "FAILURE"
        else:
            outcome = "SUCCESS"
    else:
        outcome = str(passed_characterization) + \
            "/" + str(total_characterization)
    full_name = "Eucalyptus"
    report_file_name = os.path.join(out_path, "report")
    local_command = "cat " + report_file_name + " | mutt -s \"" + \
        full_name + ": " + outcome + "\" " + args.mail
    subprocess.call(local_command, shell=True)
if killing:
    sys.exit(1)
sys.exit(0)
