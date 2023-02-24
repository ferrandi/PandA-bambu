#!/usr/bin/python

import argparse
import datetime
import distutils.spawn
import logging
import os
import re
import shlex
import shutil
import signal
import subprocess
import sys
import threading
import xml.dom.minidom
from collections import deque

line_index = 0
failure = False

def positive_integer(value):
    pos_int = int(value)
    if pos_int <= 0:
        raise argparse.ArgumentTypeError("%s must be a positive integer" % value)
    return pos_int

class StoreOrUpdateMin(argparse.Action):
    first_parsed = True
    def __call__(self, parser, namespace, values, option_string=None):
        if self.first_parsed == True :
            self.first_parsed = False
            setattr(namespace, self.dest, values)
        else :
            setattr(namespace, self.dest, min(namespace.j, values))


#Return children of a process
def GetChildren(parent_pid):
    ret = set()
    ps_command = subprocess.Popen("ps -o pid --ppid %d --noheaders" % parent_pid, shell=True, stdout=subprocess.PIPE)
    ps_output = ps_command.stdout.read()
    ps_command.wait()
    for pid_str in ps_output.split("\n")[:-1]:
        ret.add(int(pid_str))
    return ret

#Kill a process than kill its children
def kill_proc_tree(pid):
    children = GetChildren(pid)
    os.kill(pid, signal.SIGKILL)
    for child in children:
        kill_proc_tree(child)
#Process benchmark in list
def execute_tests(named_list,thread_index):
    global passed_benchmark
    global total_benchmark
    global line_index
    global children
    global failure
    lines = open(named_list).readlines()
    with lock:
        local_index = line_index
        line_index += 1
    while local_index < len(lines) and not (failure and args.stop):
        cwd = ComputeDirectory(lines[local_index])
        failed_output_file_name = os.path.join(cwd, args.tool + "_failed_output")
        if os.path.exists(failed_output_file_name):
            os.remove(failed_output_file_name)
        tool_return_value_file_name = os.path.join(cwd, args.tool + "_return_value")
        if args.restart and os.path.exists(os.path.join(cwd, args.tool + "_return_value")):
            tool_return_value_file = open(tool_return_value_file_name, "r")
            return_value = tool_return_value_file.read()
            tool_return_value_file.close()
            if return_value == "0":
                with lock:
                    total_benchmark += 1
                    passed_benchmark += 1
                    logging.info("   SKIPPING --- OVERALL: " + str(passed_benchmark) + " passed, " + str(total_benchmark-passed_benchmark) + " failed, " + str(len(lines)-total_benchmark) + " queued --- " + lines[local_index].replace("\\", ""))
                    local_index = line_index
                    line_index += 1
                continue
        HLS_output_directory = os.path.join(cwd, "HLS_output")
        if os.path.exists(HLS_output_directory):
            shutil.rmtree(HLS_output_directory)
        output_file_name = os.path.join(cwd, args.tool + "_execution_output")
        output_file = open(output_file_name, "w")
        local_args = lines[local_index]
        if local_args[0] == "\"":
            local_args = local_args[1:-1]
        if args.tool != "bambu" and args.tool != "zebu":
            tokens = shlex.split(lines[local_index])
            args_without_benchmark_name = ""
            for token in tokens:
                if token.find("--benchmark-name") == -1:
                    args_without_benchmark_name += token + " "
            local_args = args_without_benchmark_name
        local_command = "ulimit " + args.ulimit + "; exec timeout " + args.timeout + " " + tool_exe
        local_command = local_command + " " + local_args
        output_file.write("#" * 80 + "\n")
        output_file.write("cd " + cwd + "; ")
        output_file.write(local_command + "\n")
        output_file.write("#" * 80 + "\n")
        output_file.flush()
        return_value = -1
        with lock_creation_destruction:
            if not (failure and args.stop):
                children[thread_index] = subprocess.Popen(local_command, stderr=output_file, stdout=output_file, cwd=cwd, shell=True, executable="/bin/bash")
        try:
            return_value = children[thread_index].wait()
        except:
            pass
        with lock_creation_destruction:
            if return_value != 0 and (args.stop or args.returnfail):
                failure = True
            if failure and args.stop:
                for local_thread_index in range(n_jobs):
                    if children[local_thread_index] != None:
                        if children[local_thread_index].poll() == None:
                            try:
                                kill_proc_tree(children[local_thread_index].pid)
                            except OSError:
                               pass
        os.fsync(output_file.fileno())
        output_file.close()
        tool_return_value_file = open(tool_return_value_file_name, "w")
        tool_return_value_file.write(str(return_value))
        tool_return_value_file.close()
        args_file = open(os.path.join(cwd, "args"), "w")
        args_file.write(lines[local_index])
        args_file.close()
        if return_value == 0 and os.path.exists(os.path.join(cwd, args.tool + "_results_0.xml")):
            tool_results_file_name = os.path.join(cwd, args.tool + "_results")
            tool_results_file = open(tool_results_file_name, "w")
            tool_results_string = ""
            xml_document = xml.dom.minidom.parse(os.path.join(cwd, args.tool + "_results_0.xml"))
            if len(xml_document.getElementsByTagName("CYCLES")) > 0:
                cycles_tag = xml_document.getElementsByTagName("CYCLES")[0]
                tool_results_string = tool_results_string + cycles_tag.attributes["value"].value + " CYCLES"
            if len(xml_document.getElementsByTagName("CLOCK_SLACK")) > 0:
                slack_tag = xml_document.getElementsByTagName("CLOCK_SLACK")[0]
                tool_results_string = tool_results_string + " *** " + slack_tag.attributes["value"].value + "ns"
            tool_results_file.write(tool_results_string)
            tool_results_file.close()
        if not (failure and args.stop) or (return_value != -9 and return_value != 0):
            if return_value != 0:
                shutil.copy(output_file_name, str(os.path.join(os.path.dirname(output_file_name), args.tool + "_failed_output")))
            with lock:
                total_benchmark += 1
                if return_value == 0:
                    passed_benchmark += 1
                    if not args.no_clean:
                        for sub in os.listdir(cwd):
                            if os.path.isdir(os.path.join(cwd, sub)):
                                shutil.rmtree(os.path.join(cwd, sub))
                            else:
                                if sub != args.tool + "_return_value" and sub != args.tool + "_execution_output" and sub != args.tool + "_results_0.xml" and sub != "args":
                                    os.remove(os.path.join(cwd, sub))
                    if os.path.exists(os.path.join(cwd, args.tool + "_results_0.xml")):
                        logging.info("   SUCCESS (" + tool_results_string + ") --- OVERALL: " + str(passed_benchmark) + " passed, " + str(total_benchmark-passed_benchmark) + " failed, " + str(len(lines)-total_benchmark) + " queued --- " + lines[local_index].replace("\\", ""))
                    else:
                        logging.info("   SUCCESS --- OVERALL: " + str(passed_benchmark) + " passed, " + str(total_benchmark-passed_benchmark) + " failed, " + str(len(lines)-total_benchmark) + " queued --- " + lines[local_index].replace("\\", ""))
                elif return_value == 124:
                    logging.info("   FAILURE (Timeout) --- OVERALL: " + str(passed_benchmark) + " passed, " + str(total_benchmark-passed_benchmark) + " failed, " + str(len(lines)-total_benchmark) + " queued --- " + lines[local_index].replace("\\", ""))
                elif return_value == 153:
                    logging.info("   FAILURE (File size limit exceeded) --- OVERALL: " + str(passed_benchmark) + " passed, " + str(total_benchmark-passed_benchmark) + " failed, " + str(len(lines)-total_benchmark) + " queued --- " + lines[local_index].replace("\\", ""))
                else:
                    logging.info("   FAILURE --- OVERALL: " + str(passed_benchmark) + " passed, " + str(total_benchmark-passed_benchmark) + " failed, " + str(len(lines)-total_benchmark) + " queued --- " + lines[local_index].replace("\\", ""))
            with lock:
                local_index = line_index
                line_index += 1

