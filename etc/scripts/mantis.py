#!/usr/bin/env python3
import argparse
import datetime
import json
import logging
import os
import psutil
import re
import shlex
import shutil
import signal
import subprocess
import sys
import threading
import time

from collections import defaultdict, deque
from concurrent.futures import ThreadPoolExecutor, as_completed
from enum import Enum
import xml.etree.ElementTree as ET


class ExecutionOutcome(Enum):
    SUCCESS = 0
    FAILURE = -1
    TIMEOUT = -2
    CANCELLED = -4
    SKIPPED = 1


def static_vars(**kwargs):
    def decorate(func):
        for k in kwargs:
            setattr(func, k, kwargs[k])
        return func

    return decorate


def parse_arguments():
    def positive_integer(value):
        pos_int = int(value)
        if pos_int <= 0:
            raise argparse.ArgumentTypeError("%s must be a positive integer" % value)
        return pos_int

    class StoreOrUpdateMin(argparse.Action):
        first_parsed = True

        def __call__(self, parser, namespace, values, option_string=None):
            if self.first_parsed == True:
                self.first_parsed = False
                setattr(namespace, self.dest, values)
            else:
                setattr(namespace, self.dest, min(namespace.j, values))

    def parse_timeout(timeout_str):
        """Convert timeout string with units into seconds."""
        match = re.match(r"^(\d+)([smh]?)$", timeout_str.lower())
        if not match:
            raise argparse.ArgumentTypeError(
                "Invalid timeout format. Use a number followed by 's', 'm', or 'h' (e.g., 30s, 5m, 2h)."
            )

        value, unit = match.groups()
        value = int(value)

        if unit == "s" or unit == "":
            return value
        elif unit == "m":
            return value * 60
        elif unit == "h":
            return value * 3600
        else:
            raise argparse.ArgumentTypeError(
                "Unsupported time unit. Use 's' for seconds, 'm' for minutes, or 'h' for hours."
            )

    parser = argparse.ArgumentParser(description="Run commands from a list file.")
    parser.add_argument(
        "files",
        help="Directories or files to be used as list of tests",
        default=[],
        nargs="*",
        action="extend",
    )
    parser.add_argument(
        "-l",
        "--benchmarks-list",
        help="The file containing the list of tests to be performed",
    )
    parser.add_argument(
        "-b", "--benchmarks_root", help="The directory containing benchmarks"
    )
    parser.add_argument(
        "--full-source-name",
        help="Use full relative source path as benchmark name if not specified (default=false)",
        default=False,
        action="store_true",
    )
    parser.add_argument(
        "--no-clean",
        help="Skip command working directory cleanup after execution (default=false)",
        default=False,
        action="store_true",
    )
    parser.add_argument(
        "-s",
        "--skip-list",
        help="Comma-separated list of benchmark names to skip from the generated list",
        default=[],
        type=lambda v: v.split(","),
        nargs="*",
        action="extend",
    )
    parser.add_argument(
        "-t",
        "--timeout",
        help="Timeout for tool execution (default=75m)",
        type=parse_timeout,
        default="75m",
    )
    parser.add_argument(
        "-o",
        "--output",
        help='The directory where output files we be put (default="output")',
        default="output",
    )
    parser.add_argument(
        "-j",
        "--parallel",
        help='Maximum parallel jobs count (default="1")',
        default=int(os.getenv("J", "1")),
        type=positive_integer,
        action=StoreOrUpdateMin,
    )
    parser.add_argument(
        "--restart",
        help="Restart last execution (default=false)",
        default=False,
        action="store_true",
    )
    parser.add_argument(
        "--returnfail",
        help="Return FAILURE in case at least one test fails (default=false)",
        default=False,
        action="store_true",
    )
    parser.add_argument(
        "-a",
        "--args",
        help="A set of arguments defining a run configuration",
        nargs="*",
        action="extend",
    )
    parser.add_argument(
        "-c",
        "--commonargs",
        help="A set of arguments used by all configurations",
        nargs="*",
        action="extend",
    )
    parser.add_argument("--tool", help="The tool to be tested", default="bambu")
    parser.add_argument("--junitdir", help="Set the JUnit directory")
    parser.add_argument(
        "--junit-logtail",
        help="Set the JUnit log tail length for failed tests (default=300)",
        default=300,
        type=positive_integer,
    )
    parser.add_argument(
        "--summary",
        nargs="?",
        help="Merge results in a single XML file (default=summary.xml)",
        default=None,
        const="summary.xml",
    )
    parser.add_argument(
        "--summary-suffix",
        help="Append a suffix to the summary filename for each run",
        default=None,
    )

    args = parser.parse_args()

    if args.benchmarks_list and args.files:
        logging.error(
            "Positional arguments are not allowed when '--benchmark-list' is passed."
        )
        sys.exit(1)

    return args


