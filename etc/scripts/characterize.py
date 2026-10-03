#!/usr/bin/env python3
import argparse
import logging
import os
import shlex
import subprocess
import sys
from typing import List
import xml.etree.ElementTree as ET

def find_device_seed(device_name: str) -> str | None:
    vendor_dirs_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../etc/libtech"))
    for root, _, files in os.walk(vendor_dirs_root):
        for name in files:
            if name.endswith(device_name + "-seed.xml"):
                return os.path.join(root, name)
    return None


def generate_evaluation_suite(devices: List[str], tech_libraries: List[str], update_list: List[str] | None):
    device_files = []
    # Computing device file
    for device in devices:
        device_file = find_device_seed(device)
        if device_file is None:
            logging.error(f"seed file for {device} not found")
            sys.exit(-1)
        device_files.append(device_file)

    # 1. Load all component definitions
    components = {}
    
    # Load from technology files first
    for tech_file in tech_libraries:
        if os.path.exists(tech_file):
            try:
                tree = ET.parse(tech_file)
                root = tree.getroot()
                for comp in root.findall('.//cell') + root.findall('.//template'):
                    name_tag = comp.find('name')
                    if name_tag is not None and name_tag.text:
                        components[name_tag.text] = comp
            except ET.ParseError as e:
                logging.error(f"Failed to parse XML file: {tech_file} - {e}")
                sys.exit(-1)

    # Load from device files, overwriting
    for device_file in device_files:
        if os.path.exists(device_file):
            try:
                tree = ET.parse(device_file)
                root = tree.getroot()
                for comp in root.findall('.//cell') + root.findall('.//template'):
                    name_tag = comp.find('name')
                    if name_tag is not None and name_tag.text:
                        components[name_tag.text] = comp
            except ET.ParseError as e:
                logging.error(f"Failed to parse XML file: {device_file} - {e}")
                sys.exit(-1)

    # 2. Filter components to characterize
    fus_to_characterize = []
    if update_list is not None:
        for comp_name in sorted(list(components.keys())):
            if comp_name in update_list:
                fus_to_characterize.append(comp_name)
    else:
        fus_to_characterize = sorted(list(components.keys()))

    # 3. Generate characterization commands
    for fu_name in fus_to_characterize:
        component = components[fu_name]
        char_signatures = []

        # Handle <cell>
        if component.tag == 'cell':
            if component.find('component_timing_alias') is not None:
                continue
            instance_name = fu_name
            char_signatures.append(f"{fu_name}-{instance_name}")

        # Handle <template>
        elif component.tag == 'template':
            prec_values = [1, 8, 16, 32, 64]
            inputs = [p for p in component.findall('.//port_o') if p.get('id', '').startswith('in')]
            num_inputs = len(inputs)

            op = component.find('operation')
            is_commutative = op is not None and op.get('commutative') == '1'
            no_const_char = component.find('no_constant_characterization') is not None

            pipe_params_str = op.get('pipe_parameters', '') if op is not None else ''
            pipe_params = {}
            if pipe_params_str:
                try:
                    if pipe_params_str.startswith('*:'):
                        _, values_str = pipe_params_str.split(':', 1)
                        pipe_values = [int(v) for v in values_str.split(',')]
                        for p in prec_values:
                            pipe_params[p] = pipe_values
                    else:
                        for part in pipe_params_str.split('|'):
                            if ':' in part:
                                prec, values = part.split(':', 1)
                                if prec != "DSPs_y_sizes":
                                    pipe_params[int(prec)] = [int(v) for v in values.split(',')]
                except (ValueError, IndexError) as e:
                    logging.warning(f"Could not parse pipe_parameters for {fu_name}: '{pipe_params_str}' - {e}")

            for prec in prec_values:
                base_instance_parts = [str(prec)] * num_inputs
                if fu_name.startswith("widen") or fu_name.startswith("ui_widen"):
                    base_instance_parts.append(str(prec * 2))
                else:
                    base_instance_parts.append(str(prec))
                
                # Regular characterization
                instance_name_base = f"{fu_name}_{'_'.join(base_instance_parts)}"
                if prec in pipe_params:
                    for pipe_val in pipe_params[prec]:
                        char_signatures.append(f"{fu_name}-{instance_name_base}_{pipe_val}")
                else:
                    char_signatures.append(f"{fu_name}-{instance_name_base}")

                # Constant characterization
                if not no_const_char:
                    num_positions_to_zero = num_inputs
                    if is_commutative and num_inputs > 0:
                        num_positions_to_zero = 1
                    
                    for i in range(num_positions_to_zero):
                        const_instance_parts = list(base_instance_parts)
                        const_instance_parts[i] = '0'
                        instance_name_base_const = f"{fu_name}_{'_'.join(const_instance_parts)}"
                        
                        if prec in pipe_params:
                            for pipe_val in pipe_params[prec]:
                                char_signatures.append(f"{fu_name}-{instance_name_base_const}_{pipe_val}")
                        else:
                            char_signatures.append(f"{fu_name}-{instance_name_base_const}")
        
        # Write lines for all devices
        for device_file in device_files:
            device_name = os.path.basename(device_file)[:-9] # strip -seed.xml
            for component_signature in char_signatures:
                yield f"--target-datafile={device_file} --characterize={component_signature} --benchmark-name={component_signature} --configuration-name={device_name}\n"