#Computing relative path
def ComputeDirectory(line):
    configuration_name = ""
    benchmark_name = ""
    tokens = shlex.split(line)
    for token in tokens:
        if token.find("--configuration-name") != -1:
            configuration_name = token[len("--configuration-name="):]
        if token.find("--benchmark-name") != -1:
            benchmark_name = token[len("--benchmark-name="):]
    new_dir = os.path.join(abs_path, configuration_name, benchmark_name)
    return new_dir

#Search c files
def SearchCFiles(directory):
    logging.info("       Looking for file in " + str(directory))
    files = set()
    for element in os.listdir(directory):
        if os.path.isdir(os.path.join(directory, element)):
            files = files.union(SearchCFiles(os.path.join(directory, element)))
        elif (element[-2:] == ".c")  or (element[-2:] == ".C") or (element[-4:] == ".CPP") or (element[-4:] == ".cpp") or (element[-4:] == ".cxx") or (element[-3:] == ".cc") or (element[-4:] == ".c++"):
            files.add(os.path.join(directory, element))
    return files

#Collecting results
def CollectResults(directory):
    #Skip if this is a leaf directory
    if os.path.exists(os.path.join(directory, args.tool + "_return_value")) or os.listdir(directory) == []:
        return
    subdirs = [s for s in sorted(os.listdir(directory)) if os.path.isdir(os.path.join(directory,s)) and s != "panda-temp" and s != "HLS_output"]
    for subdir in subdirs:
        CollectResults(os.path.join(directory, subdir))
    tool_failed_output = open(os.path.join(directory, args.tool + "_failed_output"), "w")
    for subdir in subdirs:
        if os.path.exists(os.path.join(directory, subdir, args.tool + "_failed_output")):
            tool_failed_output.write(open(os.path.join(directory, subdir, args.tool + "_failed_output")).read())
            if os.path.exists(os.path.join(directory, subdir, args.tool + "_execution_output")):
                tool_failed_output.write("\n")
                tool_failed_output.write("\n")
                tool_failed_output.write("\n")
    tool_failed_output.close()
    report_file = open(os.path.join(directory, "report"), "w")
    for subdir in subdirs:
        if os.path.exists(os.path.join(directory, subdir, args.tool + "_return_value")):
            return_value_file_name = os.path.join(directory, subdir, args.tool + "_return_value")
            return_value_file = open(return_value_file_name)
            return_value = return_value_file.read()
            return_value_file.close()
            args_file = open(os.path.join(directory, subdir, "args"))
            command_args = args_file.readlines()[0]
            command_args = command_args.replace(abs_benchmarks_root + "/", "")
            args_file.close()
            if return_value == "0":
                tool_results_file_name = os.path.join(directory, subdir, args.tool + "_results")
                if os.path.exists(tool_results_file_name):
                    report_file.write("SUCCESS (" + open(tool_results_file_name).read() + " cycles) " + command_args.replace("\\", ""))
                else:
                    report_file.write("SUCCESS: " + command_args.replace("\\", ""))
            else:
                if return_value == "124":
                    report_file.write("FAILURE(Timeout): " + command_args.replace("\\", ""))
                else:
                    report_file.write("FAILURE: " + command_args.replace("\\", ""))
            report_file.write("\n")
        elif os.path.exists(os.path.join(directory, subdir, "report")):
            local_report_file = open(os.path.join(directory, subdir, "report"))
            report_file.write(local_report_file.read())
            local_report_file.close()
    report_file.close()
    if args.tool == "bambu":
        local_args = ""
        named_list_name = os.path.join(abs_path, "named_list")
        lines = open(named_list_name).readlines()
        for line in lines:
            local_dir = ComputeDirectory(line)
            if os.path.exists(os.path.join(local_dir, args.tool + "_results_0.xml")):
                local_args = local_args + " " + os.path.join(local_dir, args.tool + "_results_0.xml")
        if len(local_args) > 0:
            #Generate experimental setup xml
            experimental_setup_file_name = os.path.join(abs_path, "experimental_setup.xml")
            temp_list = open(experimental_setup_file_name, "w")
            bambu_version_file_name = os.path.join(abs_path, "bambu_version")
            bambu_version_file = open(bambu_version_file_name, "w")
            bambu_version_command = [tool_exe]
            bambu_version_command.extend(shlex.split("--version"))
            subprocess.call(bambu_version_command, stdout=bambu_version_file)
            bambu_version_file.close()
            bambu_version_file = open(bambu_version_file_name, "r")
            bambu_version = bambu_version_file.readlines()[-2].rstrip()
            bambu_version_file.close()
            if args.commonargs != None:
               bambu_arguments =  ' '.join(' '.join(map(str,l)) for l in args.commonargs)
            else:
               bambu_arguments = ""
            temp_list.write("<?xml version=\"1.0\"?>\n")
            temp_list.write("<experimental_setup>\n")
            temp_list.write("   <bambu_version value=\"" + bambu_version + "\"/>\n")
            temp_list.write("   <timestamp value=\"" + str(datetime.datetime.now()) + "\"/>\n")
            temp_list.write("   <script value=\"" + args.script + "\"/>\n")
            temp_list.write("   <bambu_arguments value=\"" + bambu_arguments + "\"/>\n")
            temp_list.write("   <benchmarks>\n")
            reordered_list_name = os.path.join(abs_path, "reordered_list")
            reordered_list = open(reordered_list_name, "r")
            for line in reordered_list.readlines():
                temp_list.write("      <benchmark value=\"" + line.rstrip().replace("\\\\\\-","-").replace("\\\\\\=", "=").replace("\\\\\\_", "_").replace("\\\\\\/", "/").replace("\\\\\\\"", "&quot;").replace("\\\\\\.", ".")  + "\"/>\n")
            temp_list.write("   </benchmarks>\n")
            temp_list.write("</experimental_setup>\n")
            temp_list.close();
            local_args = local_args + " " + experimental_setup_file_name
            if os.path.exists(args.spider_style) :
                local_args = local_args + " " + args.spider_style + " " + table
            else:
                local_args = local_args + " " + os.path.join(os.path.dirname(spider), args.spider_style) + " " + table