def getValidExecutable(filename: str):
    if not os.path.isfile(filename) or not os.access(filename, os.X_OK):
        return shutil.which(filename)
    return filename


def is_source_file(s: str):
    ext = os.path.splitext(s)[1].lower()
    source_exts = {
        ".bambuir",
        ".bc",
        ".c",
        ".cc",
        ".cpp",
        ".cxx",
        ".f",
        ".f77",
        ".f90",
        ".f95",
        ".for",
        ".ftn",
        ".ll",
        ".xml",
    }
    return s[0] != "-" and ext in source_exts


def parse_commands(
    command_list: list[str], benchmarks_root: str = ""
) -> list[list[str], dict[str, str]]:
    cmd_list = []
    for cmd_str in command_list:
        cmd_env = defaultdict(str)
        cmds = []
        for cmd in shlex.split(cmd_str):
            cmd = cmd.replace("BENCHMARKS_ROOT", benchmarks_root)
            if cmd.startswith("BENCHMARK_ENV="):
                for var in cmd[14:].split(","):
                    if "=" not in var:
                        raise ValueError(
                            f"Invalid BENCHMARK_ENV entry {var!r} in command: {cmd_str!r}"
                        )
                    var, value = var.split("=", 1)
                    cmd_env[var] = value
            else:
                if not cmd.startswith("/") and is_source_file(cmd):
                    cmd = os.path.abspath(os.path.join(benchmarks_root, cmd))
                cmds.append(cmd)
        cmd_list.append((cmds, cmd_env))
    return cmd_list