def main():
    parser = argparse.ArgumentParser(
        description="Characterize device", fromfile_prefix_chars='@')
    parser.add_argument("--devices", help="The devices to be characterized")
    parser.add_argument("--technology-files", help="The technology files containing the components")
    parser.add_argument(
        "--list-only", help="Only generate the list of characterizations (default=\"eval_suite_list\").", default="")
    parser.add_argument(
        "--from-list", help="Perform characterization of components from previously generated list", default="")
    parser.add_argument("--update", help="The components whose characterizations have to be updated")
    parser.add_argument("--restart", help="Restart last execution (default=false)",
                        default=False, action="store_true")
    parser.add_argument(
        '-o', "--output", help="The directory where output files we be put (default=\"output\")", default="output")

    args, mantis_args = parser.parse_known_args()

    logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')

    # Create the folder and enter in it
    out_path = os.path.abspath(args.output)

    # Check if output directory exists, if yes abort
    if os.path.exists(args.output) and not args.restart:
        logging.error(f"Output directory {args.output} already exists. Please remove it or specify a different one with -o")
        sys.exit(-1)
    os.makedirs(out_path, exist_ok=True)

    eval_suite_list = os.path.join(out_path, "eval_suite_list")
    if args.list_only != "":
        eval_suite_list = args.list_only

    # Check if device FUs list exists, if yes abort
    if os.path.exists(eval_suite_list) and not args.restart:
        logging.error(f"Device FUs list {eval_suite_list} already exists. Please remove it.")
        sys.exit(-1)

    if args.from_list != "":
        eval_suite_list = args.from_list
        if not os.path.exists(eval_suite_list):
            logging.error(f"Device FUs list {eval_suite_list} not found.")
            sys.exit(-1)


    if args.from_list == "":
        if args.technology_files == None:
            logging.error("Missing technology files")
            sys.exit(-1)

        if args.devices == None:
            logging.error("Missing devices")
            sys.exit(-1)

        devices = args.devices.split(",")
        tech_libraries = args.technology_files.split(',')
        update_list = args.update.split(',') if args.update is not None else None

        with open(eval_suite_list, "w") as suite_list:
            for command in generate_evaluation_suite(devices, tech_libraries, update_list):
                suite_list.write(command)


    if args.list_only != "":
        sys.exit(0)

    eval_suite_dir = os.path.join(args.output, "eval")
    mantis_py = os.path.abspath(os.path.join(os.path.dirname(__file__), "mantis.py"))
    if not os.path.exists(mantis_py):
        logging.error(f"mantis.py not found at {mantis_py}")
        sys.exit(-1)
    mantis_cmd = [
        sys.executable,
        mantis_py,
        "--tool", "eucalyptus",
        f'--output={eval_suite_dir}',
        "-l", eval_suite_list,
    ]
    if args.restart:
        mantis_cmd.append("--restart")
    mantis_cmd.extend(mantis_args)
    logging.info("Prepared mantis command: " + " ".join(shlex.quote(c) for c in mantis_cmd))

    try:    
        rc = subprocess.call(mantis_cmd, env=os.environ)
        if rc != 0:
            logging.error(f"mantis.py exited with return code {rc}")
            sys.exit(rc)
    except Exception as e:
        logging.error(f"Failed to invoke mantis.py: {e}")
        sys.exit(-1)

    spider_py = os.path.abspath(os.path.join(os.path.dirname(__file__), "spider.py"))
    if not os.path.exists(spider_py):
        logging.error(f"spider.py not found at {spider_py}")
        sys.exit(-1)

    # Generate technology files
    for device_folder_name in os.listdir(eval_suite_dir):
        device_folder_path = os.path.join(eval_suite_dir, device_folder_name)
        if not os.path.isdir(device_folder_path):
            continue

        device_name = os.path.basename(device_folder_name)
        out_file = os.path.join(out_path, f'{device_name}.xml')
        logging.info(f"Aggregating results for {device_name} into {out_file}")

        spider_cmd = [
            sys.executable, 
            spider_py,
            "--mode", "eucalyptus",
            "-o", out_file,
            "-i", device_folder_path,
        ]
        logging.info("Prepared spider command: " + " ".join(shlex.quote(c) for c in spider_cmd))
        try:    
            rc = subprocess.call(spider_cmd, env=os.environ)
            if rc != 0:
                logging.error(f"spider.py exited with return code {rc}")
                sys.exit(rc)
        except Exception as e:
            logging.error(f"Failed to invoke spider.py: {e}")
            sys.exit(-1)

if __name__ == "__main__":
    main()