#            logging.info("   Executing " + spider + " " + local_args)
            logging.info("   Executing " + spider)
            local_command = [spider]
            local_command.extend(shlex.split(local_args))
            return_value = subprocess.call(local_command)
        logging.info("Collected results of " + directory)


#Create Junit Body
def CreateJunitBody(directory,ju_file):
    #Skip if this is a leaf directory
    if os.path.exists(os.path.join(directory, args.tool + "_return_value")) or os.listdir(directory) == []:
        return
    subdirs = [s for s in sorted(os.listdir(directory)) if os.path.isdir(os.path.join(directory,s)) and s != "panda-temp" and s != "HLS_output"]
    print_testsuite = False
    for subdir in subdirs:
        if os.path.exists(os.path.join(directory, subdir, args.tool + "_return_value")):
            print_testsuite = True
        CreateJunitBody(os.path.join(directory, subdir),ju_file)
    failed_counter_file_name = os.path.join(abs_path, "failed_counter")
    failed_counter = "0"
    if os.path.exists(failed_counter_file_name):
        failed_counter_file = open(failed_counter_file_name)
        failed_counter = failed_counter_file.read()

    if print_testsuite and len(subdirs) > 0:
        ju_file.write("  <testsuite disabled=\"0\" errors=\"0\" failures=\""+failed_counter+"\" name=\""+directory+"\" tests=\""+ str(len(subdirs)) + "\" timestamp=\"" + str(datetime.datetime.now()) + "\">\n")
    for subdir in subdirs:
        if os.path.exists(os.path.join(directory, subdir, args.tool + "_return_value")):
            return_value_file_name = os.path.join(directory, subdir, args.tool + "_return_value")
            return_value_file = open(return_value_file_name)
            return_value = return_value_file.read()
            return_value_file.close()
            args_file = open(os.path.join(directory, subdir, "args"))
            command_args = args_file.readlines()[0]
            command_args = command_args.replace(abs_benchmarks_root + "/", "")
            args_file.close()
            if return_value == "0":
                ju_file.write("    <testcase classname=\"PandA-bambu-tests\" name=\"" + command_args.replace("\\", "") + "\">\n")
            else:
                if return_value == "124":
                    ju_file.write("    <testcase classname=\"PandA-bambu-tests\" name=\"" + command_args.replace("\\", "") + "\">\n")
                    ju_file.write("      <failure type=\"FAILURE(Timeout)\"></failure>\n")
                    ju_file.write("      <system-out>\n")
                    ju_file.write("<![CDATA[\n")
                else:
                    ju_file.write("    <testcase classname=\"PandA-bambu-tests\" name=\"" + command_args.replace("\\", "") + "\">\n")
                    ju_file.write("      <failure type=\"FAILURE\"></failure>\n")
                    ju_file.write("      <system-out>\n")
                    ju_file.write("<![CDATA[\n")
                with open(os.path.join(directory, args.tool + "_failed_output")) as f:
                    for line in deque(f, maxlen=15):
                        ju_file.write(line)
                ju_file.write("]]>\n")
                ju_file.write("      </system-out>\n")
            ju_file.write("    </testcase>\n")
    if print_testsuite and len(subdirs) > 0:
        ju_file.write("  </testsuite>\n")