def generate_exec_list(
    exec_list,  # : str | list[str]
    matrix_args: list[list[str]],
    common_args: list[str],
    exec_cwd: str,
    outdir: str,
    tool_exe: str = None,
    full_source_name: bool = False,
):

    ### Initialize command list
    command_list = []
    if isinstance(exec_list, str):
        ###  from list file
        with open(exec_list, "r") as file:
            for line in file:
                if line.strip() and line.strip()[0] != "#":
                    command_list.append(line.strip())
    elif isinstance(exec_list, list):
        ###  from command line files
        for e in exec_list:
            be = os.path.join(exec_cwd, e)
            if os.path.exists(be):
                if is_source_file(be):
                    command_list.append(([be], defaultdict(str)))
                elif os.path.isdir(be):
                    for root, _, files in os.walk(be):
                        for file in files:
                            if is_source_file(file):
                                command_list.append(os.path.join(root, file))
                else:
                    logging.warning(f"Unknown file format: {e}")
            else:
                logging.error(f"File or directory not found: {be}")

    if len(command_list) == 0:
        logging.error(f"No commands were provided.")
        sys.exit(1)
    command_list = parse_commands(command_list, exec_cwd)

    ### Parse matrix args
    args_matrix = []
    if matrix_args:
        args_matrix.extend(matrix_args)
    if len(args_matrix) == 0:
        args_matrix.append("")
    args_matrix = parse_commands(args_matrix, exec_cwd)

    ### Add missing configuration names
    is_bambu_exe = os.path.basename(tool_exe) == "bambu"
    if is_bambu_exe:
        for i in range(len(args_matrix)):
            has_config = False
            for arg in args_matrix[i][0]:
                if arg.startswith("--configuration-name"):
                    has_config = True
                    break
            if has_config:
                continue
            args_matrix[i][0].append(f"--configuration-name=conf_{i}")

    ### Parse common args
    common_args = common_args if common_args else []

    ### Generate execution list
    exec_list = []
    exec_counter = 0
    for margs, menv in args_matrix:
        for bargs, benv in command_list:
            cmd_list = [tool_exe] if tool_exe else []
            cmd_list.extend(margs)
            cmd_list.extend(bargs)
            cmd_list.extend(common_args)
            config_name = "no_conf"
            benchmark_name = None
            first_source = None
            for i in range(len(cmd_list)):
                if cmd_list[i].startswith("--benchmark-name"):
                    if len(cmd_list[i]) == 16:
                        benchmark_name = cmd_list[i + 1]
                    else:
                        benchmark_name = cmd_list[i][17:]
                elif cmd_list[i].startswith("--configuration-name"):
                    if len(cmd_list[i]) == 20:
                        config_name = cmd_list[i + 1]
                    else:
                        config_name = cmd_list[i][21:]
                elif first_source is None and is_source_file(cmd_list[i]):
                    if full_source_name:
                        first_source = os.path.splitext(
                            os.path.relpath(cmd_list[i], exec_cwd)
                        )[0].replace(os.path.sep, "_")
                    else:
                        first_source = os.path.splitext(os.path.basename(cmd_list[i]))[
                            0
                        ]
            if benchmark_name is None:
                if first_source is None:
                    benchmark_name = f"exec_{exec_counter}"
                    exec_counter += 1
                else:
                    benchmark_name = first_source
                if is_bambu_exe:
                    cmd_list.append(f"--benchmark-name={benchmark_name}")
            cmd_env = menv | benv
            command_id = f"{config_name}:{benchmark_name}"
            logging.info(f"  {command_id}")
            cmd_cwd = os.path.join(outdir, config_name, benchmark_name)
            if os.path.exists(cmd_cwd):
                logging.error(f"Duplicate test case: {command_id}")
                sys.exit(1)
            os.makedirs(cmd_cwd, exist_ok=True)
            exec_list.append((command_id, cmd_list, cmd_env, cmd_cwd))
    return exec_list


def get_bambu_result(cwd):
    results_xml = os.path.join(cwd, "bambu_results.xml")
    if os.path.exists(results_xml):
        tree = ET.parse(results_xml)
        results = []
        eval_elm = tree.find(".//evaluation")
        runs = tree.findall(".//timing//simulation//run")
        if len(runs) == 0:
            runs = tree.findall(".//timing//evaluation//run")
        if len(runs) > 0:
            run_vals = []
            for r in runs:
                if r is None or r.text is None:
                    continue
                txt = r.text.strip()
                if not txt:
                    continue
                try:
                    run_vals.append(int(txt))
                except Exception:
                    continue
            total = sum(run_vals)
            count = len(run_vals)
            avg = float(total) / float(count) if count else 0.0
            results.append(f"{avg:.1f} CYCLES")
        elif eval_elm is not None and eval_elm.get("CYCLES") is not None:
            results.append(f"{float(eval_elm.get('CYCLES')):.1f} CYCLES")
        slack = None
        if eval_elm is not None:
            slack = eval_elm.get("CLOCK_SLACK")
        if slack is None and (slack_elm := tree.find(".//resources")) is not None:
            slack = slack_elm.get("CLOCK_SLACK") or slack_elm.get("SLACK")
        if slack is not None:
            results.append(f"{slack}ns")
        if len(results) != 0:
            return " *** ".join(results)
    return None


def _parse_bool_env(value: str):
    return str(value).lower() in ("1", "true", "yes", "on")


def _get_cmd_option(commands: list[str], option: str):
    for i, cmd in enumerate(commands):
        if cmd == option:
            return commands[i + 1] if i + 1 < len(commands) else None
        if cmd.startswith(option + "="):
            return cmd[len(option) + 1 :]
    return None