#Create PerfPublisher Body
def CreatePerfPublisherBody(directory,pp_file):
    #Skip if this is a leaf directory
    if os.path.exists(os.path.join(directory, args.tool + "_return_value")) or os.listdir(directory) == []:
        return
    subdirs = [s for s in sorted(os.listdir(directory)) if os.path.isdir(os.path.join(directory,s)) and s != "panda-temp" and s != "HLS_output"]
    print_testsuite = False
    for subdir in subdirs:
        if os.path.exists(os.path.join(directory, subdir, args.tool + "_return_value")):
            print_testsuite = True
        CreatePerfPublisherBody(os.path.join(directory, subdir),pp_file)

    for subdir in subdirs:
        if os.path.exists(os.path.join(directory, subdir, args.tool + "_return_value")):
            pp_file.write("  <test name=\""+ str(directory)+"/"+ str(subdir) +"\" executed=\"yes\">\n")
            pp_file.write("    <result>\n")
            return_value_file_name = os.path.join(directory, subdir, args.tool + "_return_value")
            return_value_file = open(return_value_file_name)
            return_value = return_value_file.read()
            return_value_file.close()
            args_file = open(os.path.join(directory, subdir, "args"))
            command_args = args_file.readlines()[0]
            command_args = command_args.replace(abs_benchmarks_root + "/", "")
            args_file.close()
            if return_value == "0":
                pp_file.write("      <success passed=\"yes\" state=\"100\" hasTimedOut=\"false\"/>\n")
                cycles_tag = ""
                areatime_tag = ""
                slice_tag = ""
                sliceluts_tag = ""
                registers_tag = ""
                dsps_tag = ""
                brams_tag = ""
                period_tag = ""
                dsps_tag = ""
                slack_tag = ""
                frequency_tag = ""
                HLS_execution_time_tag = ""
                if os.path.exists(os.path.join(directory, subdir, args.tool + "_results_0.xml")):
                    xml_document = xml.dom.minidom.parse(os.path.join(directory, subdir, args.tool + "_results_0.xml"))
                    if len(xml_document.getElementsByTagName("CYCLES")) > 0:
                        cycles_tag = str(xml_document.getElementsByTagName("CYCLES")[0].attributes["value"].value)
                    if len(xml_document.getElementsByTagName("AREAxTIME")) > 0:
                        areatime_tag = str(xml_document.getElementsByTagName("AREAxTIME")[0].attributes["value"].value)
                    if len(xml_document.getElementsByTagName("SLICE")) > 0:
                        slice_tag = str(xml_document.getElementsByTagName("SLICE")[0].attributes["value"].value)
                    if len(xml_document.getElementsByTagName("SLICE_LUTS")) > 0:
                        sliceluts_tag = str(xml_document.getElementsByTagName("SLICE_LUTS")[0].attributes["value"].value)
                    if len(xml_document.getElementsByTagName("REGISTERS")) > 0:
                        registers_tag = str(xml_document.getElementsByTagName("REGISTERS")[0].attributes["value"].value)
                    if len(xml_document.getElementsByTagName("DSPS")) > 0:
                        dsps_tag = str(xml_document.getElementsByTagName("DSPS")[0].attributes["value"].value)
                    if len(xml_document.getElementsByTagName("BRAMS")) > 0:
                        brams_tag = str(xml_document.getElementsByTagName("BRAMS")[0].attributes["value"].value)
                    if len(xml_document.getElementsByTagName("PERIOD")) > 0:
                        period_tag = str(xml_document.getElementsByTagName("PERIOD")[0].attributes["value"].value)
                    if len(xml_document.getElementsByTagName("CLOCK_SLACK")) > 0:
                        slack_tag = str(xml_document.getElementsByTagName("CLOCK_SLACK")[0].attributes["value"].value)
                    if len(xml_document.getElementsByTagName("FREQUENCY")) > 0:
                        frequency_tag = str(xml_document.getElementsByTagName("FREQUENCY")[0].attributes["value"].value)
                    if len(xml_document.getElementsByTagName("HLS_execution_time")) > 0:
                        HLS_execution_time_tag = str(xml_document.getElementsByTagName("HLS_execution_time")[0].attributes["value"].value)

                if cycles_tag != "":
                    pp_file.write("      <performance unit=\"cycles\" mesure=\""+ cycles_tag + "\" isRelevant=\"true\"/>\n")
                if HLS_execution_time_tag != "":
                    pp_file.write("      <executiontime unit=\"s\" mesure=\""+ HLS_execution_time_tag + "\" isRelevant=\"true\"/>\n")

                if areatime_tag != "" or slice_tag != "" or sliceluts_tag != "" or registers_tag != "" or dsps_tag != "" or brams_tag != "" or period_tag != "" or  dsps_tag != "" or slack_tag != "" or frequency_tag != "":
                    pp_file.write("      <metrics>\n")
                    if areatime_tag != "":
                        pp_file.write("        <areatime unit=\"lutxns\" mesure=\""+ areatime_tag + "\" isRelevant=\"true\"/>\n")
                    if slice_tag != "":
                        pp_file.write("        <slices unit=\"ns\" mesure=\""+ slice_tag + "\" isRelevant=\"true\"/>\n")
                    if sliceluts_tag != "":
                        pp_file.write("        <sliceluts unit=\"slice\" mesure=\""+ sliceluts_tag + "\" isRelevant=\"true\"/>\n")
                    if registers_tag != "":
                        pp_file.write("        <registers unit=\"registers\" mesure=\""+ registers_tag + "\" isRelevant=\"true\"/>\n")
                    if dsps_tag != "":
                        pp_file.write("        <dsps unit=\"dsp\" mesure=\""+ dsps_tag + "\" isRelevant=\"true\"/>\n")
                    if brams_tag != "":
                        pp_file.write("        <brams unit=\"bram\" mesure=\""+ brams_tag + "\" isRelevant=\"true\"/>\n")
                    if period_tag != "":
                        pp_file.write("        <period unit=\"ns\" mesure=\""+ period_tag + "\" isRelevant=\"true\"/>\n")
                    if slack_tag != "":
                        pp_file.write("        <slack unit=\"ns\" mesure=\""+ slack_tag + "\" isRelevant=\"true\"/>\n")
                    if frequency_tag != "":
                        pp_file.write("        <frequency unit=\"MHz\" mesure=\""+ frequency_tag + "\" isRelevant=\"true\"/>\n")
                    pp_file.write("      </metrics>\n")
            else:
                if return_value == "124":
                    pp_file.write("      <success passed=\"no\" state=\"0\" hasTimedOut=\"true\"/>\n")
                else:
                    pp_file.write("      <success passed=\"no\" state=\"0\" hasTimedOut=\"false\"/>\n")
            pp_file.write("    </result>\n")
            pp_file.write("  </test>\n")

parser = argparse.ArgumentParser(description="Performs panda tests", fromfile_prefix_chars='@')
parser.add_argument("files", help="The files to be tested: they can be configuration files, directories containing benchmarks or source code files.", nargs='*', action="append")
parser.add_argument('-l', "--benchmarks_list", help="The file containing the list of tests to be performed", nargs='*', action="append")
parser.add_argument('-b', "--benchmarks_root", help="The directory containing benchmarks")
parser.add_argument('-o', "--output", help="The directory where output files we be put (default=\"output\")", default="output")
parser.add_argument('-j', help="The number of jobs which execute the benchmarks (default=\"1\")", default=1, type=positive_integer, action=StoreOrUpdateMin)
parser.add_argument("--bambu", help="The bambu executable (default=/opt/panda/bin/bambu)", default="/opt/panda/bin/bambu")
parser.add_argument("--spider", help="The spider executable (default=/opt/panda/bin/spider)", default="/opt/panda/bin/spider")
parser.add_argument("--spider-style", help="The spider table style relative to the spider executable (default=../lib/latex_format_bambu_results.xml)", default="../lib/latex_format_bambu_results.xml")
parser.add_argument("--zebu", help="The zebu executable (default=/opt/panda/bin/zebu)", default="/opt/panda/bin/zebu")
parser.add_argument('-t', "--timeout", help="Timeout for tool execution (default=60m)", default="60m")
parser.add_argument('-a', "--args", help="A set of arguments to be passed to the tool", nargs='*', action='append')
parser.add_argument('-c', "--commonargs", help="A set of arguments to be passed to the tool", nargs='*', action='append')
parser.add_argument("--table", help="Print the results in tex format", default="results.tex")
parser.add_argument("--tool", help="The tool to be tested", default="bambu")
parser.add_argument("--ulimit", help="The ulimit options", default="-f 2097152 -v 8388608 -s 16384")
parser.add_argument("--stop", help="Stop the execution on first error (default=false)", default=False, action="store_true")
parser.add_argument("--returnfail", help="Return FAILURE in case at least one test fails (default=false)", default=False, action="store_true")
parser.add_argument("--mail", help="Send a mail with the result")
parser.add_argument("--name", help="Set the name of this regression (default=Bambu regression)", nargs='*', action='append')
parser.add_argument("--no-clean", help="Do not clean produced files", default=False, action="store_true")
parser.add_argument("--restart", help="Restart last execution (default=false)", default=False, action="store_true")
parser.add_argument("--script", help="Set the bash script in the generated tex", default="")
parser.add_argument("--junitdir", help="Set the JUnit directory", default="")
parser.add_argument("--perfpublisherdir", help="Set the PerfPublisher directory", default="")

args = parser.parse_args()
n_jobs = args.j # set this, because it will be overwritten by the parse of modified_argv
logging.basicConfig(level=logging.INFO,format='%(levelname)s: %(message)s')

#The absolute path of current script
abs_script = os.path.abspath(sys.argv[0])

#Expand configuration files
modified_argv = []
modified_argv.append(sys.argv[0])
abs_configuration_dir=""
for arg in sys.argv[1:]:
    if arg in args.files[0]:
        #.c/c++ file
        if (arg[-2:] == ".c") or (arg[-2:] == ".C") or (arg[-4:] == ".CPP") or (arg[-4:] == ".cpp") or (arg[-4:] == ".cxx") or (arg[-3:] == ".cc") or (arg[-4:] == ".c++"):
            modified_argv.append(arg)
        #Check if it is a directory
        elif os.path.exists(arg) and os.path.isdir(arg):
            modified_argv.append(arg)
        elif args.benchmarks_root != None and os.path.exists(os.path.join(os.path.abspath(args.benchmarks_root), arg)) and os.path.isdir(os.path.join(os.path.abspath(args.benchmarks_root), arg)):
            modified_argv.append(arg)
        elif os.path.exists(os.path.join(os.path.dirname(abs_script), arg)) and os.path.isdir(os.path.join(os.path.dirname(abs_script), arg)):
            modified_argv.append(arg)
        else:
            modified_argv.append("@" + arg)
    else:
        modified_argv.append(arg)
args = parser.parse_args(modified_argv)

#The absolute path of current script
abs_script = os.path.abspath(sys.argv[0])

#The table to be produced
table = os.path.abspath(args.table)
#Check if output directory exists, if yes abort
if os.path.exists(args.output) and not args.restart:
    logging.error("Output directory " + args.output + " already exists. Please remove it or specify a different one with -o")
    sys.exit(1)

#Check if JUnit dir exist
if args.junitdir != "" and not os.path.exists(args.junitdir):
    os.mkdir(args.junitdir)
#Check if PerfPublisher dir exist
if args.perfpublisherdir != "" and not os.path.exists(args.perfpublisherdir):
    os.mkdir(args.perfpublisherdir)
#compute JUnit file name
junit_file_name =  ""
if args.junitdir != "":
    junit_index = 0
    junit_file_name = os.path.abspath(os.path.join(args.junitdir, "Junit_report"+str(junit_index)+".xml"))
    while os.path.isfile(junit_file_name) :
        junit_index = junit_index + 1
        junit_file_name = os.path.abspath(os.path.join(args.junitdir, "Junit_report"+str(junit_index)+".xml"))