def check_bambu_expectations(cwd: str, env: dict[str, str], commands: list[str], benchmark_name: str):
    expects_loop_ii = env.get("MANTIS_EXPECT_LOOP_II")
    expects_function_ii = env.get("MANTIS_EXPECT_FUNCTION_II")
    expects_function_pipelined = env.get("MANTIS_EXPECT_FUNCTION_PIPELINED")
    if not any((expects_loop_ii, expects_function_ii, expects_function_pipelined)):
        return None

    results_xml = os.path.join(cwd, "bambu_results.xml")
    if not os.path.exists(results_xml):
        return "missing-bambu-results"

    root = ET.parse(results_xml).getroot()
    hls_results = root.find(".//hls_results")
    if hls_results is None:
        return "missing-hls-results"

    function_name = (
        env.get("MANTIS_EXPECT_FUNCTION")
        or _get_cmd_option(commands, "--top-fname")
        or benchmark_name
    )
    function_node = None
    for candidate in hls_results:
        if candidate.get("name") == function_name:
            function_node = candidate
            break
    if function_node is None:
        return f"missing-function:{function_name}"

    if expects_function_pipelined is not None:
        actual = _parse_bool_env(function_node.get("function_pipelined", "0"))
        expected = _parse_bool_env(expects_function_pipelined)
        if actual != expected:
            return f"function-pipelined-mismatch:{function_name}:expected={int(expected)}:got={int(actual)}"

    if expects_function_ii is not None:
        actual = int(function_node.get("function_ii", "0"))
        expected = int(expects_function_ii)
        if actual != expected:
            return f"function-ii-mismatch:{function_name}:expected={expected}:got={actual}"

    if expects_loop_ii is not None:
        expected = int(expects_loop_ii)
        loop_bb = env.get("MANTIS_EXPECT_LOOP_BB")
        loops = [child for child in function_node if child.tag.startswith("loop_")]
        if loop_bb is not None:
            loop_bb = str(loop_bb)
            if loop_bb.upper().startswith("BB"):
                loop_bb = loop_bb[2:]
            loops = [loop for loop in loops if loop.get("bb") == loop_bb]
            if not loops:
                return f"missing-loop:{function_name}:BB{loop_bb}"
        if expected == 0:
            if loops:
                actual_str = ",".join(str(int(loop.get("ii", "0"))) for loop in loops)
                suffix = f":BB{loop_bb}" if loop_bb is not None else ""
                return f"loop-ii-mismatch:{function_name}{suffix}:expected=0:got={actual_str}"
            return None
        if not loops:
            return f"missing-loop:{function_name}"
        actual_values = sorted({int(loop.get("ii", "0")) for loop in loops})
        if expected not in actual_values:
            actual_str = ",".join(str(v) for v in actual_values)
            suffix = f":BB{loop_bb}" if loop_bb is not None else ""
            return f"loop-ii-mismatch:{function_name}{suffix}:expected={expected}:got={actual_str}"

    return None


def kill_process_tree(pid: int):
    """
    Kill a process and all its child processes recursively, with fallback to SIGKILL if SIGTERM times out.

    Args:
       pid (int): The PID of the process to be terminated.
    """
    try:
        parent = psutil.Process(pid)
        children = parent.children(recursive=True)

        # Attempt to terminate the parent process
        try:
            parent.terminate()
            parent.wait(timeout=5)
        except psutil.TimeoutExpired:
            parent.kill()

        # Attempt to terminate orphan children processes
        for child in children:
            try:
                child.terminate()
            except psutil.NoSuchProcess:
                continue

        # Wait for child processes to exit, fallback to kill if timeout
        _, still_alive = psutil.wait_procs(children, timeout=5)
        for child in still_alive:
            try:
                child.kill()
            except psutil.NoSuchProcess:
                continue

    except psutil.NoSuchProcess:
        print(f"No process with PID {pid} exists.")
    except Exception as e:
        print(f"An error occurred: {e}")