#compute PerfPublisher file name
perfpublisher_name =  ""
perfpublisher_file_name =  ""
if args.perfpublisherdir != "":
    perfpublisher_index = 0
    perfpublisher_name =  "PerfPublisher_report"+str(perfpublisher_index)
    perfpublisher_file_name = os.path.abspath(os.path.join(args.perfpublisherdir, perfpublisher_name+".xml"))
    while os.path.isfile(perfpublisher_file_name) :
        perfpublisher_index = perfpublisher_index + 1
        perfpublisher_name =  "PerfPublisher_report"+str(perfpublisher_index)
        perfpublisher_file_name = os.path.abspath(os.path.join(args.perfpublisherdir, perfpublisher_name+".xml"))

#Create the folder and enter in it
abs_path = os.path.abspath(args.output)

if args.restart:
    if not os.path.exists(abs_path):
        args.restart = False
if not args.restart:
    os.mkdir(abs_path)
os.chdir(abs_path)


#Skipping if all benchmarks already pass
if args.restart:
    failed_counter_file_name = os.path.join(abs_path, "failed_counter")
    if os.path.exists(failed_counter_file_name):
        failed_counter_file = open(failed_counter_file_name)
        failed_counter = failed_counter_file.read()
        if failed_counter == "0" and args.junitdir == "" and args.perfpublisherdir == "":
            logging.info("Already pass")
            sys.exit(0)

#Check tool executable
tool_exe = ""
if args.tool == "bambu":
    if os.path.isfile(args.bambu) and os.access(args.bambu, os.X_OK):
        tool_exe = args.bambu
    else:
        #Check bambu in the path
        for path in os.environ["PATH"].split(os.pathsep):
            exe_file = os.path.join(path, "bambu")
            if os.path.isfile(exe_file) and os.access(exe_file, os.X_OK):
                tool_exe = exe_file
    if tool_exe == "":
        if args.bambu != "opt/panda/bin/bambu":
            if not os.path.isfile(args.bambu):
                logging.error(args.bambu + " does not exist")
            else:
                logging.error(args.bambu + " is not an executable")
        else:
            logging.error("bambu not found")
        sys.exit(1)
    logging.info("Bambu found: " + tool_exe)
elif args.tool == "zebu":
    if os.path.isfile(args.zebu) and os.access(args.zebu, os.X_OK):
        tool_exe = args.zebu
    else:
        #Check zebu in the path
        for path in os.environ["PATH"].split(os.pathsep):
            exe_file = os.path.join(path, "zebu")
            if os.path.isfile(exe_file) and os.access(exe_file, os.X_OK):
                tool_exe = exe_file
    if tool_exe == "":
        if args.zebu != "opt/panda/bin/zebu":
            if not os.path.isfile(args.zebu):
                logging.error(args.zebu + " does not exist")
            else:
                logging.error(args.zebu + " is not an executable")
        else:
            logging.error("zebu not found")
        sys.exit(1)
    logging.info("Zebu found: " + tool_exe)
else:
   tool_exe = args.tool
   if distutils.spawn.find_executable(tool_exe) == None:
      logging.error(tool_exe + " not found")
      sys.exit(1)

if args.benchmarks_root is None:
     abs_benchmarks_root = abs_configuration_dir
else:
    if os.path.isabs(args.benchmarks_root):
        abs_benchmarks_root = os.path.abspath(args.benchmarks_root)
    else:
        if os.path.exists(os.path.join(os.path.abspath(".."), args.benchmarks_root)):
            abs_benchmarks_root = os.path.join(os.path.abspath(".."), args.benchmarks_root)
        else:
            if os.path.exists(os.path.join(os.path.abspath(os.path.join(os.path.dirname(abs_script), "../../..")), args.benchmarks_root)):
                abs_benchmarks_root = os.path.join(os.path.abspath(os.path.join(os.path.dirname(abs_script), "../../..")), args.benchmarks_root)
            else:
                logging.error(args.benchmarks_root + " not found")
                sys.exit(1)

#Check spider executable
spider = ""
if os.path.isfile(args.spider) and os.access(args.spider, os.X_OK):
    spider = args.spider
else:
    #Check spider in the path
    for path in os.environ["PATH"].split(os.pathsep):
        exe_file = os.path.join(path, "spider")
        if os.path.isfile(exe_file) and os.access(exe_file, os.X_OK):
            spider = exe_file
if spider == "":
    if args.spider != "opt/panda/bin/spider":
        if not os.path.isfile(args.spider):
            logging.error(args.spider + " does not exist")
        else:
            logging.error(args.spider + " is not an executable")
    else:
        logging.error("spider not found")
    sys.exit(1)

if args.benchmarks_root is None:
     abs_benchmarks_root = abs_configuration_dir
else:
    if os.path.isabs(args.benchmarks_root):
        abs_benchmarks_root = os.path.abspath(args.benchmarks_root)
    else:
        if os.path.exists(os.path.join(os.path.abspath(".."), args.benchmarks_root)):
            abs_benchmarks_root = os.path.join(os.path.abspath(".."), args.benchmarks_root)
        else:
            if os.path.exists(os.path.join(os.path.abspath(os.path.join(os.path.dirname(abs_script), "../../..")), args.benchmarks_root)):
                abs_benchmarks_root = os.path.join(os.path.abspath(os.path.join(os.path.dirname(abs_script), "../../..")), args.benchmarks_root)
            else:
                logging.error(args.benchmarks_root + " not found")
                sys.exit(1)
mutt = None
logging.info("Spider found: " + spider)

#Check mutt executable
if args.mail != None:
    #Check mutt in the path
    for path in os.environ["PATH"].split(os.pathsep):
        exe_file = os.path.join(path, "mutt")
        if os.path.isfile(exe_file) and os.access(exe_file, os.X_OK):
            mutt = exe_file
    if mutt == None:
        logging.error("mutt not found")
        sys.exit(1)


if not args.restart:
    #Check if file lists exist
    abs_lists = []
    if args.benchmarks_list != None:
        for relative_list in args.benchmarks_list:
            #First look in the current directory
            if os.path.exists(os.path.abspath("../" + relative_list[0])):
                abs_lists.append(os.path.abspath("../" + relative_list[0]))
            #Then look in script directory
            elif os.path.exists(os.path.join(os.path.dirname(abs_script), relative_list[0])):
                abs_lists.append(os.path.join(os.path.dirname(abs_script), relative_list[0]))
            #Then look in configuration directory
            elif os.path.exists(os.path.join(abs_configuration_dir, relative_list[0])):
                abs_lists.append(os.path.join(abs_configuration_dir, relative_list[0]))
            #Then look in benchmarks root
            elif os.path.exists(os.path.join(abs_benchmarks_root, relative_list[0])):
                abs_lists.append(os.path.join(abs_benchmarks_root, relative_list[0]))
            else:
                logging.error(relative_list[0] + " does not exist")
                sys.exit(1)
    files_list = []
    #Create temp list with arg
    for arg in args.files[0]:
        #.c/.c++ file
        if (arg[-2:] == ".c") or (arg[-2:] == ".C") or (arg[-4:] == ".CPP") or (arg[-4:] == ".cpp") or (arg[-4:] == ".cxx") or (arg[-3:] == ".cc") or (arg[-4:] == ".c++"):
            files_list.append(arg)
        #Check if it is a directory
        elif os.path.exists(arg) and os.path.isdir(arg):
            files_list.append(arg)
        elif os.path.exists(os.path.join(os.path.dirname(abs_script), arg)) and os.path.isdir(os.path.join(os.path.dirname(abs_script), arg)):
            files_list.append(arg)
        elif os.path.exists(os.path.join(abs_benchmarks_root, arg)) and os.path.isdir(os.path.join(abs_benchmarks_root, arg)):
            files_list.append(os.path.join(abs_benchmarks_root, arg))

    if args.benchmarks_list == None and len(files_list) == 0 and (args.tool == "bambu" or args.tool == "zebu"):
        logging.error("Benchmarks not found")
        sys.exit(2)

    if len(files_list) > 0:
        temp_list = open(os.path.join(abs_path, "temp_list"), "w")
        for element in files_list:
            temp_list.write(element)
        temp_list.close()
        abs_lists.append(os.path.join(abs_path, "temp_list"))

    #Reordering elements in each row
    reordered_list_name = os.path.join(abs_path, "reordered_list")
    reordered_list = open(reordered_list_name, "w")

    logging.info("Preparing benchmark list")
    logging.info("   Reordering arguments")
    for abs_list in abs_lists:
        list_file = open(abs_list)
        lines = list_file.readlines()
        list_file.close()
        for line in lines:
            if line.strip() == "":
                continue
            if line[0] =='#':
                continue
            if args.tool != "bambu" and args.tool != "zebu":
                reordered_list.write(line)
                continue
            tokens = shlex.split(line)
            parameters = list()
            #Flag used to ad-hoc manage --param arg
            follow_param = False
            for token in tokens:
                if token[0] == '-':
                    parameters.append(re.escape(token))
                    if token.find("--param") != -1:
                        follow_param = True;
                    else:
                        follow_param = False;
                else:
                    if follow_param == True:
                        parameters.append(re.escape(token))
                    else:
                        reordered_list.write(token + " ")
                    follow_param = False;
            for parameter in parameters:
                reordered_list.write(re.escape(parameter) + " ")
            reordered_list.write("\n")
    reordered_list.close()

    #Expanding directory
    expanded_list_name = os.path.join(abs_path, "expanded_list")
    expanded_list = open(expanded_list_name, "w")

    logging.info("   Expanding directory")
    lines = open(reordered_list_name).readlines()
    for line in lines:
        if line.strip() == "":
            continue
        tokens = shlex.split(line)
        if args.tool == "bambu" or args.tool == "zebu":
            if(tokens[0][0] != '/'):
               first_parameter = os.path.join(abs_benchmarks_root, tokens[0])
            else:
               first_parameter = tokens[0]
        else:
            first_parameter = tokens[0].replace("BENCHMARKS\_ROOT", abs_benchmarks_root)
        other_parameters = tokens[1:len(tokens)]
        if not os.path.exists(first_parameter) and (args.tool == "bambu" or args.tool == "zebu"):
            logging.error(first_parameter + " does not exist")
            sys.exit(1)
        #Check if it is a directory or a file
        if os.path.isdir(first_parameter):
            logging.info("       " + tokens[0])
            c_files = SearchCFiles(first_parameter)
            c_files = sorted(c_files)
            for c_file in c_files:
                expanded_list.write(c_file)
                for other_parameter in other_parameters:
                    expanded_list.write(" " + other_parameter.replace("BENCHMARKS\_ROOT", abs_benchmarks_root))
                expanded_list.write("\n")
        else:
            expanded_list.write(first_parameter)
            for other_parameter in other_parameters:
                if ((other_parameter[-2:] == ".c") or (other_parameter[-2:] == ".C") or (other_parameter[-4:] == ".CPP") or (other_parameter[-4:] == ".cpp") or (other_parameter[-4:] == ".cxx") or (other_parameter[-3:] == ".cc") or (other_parameter[-4:] == ".c++") or other_parameter[-4:] == ".xml") and other_parameter[0] != '\\':
                    if other_parameter[0] == '/':
                        expanded_list.write(" " + other_parameter)
                    else:
                        expanded_list.write(" " + os.path.join(abs_benchmarks_root, other_parameter))
                else:
                    expanded_list.write(" " + other_parameter.replace("BENCHMARKS\_ROOT", abs_benchmarks_root).replace("BENCHMARKS_ROOT", abs_benchmarks_root))
            expanded_list.write("\n")
    expanded_list.close()

    #Adding parameters
    logging.info("   Considering all tool arguments")
    arg_lists = args.args
    if not arg_lists:
        arg_lists = [("")]
    arged_list_name = os.path.join(abs_path, "arged_list")
    arged_list = open(arged_list_name, "w")
    lines = open(expanded_list_name).readlines()
    for arg_list in arg_lists:
        for line in lines:
            arged_list.write(line.rstrip())
            if len(arg_list) > 0:
                arg = arg_list[0]
                if arg[0] == "\"":
                    arg = arg[1:-1]
                arged_list.write(" " + arg)
            if args.commonargs != None and len(args.commonargs) > 0:
                for commonarg in args.commonargs:
                    arged_list.write(" " + commonarg[0].replace("#", " "))
            arged_list.write("\n")
    arged_list.close()

    #Name of benchmarks
    full_names = set()


    #Adding benchmark name
    logging.info("   Adding benchmark name")
    named_list_name = os.path.join(abs_path, "named_list")
    named_list = open(named_list_name, "w")
    lines = open(arged_list_name).readlines()
    for line in lines:
        named_list.write(line.rstrip())
        #Retrieve configuration name and benchmark name
        configuration_name = ""
        benchmark_name = ""
        tokens = shlex.split(line)
        for token in tokens:
            if token.find("--configuration-name") != -1:
                configuration_name = token[len("--configuration-name="):]
            if token.find("--benchmark-name") != -1:
                benchmark_name = token[len("--benchmark-name="):]
        if benchmark_name == "":
            if args.tool != "bambu" and args.tool != "zebu":
                logging.error("Missing benchmark name")
                sys.exit(1)
            benchmark_name = os.path.basename(line.split()[0])[:-2]
            named_list.write(" --benchmark-name=" + benchmark_name)
        full_name = configuration_name + ":" + benchmark_name
        logging.info("     " + full_name)
        if full_name in full_names:
            logging.error("Duplicated configuration name - benchmark name: " + full_name)
            sys.exit(1)
        full_names.add(full_name)
        named_list.write("\n")
    named_list.close()


    #Generating folder
    logging.info("   Generating output directories")
    lines = open(named_list_name).readlines()
    for line in lines:
        new_dir = ComputeDirectory(line)
        logging.info("      Creating directory " + new_dir)
        os.makedirs(new_dir)