@static_vars(stats=threading.Lock(), queued=0, failures=0, successes=0)
def log_execution(outcome: ExecutionOutcome, message: str, result=None):
    with log_execution.stats:
        log_execution.queued -= 1

        prefix = ""
        if outcome == ExecutionOutcome.SUCCESS:
            log_execution.successes += 1
            prefix = "SUCCESS"
        elif outcome == ExecutionOutcome.SKIPPED:
            log_execution.successes += 1
            prefix = "SKIPPED"
        else:
            log_execution.failures += 1
            prefix = "FAILURE"

        if result:
            prefix += f" ({result})"
        logging.info(
            f"   {prefix} --- OVERALL: {log_execution.successes} passed, {log_execution.failures} failed, {log_execution.queued} queued --- {message}"
        )


@static_vars(
    cancel_event=threading.Event(),
    condition=threading.Condition(),
    get_result=lambda cwd: None,
    skip_list=[],
    clean_cwd=True,
    skip_clean={"execution.log", "timeout.log", "failure.log", "return_value"},
    cwd_wait_timeout=5.0,
    cwd_wait_interval=0.1,
)
def run_command(
    id: str, commands: list[str], env: dict[str, str], cwd: str, timeout: int
):
    """Run a shell command, handle early abortion, and enforce a timeout."""

    cid, bid = id.split(":")
    output_file = os.path.join(cwd, "execution.log")
    timeout_ln = os.path.join(os.path.dirname(output_file), "timeout.log")
    failure_ln = os.path.join(os.path.dirname(output_file), "failure.log")
    retval_file = os.path.join(cwd, "return_value")
    log_message = ""
    if len(env) > 0:
        log_message += "["
        log_message += ", ".join([f"{key}='{val}'" for key, val in env.items()])
        log_message += "] "
    log_message += " ".join(commands[1:])
    expect_failure = str(env.get("MANTIS_EXPECT_FAILURE", "")).lower() in (
        "1",
        "true",
        "yes",
        "on",
    )
    expect_failure_regex = env.get("MANTIS_EXPECT_FAILURE_REGEX", "")

    ### User-required abort
    with run_command.condition:
        if run_command.cancel_event.is_set():
            log_execution(ExecutionOutcome.CANCELLED, log_message)
            return id, cwd, ExecutionOutcome.CANCELLED

    ### User-required skip
    if (
        id in run_command.skip_list
        or cid + ":" in run_command.skip_list
        or bid in run_command.skip_list
    ):
        log_execution(ExecutionOutcome.SKIPPED, log_message, "skip-list")
        return id, cwd, ExecutionOutcome.SKIPPED

    ### Already executed succesfully
    if os.path.exists(retval_file) and open(retval_file, "r").read() == "0\n":
        expectation_result = check_bambu_expectations(cwd, env, commands, bid)
        if expectation_result is None:
            old_result = run_command.get_result(cwd)
            log_execution(ExecutionOutcome.SKIPPED, log_message, old_result)
            return id, cwd, ExecutionOutcome.SKIPPED

    deadline = time.time() + run_command.cwd_wait_timeout
    while not os.path.isdir(cwd):
        if time.time() >= deadline:
            reason = "cwd-not-ready"
            logging.error(f"Working directory not ready for {id}: {cwd}")
            with open(failure_ln, "w") as f:
                f.write("LAUNCH ERROR: working directory not ready\n")
            with open(retval_file, "w") as f:
                f.write("-1\n")
            log_execution(ExecutionOutcome.FAILURE, log_message, reason)
            return id, cwd, ExecutionOutcome.FAILURE
        time.sleep(run_command.cwd_wait_interval)

    for e in os.scandir(cwd):
        if os.path.isdir(e):
            shutil.rmtree(e)
        else:
            os.remove(e)

    with open(output_file, "w") as output:
        for key, val in env.items():
            output.write(f"export {key}='{val}'; ")
        output.write(" ".join(f"'{cmd}'" for cmd in commands) + "\n")
        output.write("#" * 74 + "\n")
        output.flush()
        try:
            process = subprocess.Popen(
                commands,
                env=os.environ | env,
                cwd=cwd,
                stdout=output,
                stderr=subprocess.STDOUT,
                start_new_session=True,
            )
        except Exception as e:
            with open(failure_ln, "w") as f:
                f.write(f"LAUNCH ERROR: {e}\n")
            with open(retval_file, "w") as f:
                f.write("-1\n")
            log_execution(ExecutionOutcome.FAILURE, log_message, "launch-error")
            return id, cwd, ExecutionOutcome.FAILURE

        outcome = ExecutionOutcome.SUCCESS
        result = None

        def monitor_process():
            nonlocal outcome
            """Monitor the process and notify when it completes."""
            try:
                process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                outcome = ExecutionOutcome.TIMEOUT
                kill_process_tree(process.pid)
            finally:
                with run_command.condition:
                    run_command.condition.notify_all()

        monitor_thread = threading.Thread(target=monitor_process)
        monitor_thread.start()

        try:
            with run_command.condition:
                while process.poll() is None:
                    if run_command.cancel_event.is_set():
                        kill_process_tree(process.pid)
                        outcome = ExecutionOutcome.CANCELLED
                        break
                    run_command.condition.wait()  # Wait for process completion or cancellation

            output.write("\n" + "#" * 74 + "\n")
            if outcome == ExecutionOutcome.SUCCESS:
                return_value = process.returncode
                output.write(f"RETURN VALUE: {return_value}\n")
                with open(retval_file, "w") as file:
                    file.write(f"{return_value}\n")
                if return_value != 0:
                    if expect_failure:
                        if expect_failure_regex:
                            output.flush()
                            with open(output_file, "r") as log_file:
                                log_text = log_file.read()
                            if re.search(expect_failure_regex, log_text, re.MULTILINE):
                                result = "expected-failure"
                            else:
                                outcome = ExecutionOutcome.FAILURE
                                result = "missing-expected-failure-log"
                                os.symlink(os.path.basename(output_file), failure_ln)
                        else:
                            result = "expected-failure"
                    else:
                        outcome = ExecutionOutcome.FAILURE
                        if return_value < 0:
                            try:
                                _sig_name = signal.Signals(-return_value).name
                            except Exception:
                                _sig_name = f"SIG{-return_value}"
                            result = f"crash: {_sig_name}"
                        else:
                            result = f"code: {return_value}"
                        os.symlink(os.path.basename(output_file), failure_ln)
                else:
                    if expect_failure:
                        outcome = ExecutionOutcome.FAILURE
                        result = "expected-failure-not-triggered"
                        os.symlink(os.path.basename(output_file), failure_ln)
                    else:
                        expectation_result = check_bambu_expectations(cwd, env, commands, bid)
                        if expectation_result is not None:
                            outcome = ExecutionOutcome.FAILURE
                            result = expectation_result
                            os.symlink(os.path.basename(output_file), failure_ln)
                        else:
                            result = run_command.get_result(cwd)
            elif outcome == ExecutionOutcome.TIMEOUT:
                output.write("PROCESS TIMEOUT\n")
                outcome = ExecutionOutcome.FAILURE
                result = "Timeout"
                os.symlink(os.path.basename(output_file), timeout_ln)
            elif outcome == ExecutionOutcome.CANCELLED:
                output.write("PROCESS CANCELLED\n")
                outcome = ExecutionOutcome.FAILURE
                result = "Cancelled"

            log_execution(outcome, log_message, result)
            if run_command.clean_cwd:
                for relpath in os.listdir(cwd):
                    if relpath not in run_command.skip_clean:
                        path = os.path.join(cwd, relpath)
                        if os.path.isdir(path):
                            shutil.rmtree(path)
                        else:
                            os.remove(path)
        except Exception as e:
            logging.exception(f"Execution error in run_command for {id}: {e}")
            try:
                with open(failure_ln, "w") as f:
                    f.write(f"RUNTIME ERROR: {e}\n")
                with open(retval_file, "w") as f:
                    f.write("-1\n")
            except Exception:
                pass
            log_execution(ExecutionOutcome.FAILURE, log_message, "runtime-error")
        finally:
            monitor_thread.join()
    return (id, cwd, outcome)