else:
    logging.info("   Skipping generation of lists and directories")
    named_list_name = os.path.join(abs_path, "named_list")
    if not os.path.exists(named_list_name):
        logging.error("List of previous run not found")
        sys.exit(1)

#Create threads
logging.info("   Launching tool")
lock = threading.RLock()
lock_creation_destruction = threading.RLock()
passed_benchmark = 0
total_benchmark = 0
threads = []
children = [None] * n_jobs
for thread_index in range(n_jobs):
    threads.insert(thread_index, threading.Thread(target=execute_tests, args=(named_list_name, thread_index)))
    threads[thread_index].daemon=True
    threads[thread_index].start()

try:
    #Wait threads
    for thread_index in range(n_jobs):
        while threads[thread_index].isAlive():
            threads[thread_index].join(100)
except KeyboardInterrupt:
    logging.error("SIGINT received")
    failure = True
    for local_thread_index in range(n_jobs):
        if children[local_thread_index] != None:
           if children[local_thread_index].poll() == None:
              try:
                 kill_proc_tree(children[local_thread_index].pid)
              except OSError:
                 pass
    sys.exit(1)

#Collect results
CollectResults(abs_path)

#In case, it create the JUnit file
if args.junitdir != "":
     junit_file = open(junit_file_name, "w")
     junit_file.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
     junit_file.write("<testsuites disabled=\"0\" errors=\"0\" failures=\""+str(total_benchmark - passed_benchmark)+"\" name=\"" + abs_path + "\" tests=\"" + str(total_benchmark) + "\">\n")
     CreateJunitBody(abs_path,junit_file)
     junit_file.write("</testsuites>\n")

#In case, it create the PerfPublisher file
if args.perfpublisherdir != "":
     perfpublisher_file = open(perfpublisher_file_name, "w")
     perfpublisher_file.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
     perfpublisher_file.write("<report name=\""+ perfpublisher_name + "\" categ=\"PandA-Report\">\n")
     CreatePerfPublisherBody(abs_path,perfpublisher_file)
     perfpublisher_file.write("</report>\n")

#Prepare final report
if args.tool == "bambu" or args.tool == "zebu":
    report_file_name = os.path.join(abs_path, "report")
    report_file = open(report_file_name)
    lines = report_file.readlines()
    report_file.close()
    report_file = open(report_file_name, "w")
    command = [tool_exe, "--version"]
    subprocess.call(command, stderr=report_file, stdout=report_file)
    report_file.write("SYSTEM INFORMATION:\n")
    report_file.flush()
    command = ["lsb_release", "-a"]
    subprocess.call(command, stderr=report_file, stdout=report_file)
    report_file.write("\n")
    report_file.write("CURRENT TIME:\n")
    report_file.write(str(datetime.datetime.now())+ "\n\n")
    report_file.write("PASSED TESTS:\n")
    report_file.write(str(passed_benchmark) + "/" + str(total_benchmark) + "\n\n")

    failed_counter_file_name = os.path.join(abs_path, "failed_counter")
    failed_counter_file = open(failed_counter_file_name, "w")
    failed_counter_file.write(str(total_benchmark - passed_benchmark))
    failed_counter_file.close()

    for line in lines:
        report_file.write(line)
    report_file.close()

if args.mail != None:
    outcome = ""
    if args.stop:
        if failure:
            outcome = "FAILURE"
        else:
            outcome = "SUCCESS"
    else:
        outcome = str(passed_benchmark) + "/" + str(total_benchmark)
    full_name = ""
    if len(args.name):
        for name in args.name:
            full_name = full_name + name[0]
    else:
        full_name = "Bambu"
    local_command = "cat " + report_file_name + " | mutt -s \"" + full_name + ": " + outcome + "\" " + args.mail
    subprocess.call(local_command, shell=True)
if failure:
    sys.exit(1)
sys.exit(0)