def main():
    args = parse_arguments()

    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s [%(levelname)-8s] %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    )

    ### Verify arguments
    outdir = args.output
    if not args.restart and os.path.exists(outdir):
        logging.error(
            f"Output directory {outdir} already exists. Please remove it or specify a different one."
        )
        sys.exit(1)

    benchmarks_root = args.benchmarks_root if args.benchmarks_root else os.path.curdir
    if args.benchmarks_list:
        exec_list = args.benchmarks_list
        if not os.path.exists(exec_list):
            exec_list = os.path.join(benchmarks_root, exec_list)
            if not os.path.exists(exec_list):
                logging.error(f"Benchmarks list file not found: {args.benchmarks_list}")
                sys.exit(1)
    else:
        exec_list = args.files
    matrix_args = args.args
    common_args = args.commonargs
    command_timeout = args.timeout

    tool_exe = getValidExecutable(args.tool)
    if tool_exe is None:
        logging.error(f"Executable {args.tool} not found or invalid")
        sys.exit(1)

    junit_report_filename = os.path.join(
        args.junitdir if args.junitdir else outdir, "test_report.xml"
    )
    os.makedirs(os.path.dirname(junit_report_filename), exist_ok=True)

    retval = 0

    ### Generate execution list
    logging.info("Preparing benchmark list:")
    exec_list_filename = os.path.join(outdir, "exec_list.json")
    if args.restart:
        logging.warning(
            "Restarted test suites will ignore all arguments for the test command line generation (args, commonargs, benchmarks-root, benchmarks-list, skip-list)"
        )
        if not os.path.exists(exec_list_filename):
            logging.error(
                "Execution list from previous run not found, restart not possible."
            )
            sys.exit(1)
        with open(exec_list_filename, "r") as file:
            exec_list = json.load(file)
        for id, _, env, cwd in exec_list:
            logging.info(f"  {id}")
    else:
        exec_list = generate_exec_list(
            exec_list,
            matrix_args,
            common_args,
            benchmarks_root,
            outdir,
            tool_exe,
            args.full_source_name,
        )
        with open(exec_list_filename, "w") as file:
            json.dump(exec_list, file, indent="\t")

    ### Testsuite report XML
    testsuites = ET.Element(
        "testsuites",
        {
            "disabled": "0",
            "tests": str(len(exec_list)),
            "failures": str(log_execution.failures),
            "name": os.path.basename(outdir),
        },
    )

    def update_report(id: str, cwd: str, outcome: ExecutionOutcome):
        nonlocal retval

        conf, bench = id.split(":")
        testsuite = testsuites.find(f".//testsuite[@name='{conf}']")
        if testsuite is None:
            testsuite = ET.SubElement(
                testsuites,
                "testsuite",
                {"disabled": "0", "name": conf, "tests": "0", "failures": "0"},
            )
        testsuite.set("tests", str(int(testsuite.get("tests")) + 1))
        execution_log_filename = os.path.join(cwd, "execution.log")
        testcase = ET.SubElement(
            testsuite,
            "testcase",
            {
                "name": bench,
                "command": open(execution_log_filename, "r").readline().strip(),
            },
        )
        if outcome != ExecutionOutcome.SKIPPED and outcome != ExecutionOutcome.SUCCESS:
            retval = 1
            testsuite.set("failures", str(int(testsuite.get("failures")) + 1))
            ET.SubElement(
                testcase,
                "failure",
                {
                    "type": (
                        "FAILURE" if outcome == ExecutionOutcome.FAILURE else "TIMEOUT"
                    )
                },
            )
            logtail = ET.SubElement(testcase, "stdout")
            if os.path.exists(execution_log_filename):
                logtail.text = "\n"
                with open(execution_log_filename, "r") as log:
                    for line in deque(log, maxlen=args.junit_logtail):
                        logtail.text += line
            else:
                logtail.text = "Missing execution log"

    ### Execute commands
    logging.info(f"Launching tool {tool_exe}")
    log_execution.queued = len(exec_list)
    run_command.skip_list = args.skip_list
    run_command.clean_cwd = not args.no_clean
    if os.path.basename(tool_exe) == "bambu":
        run_command.get_result = get_bambu_result
        run_command.skip_clean.add("bambu_results.xml")
    elif os.path.basename(tool_exe) == "eucalyptus":
        run_command.get_result = get_bambu_result
        run_command.skip_clean.add("bambu_results.xml")
        run_command.skip_clean.add("characterization.xml")

    # Be robust to user-defined signals used by underlying tools (e.g., SIGUSR1/SIGPIPE)
    for _sig_name in ("SIGUSR1", "SIGPIPE"):
        _sig = getattr(signal, _sig_name, None)
        if _sig is not None:
            try:
                signal.signal(_sig, lambda s, f, name=_sig_name: logging.warning(f"Ignoring {name} received"))
            except Exception:
                pass

    logging.info("  Press Ctrl+C to abort...")
    with ThreadPoolExecutor(max_workers=args.parallel) as executor:
        fut2meta = {}
        for id, cmd, env, cwd in exec_list:
            fut = executor.submit(run_command, id, cmd, env, cwd, command_timeout)
            fut2meta[fut] = (id, cwd)
        try:
            for fut in as_completed(fut2meta):
                id, cwd = fut2meta[fut]
                try:
                    _id, _cwd, outcome = fut.result()
                except Exception as e:
                    logging.exception(f"Worker crashed for {id}: {e}")
                    fail_ln = os.path.join(cwd, "failure.log")
                    with open(fail_ln, "w") as f:
                        f.write(f"INFRASTRUCTURE CRASH: {e}\n")
                    update_report(id, cwd, ExecutionOutcome.FAILURE)
                    continue
                update_report(_id, _cwd, outcome)
        except KeyboardInterrupt:
            logging.info("KeyboardInterrupt received, notifying abortion...")
            retval = 1
            run_command.cancel_event.set()
            with run_command.condition:
                run_command.condition.notify_all()  # Wake up all waiting threads
            try:
                executor.shutdown(wait=False, cancel_futures=True)
            except TypeError:
                executor.shutdown(wait=False)

    ### Create execution report
    ET.indent(testsuites)
    ET.ElementTree(testsuites).write(junit_report_filename, encoding="utf-8")

    ### Gather results
    if args.summary:
        if os.path.basename(tool_exe) != "bambu":
            logging.error("Results collection is only available for Bambu HLS.")
            sys.exit(1)

        spider_py = os.path.abspath(os.path.join(os.path.dirname(__file__), "spider.py"))
        if not os.path.exists(spider_py):
            logging.error(f"spider.py not found at {spider_py}")
            sys.exit(-1)

        # Initialize res_file with the default summary path
        res_file = os.path.abspath(args.summary)
        
        # Apply suffix if specified
        if args.summary_suffix:
            # Parse the summary filename and append suffix
            summary_dir = os.path.dirname(args.summary)
            summary_name = os.path.basename(args.summary)
            # Replace extension with suffix if needed, or append suffix before extension
            if summary_name.endswith('.xml'):
                summary_name = summary_name[:-4] + args.summary_suffix + '.xml'
            else:
                summary_name = summary_name + args.summary_suffix
            res_file = os.path.abspath(os.path.join(summary_dir, summary_name))
        logging.info(f"Collect results to {res_file}")
        subprocess.call([sys.executable, spider_py, "-i", outdir, "-o", res_file], env=os.environ)

    if args.returnfail:
        sys.exit(retval)


if __name__ == "__main__":
    main()
