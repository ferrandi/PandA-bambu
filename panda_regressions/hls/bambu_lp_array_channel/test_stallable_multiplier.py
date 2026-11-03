#!/usr/bin/env python3
"""Cycle-accurate checks for the stallable multiplier templates in C_HLS_IPs.xml."""

import os
from concurrent.futures import ThreadPoolExecutor
from copy import deepcopy
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

from test_timed_capture import resolve_parallelism


ROOT = Path(__file__).resolve().parents[3]
MODEL_PARAMS = (0, 1, 2, 3, 7)


def templates(path):
    root = ET.parse(ROOT / path).getroot()
    return {node.findtext("name"): node for node in root.iter("template")}


def functionality(template, key):
    model = template.find(".//NP_functionality")
    if model is None or model.get(key) is None:
        raise RuntimeError(f"Missing {key} model in template {template.findtext('name')}")
    return model.get(key)


def assert_stallable_contract(template):
    name = template.findtext("name")
    attribute = template.find("attribute[@name='Stallable'][@value_type='boolean']")
    if attribute is None or (attribute.text or "").strip().lower() not in {"1", "true"}:
        raise RuntimeError(f"{name} does not declare Stallable=true")
    module = template.find(".//module_o")
    module_name = module.get("id") if module is not None else name
    ports = module.findall("port_o") if module is not None else []
    port_names = [port.get("id") for port in ports]
    stall = module.find("port_o[@id='stall_port']") if module is not None else None
    type_desc = stall.find("structural_type_descriptor") if stall is not None else None
    if (stall is None or stall.get("dir") != "IN" or type_desc is None or type_desc.get("type") != "BOOL" or
            type_desc.get("size") != "1" or int(type_desc.get("vector_size", "0")) != 0):
        raise RuntimeError(f"{name} must expose a scalar one-bit BOOL input named stall_port")

    controls = [port for port in ("clock", "reset", "start_port") if port in port_names]
    stall_index = port_names.index("stall_port")
    if port_names[:stall_index] != controls or any(
            port not in port_names[stall_index + 1:] for port in ("in1", "in2")):
        raise RuntimeError(f"{name} must order controls before stall_port and stall_port before in1/in2")

    model = module.find("NP_functionality") if module is not None else None
    expected_library_tokens = [module_name] + [
        port for port in port_names if port not in {"clock", "reset", "start_port", "stall_port"}
    ] + ["PIPE_PARAMETER"]
    if model is None or model.get("LIBRARY", "").split() != expected_library_tokens:
        raise RuntimeError(f"{name} LIBRARY model must list only parametrized data ports")


LATTICE_DEVICES = (
    "LFE335EA8FN484C",
    "LFE5U85F8BG756C",
    "LFE5UM85F8BG756C",
)


def assert_lattice_stallable_contract():
    for device in LATTICE_DEVICES:
        seed = templates(f"etc/libtech/lattice/{device}-seed.xml")
        characterized = templates(f"etc/libtech/lattice/{device}.xml")
        for name in ("mul_node_FU", "ui_mul_node_FU"):
            pair = (seed[name], characterized[name])
            for template in pair:
                assert_stallable_contract(template)
                model = template.find(".//NP_functionality")
                if model is None or f"{name} in1 in2 out1 PIPE_PARAMETER" != model.get("LIBRARY"):
                    raise RuntimeError(f"{device} {name} LIBRARY model must omit non-parameterized stall_port")
                body = model.get("VERILOG_PROVIDED", "")
                for ce in range(4):
                    if not re.search(r"\.CE" + str(ce) + r"\(\s*~stall_port\s*\)", body):
                        raise RuntimeError(f"{device} {name} does not gate CE{ce} with stall")
                register_selectors = re.findall(r'\.pmi_reg_(?:inputa|inputb|pipeline|output|signeda|signedb)_ce\("(CE[0-3])"\)', body)
                if len(register_selectors) != 6 or set(register_selectors) != {"CE0"}:
                    raise RuntimeError(f"{device} {name} PMI register CEs are not all controlled by CE0")
            # The seed and characterized forms must expose the same port and PMI behavior.
            for template in pair:
                assert_stallable_contract(template)
            seed_port = seed[name].find(".//port_o[@id='stall_port']/structural_type_descriptor")
            full_port = characterized[name].find(".//port_o[@id='stall_port']/structural_type_descriptor")
            if seed_port.attrib != full_port.attrib:
                raise RuntimeError(f"{device} {name} seed and characterized stall port differ")
            seed_model = seed[name].find(".//NP_functionality")
            full_model = characterized[name].find(".//NP_functionality")
            if (seed_model.get("LIBRARY"), seed_model.get("VERILOG_PROVIDED", "").split()) != \
                    (full_model.get("LIBRARY"), full_model.get("VERILOG_PROVIDED", "").split()):
                raise RuntimeError(f"{device} {name} seed and characterized PMI models differ")


def run_lattice_device_resolution_probe(root_dir, device):
    bambu = shutil.which("bambu")
    if not bambu:
        raise RuntimeError("bambu is required for Lattice device resolution probes")
    with tempfile.TemporaryDirectory(prefix="lattice-device-resolution-", dir=root_dir) as temp:
        work = Path(temp)
        source = work / "vector_mul.cpp"
        source.write_text("""typedef int v2si __attribute__((vector_size(8)));
void vector_mul(int a, int b, int *out) {
   v2si va = {a, a + 1};
   v2si vb = {b, b + 1};
   v2si vc = va * vb;
   out[0] = vc[0];
   out[1] = vc[1];
}
""")
        output_dir = work / device
        output_dir.mkdir()
        env = os.environ.copy()
        env["TMPDIR"] = str(work)
        env["TMP"] = env["TMPDIR"]
        env["TEMP"] = env["TMPDIR"]
        run([bambu, "--generate-interface=INFER", str(source), "--top-fname=vector_mul",
             f"--benchmark-name=lattice_stall_{device}", "--clock-period=5",
             f"--device-name={device}", f"--output-directory={output_dir}"], work, env)
        rtl = "\n".join(path.read_text(encoding="utf-8", errors="replace")
                          for path in output_dir.rglob("*.v"))
        if "ui_vec_mul_node_FU" not in rtl or "ui_mul_node_FU" not in rtl:
            raise RuntimeError(f"{device} vector multiplier RTL did not resolve a stallable unsigned multiplier")
        if not re.search(r"\.stall_port\s*\(\s*selector_stallable_fus\s*\)", rtl) or \
                not re.search(r"\.stall_port\s*\(\s*stall_port\s*\)", rtl):
            raise RuntimeError(f"{device} vector multiplier hierarchy does not propagate the FSM stall")
        for ce in range(4):
            if not re.search(r"\.CE" + str(ce) + r"\(\s*~stall_port\s*\)", rtl):
                raise RuntimeError(f"{device} generated scalar PMI model does not gate CE{ce}")


def lattice_widening_cases(c_hls, bin_dir):
    cases = []
    for device in LATTICE_DEVICES:
        device_templates = templates(f"etc/libtech/lattice/{device}.xml")
        for name, scalar_name, is_signed in (
                ("widen_mul_node_FU", "mul_node_FU", True),
                ("ui_widen_mul_node_FU", "ui_mul_node_FU", False)):
            wrapper = c_hls[name]
            scalar = device_templates[scalar_name]
            component = functionality(wrapper, "VERILOG_PROVIDED")
            model = lattice_verilog_model(scalar_name, scalar, is_signed)
            model += "\n" + verilog_model(name, component, is_signed)
            model_name = f"{device}_{name}"
            tb = verilog_testbench(name, is_signed)
            cases.append((model_name,
                          lambda work, model_name=model_name, model=model, tb=tb:
                              simulate_model(model_name, model, tb, MODEL_PARAMS, "verilog", bin_dir, work)))
    return cases


def write_stall_port_overlay(template, path):
    technology = ET.Element("technology")
    library = ET.SubElement(technology, "library")
    ET.SubElement(library, "name").text = "STD_FU"
    library.append(deepcopy(template))
    ET.ElementTree(technology).write(path, encoding="utf-8", xml_declaration=True)


def run_bad_port_probe(template, mutation, root_dir):
    bambu = shutil.which("bambu")
    if not bambu:
        raise RuntimeError("bambu is required for the malformed Stallable port probes")
    with tempfile.TemporaryDirectory(prefix=f"bad-stall-{mutation}-", dir=root_dir) as temp:
        work = Path(temp)
        overlay = deepcopy(template)
        stall = overlay.find(".//module_o/port_o[@id='stall_port']")
        if mutation == "missing":
            overlay.find(".//module_o").remove(stall)
        elif mutation == "wide":
            stall.find("structural_type_descriptor").set("size", "2")
        else:
            raise ValueError(f"unknown malformed port case: {mutation}")
        overlay_path = work / "malformed_stall.xml"
        write_stall_port_overlay(overlay, overlay_path)
        output_dir = work / "output"
        output_dir.mkdir()
        env = os.environ.copy()
        env["TMPDIR"] = str(work)
        env["TMP"] = env["TMPDIR"]
        env["TEMP"] = env["TMPDIR"]
        result = subprocess.run(
            [bambu, "--configuration-name=LP-ARRAY-CHANNEL-MODELSIM", "--generate-interface=INFER",
             str(ROOT / "panda_regressions/hls/bambu_lp_array_channel/lp_mul_array.cpp"),
             "--top-fname=mul_stage", f"--benchmark-name=bad_stall_{mutation}", "--clock-period=5",
             "--device-name=xc7z020,-1,clg484", f"--output-directory={output_dir}", str(overlay_path)],
            cwd=work, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False,
        )
        diagnostic = result.stdout
        module_name = template.findtext("name")
        rejected_by_stall_check = "Stallable functional unit" in diagnostic and \
            "scalar one-bit BOOL input named stall_port" in diagnostic
        if result.returncode == 0 or not rejected_by_stall_check:
            raise RuntimeError(
                f"Bambu did not reject the {mutation} stall port with an expected diagnostic "
                f"(status {result.returncode}):\n{diagnostic}"
            )


def run_stall_port_order_probe(template, placement, root_dir):
    runner = ROOT / "panda_regressions/hls/bambu_lp_array_channel/run_case.sh"
    if not runner.is_file() or not shutil.which("bambu"):
        raise RuntimeError("run_case.sh and bambu are required for Stallable port-order probes")
    if placement not in {"before", "between"}:
        raise ValueError(f"unknown stall port placement: {placement}")
    with tempfile.TemporaryDirectory(prefix=f"stall-order-{placement}-", dir=root_dir) as temp:
        work = Path(temp)
        overlay = deepcopy(template)
        module = overlay.find(".//module_o")
        stall = module.find("port_o[@id='stall_port']")
        if stall is None:
            raise RuntimeError("Stallable multiplier template has no stall port")
        target = module.find("port_o[@id='in1']" if placement == "before" else "port_o[@id='in2']")
        if target is None:
            raise RuntimeError(f"Stallable multiplier template has no target operand for {placement} placement")
        module.remove(stall)
        module.insert(list(module).index(target), stall)
        overlay_path = work / "stall_order.xml"
        write_stall_port_overlay(overlay, overlay_path)
        case_dir = work / "case"
        case_dir.mkdir()
        env = os.environ.copy()
        env["TMPDIR"] = str(work)
        env["TMP"] = env["TMPDIR"]
        env["TEMP"] = env["TMPDIR"]
        env.update({
            "LP_TB": "tb_lp_mul_array.v",
            "LP_TB_TOP": "tb_lp_mul_array",
            "LP_EXPECT_PIPE_PARAMETER": "2",
            "LP_EXPECT_STALL": "1",
            "LP_EXPECT_NO_MUL_CAPTURE": "1",
            "LP_EXPECT_CAPTURE_COUNT": "0",
            "MANTIS_EXPECT_LOOP_II": "1",
            "MANTIS_EXPECT_LOOP_BB": "2",
            "MANTIS_EXPECT_FUNCTION": "_Z9mul_stageRA16_iS0_R10ac_channelIiE",
        })
        output = run(["bash", str(runner), "--configuration-name=LP-ARRAY-CHANNEL-MODELSIM", "--generate-interface=INFER",
                      str(ROOT / "panda_regressions/hls/bambu_lp_array_channel/lp_mul_array.cpp"),
                      "--top-fname=mul_stage", f"--benchmark-name=stall_order_{placement}", "--clock-period=5",
                      "--device-name=xc7z020,-1,clg484", str(overlay_path)], case_dir, env)
        for expected in ("PASS [BP=0]", "PASS [BP=1]", "PASS [BP=2]", "PASS [MUT oracle sensitivity]"):
            if expected not in output:
                raise RuntimeError(f"{placement} layout run_case.sh omitted {expected}:\n{output}")
        rtl = "\n".join(path.read_text(encoding="utf-8", errors="replace")
                         for path in case_dir.glob("*.v"))
        modules = re.findall(r"(?ms)^module\s+datapath_[A-Za-z0-9_$]+\s*\(.*?^endmodule", rtl)
        candidates = [body for body in modules if "ui_mul_node_FU" in body]
        if len(candidates) != 1:
            raise RuntimeError(f"{placement} stall layout expected one multiplier datapath, found {len(candidates)}")
        body = candidates[0]
        multipliers = re.findall(
            r"(?ms)^\s*ui_mul_node_FU\s*#\((.*?)\)\s*([A-Za-z0-9_$]+)\s*\((.*?)\);", body)
        if len(multipliers) != 1:
            raise RuntimeError(f"{placement} stall layout expected one multiplier instance, found {len(multipliers)}")
        parameters, instance, connections = multipliers[0]
        for name, width in (("in1", 32), ("in2", 32)):
            if not re.search(r"\.BITSIZE_" + name + r"\s*\(\s*" + str(width) + r"\s*\)", parameters):
                raise RuntimeError(f"{placement} layout has incorrect {name} width in {instance}: {parameters}")
        pins = {}
        for name, signal in re.findall(r"\.(in1|in2|stall_port)\s*\(\s*([^()]+?)\s*\)", connections):
            if name in pins:
                raise RuntimeError(f"{placement} layout connects multiplier {name} more than once")
            pins[name] = signal.strip()
        if set(pins) != {"in1", "in2", "stall_port"} or pins["in1"] == pins["in2"]:
            raise RuntimeError(f"{placement} layout does not connect two distinct operands: {pins}")
        if not re.fullmatch(r"selector_stallable_fus", pins["stall_port"]):
            raise RuntimeError(f"{placement} layout connected stall_port to {pins['stall_port']}, not the FSM selector")
        result = re.search(r"\.out1\s*\(\s*([A-Za-z_][A-Za-z0-9_$]*)\s*\)", connections)
        if not result:
            raise RuntimeError(f"{placement} layout has no multiplier result connection")
        captures = re.findall(
            r"(?ms)^\s*TIMED_CAPTURE\s*#\(.*?\)\s*[A-Za-z0-9_$]+\s*\((.*?)\);", body)
        if any(re.search(r"\.in1\s*\(\s*" + re.escape(result.group(1)) + r"\s*\)", capture)
               for capture in captures):
            raise RuntimeError(f"{placement} layout still captures the bound Stallable multiplier result")


def run_stall_argument_cosimulation(root_dir):
    bambu = shutil.which("bambu")
    if not bambu:
        raise RuntimeError("bambu is required for the C argument named stall probe")
    with tempfile.TemporaryDirectory(prefix="stall-argument-", dir=root_dir) as temp:
        work = Path(temp)
        source = work / "stall_argument.cpp"
        source.write_text("""__attribute__((noinline)) int child(int stall, int x)
{
   return stall * 7 + x * 3;
}

int caller(int first, int second)
{
   return child(first, second);
}
""")
        testbench = work / "tb_stall_argument.cpp"
        testbench.write_text("""#include <iostream>

int caller(int first, int second);

int main()
{
   const int first = caller(-13, 7);
   const int second = caller(5, -11);
   const int third = caller(-2, -9);
   if(first != -70 || second != 2 || third != -41)
   {
      std::cerr << "STALL_ARGUMENT_FAIL " << first << ' ' << second << ' ' << third << '\\n';
      return 1;
   }
   std::cout << "STALL_ARGUMENT_COSIM_PASS\\n";
   return 0;
}
""")
        output_dir = work / "output"
        output_dir.mkdir()
        env = os.environ.copy()
        env["TMPDIR"] = str(work)
        env["TMP"] = env["TMPDIR"]
        env["TEMP"] = env["TMPDIR"]
        output = run([bambu, "--configuration-name=LP-ARRAY-CHANNEL-MODELSIM", "--generate-interface=INFER",
                      str(source), "--top-fname=caller", "--benchmark-name=stall_argument", "--clock-period=5",
                      "--device-name=xc7z020,-1,clg484", f"--output-directory={output_dir}",
                      f"--generate-tb={testbench}", "--simulate", "--simulator=MODELSIM"], work, env)
        rtl = "\n".join(path.read_text(encoding="utf-8", errors="replace")
                          for path in output_dir.rglob("*.v"))
        if not re.search(r"(?m)^module\s+[^\s(]*child[^\s(]*\s*\(", rtl):
            raise RuntimeError("noinline child function is absent from generated RTL")
        if not re.search(r"\.stall\s*\(\s*[^,()]+\s*\)", rtl):
            raise RuntimeError("generated child FU does not expose its ordinary stall data argument")
        if "STALL_ARGUMENT_COSIM_PASS" not in output:
            logs = "\n".join(path.read_text(encoding="utf-8", errors="replace")
                                for path in output_dir.rglob("*.log"))
            if "STALL_ARGUMENT_COSIM_PASS" not in logs:
                raise RuntimeError("Bambu co-simulation did not report the stall-argument testbench pass")


def run_nonstallable_capture_probe(template, root_dir):
    bambu = shutil.which("bambu")
    if not bambu:
        raise RuntimeError("bambu is required for the non-Stallable capture fallback probe")
    module_name = template.findtext("name")
    with tempfile.TemporaryDirectory(prefix="nonstallable-capture-", dir=root_dir) as temp:
        work = Path(temp)
        overlay = deepcopy(template)
        attribute = overlay.find("attribute[@name='Stallable']")
        if attribute is None:
            raise RuntimeError("generic multiplier template has no Stallable attribute")
        attribute.text = "0"
        overlay_path = work / "nonstallable_mul.xml"
        write_stall_port_overlay(overlay, overlay_path)
        output_dir = work / "output"
        output_dir.mkdir()
        env = os.environ.copy()
        env["TMPDIR"] = str(work)
        env["TMP"] = env["TMPDIR"]
        env["TEMP"] = env["TMPDIR"]
        run([bambu, "--configuration-name=LP-ARRAY-CHANNEL-MODELSIM", "--generate-interface=INFER",
             str(ROOT / "panda_regressions/hls/bambu_lp_array_channel/lp_co_launch.cpp"),
             "--top-fname=co_launch", "--benchmark-name=nonstallable_capture_fallback", "--clock-period=5",
             "--device-name=xc7z020,-1,clg484", f"--output-directory={output_dir}", str(overlay_path)], work, env)
        rtl = "\n".join(path.read_text(encoding="utf-8", errors="replace")
                          for path in output_dir.rglob("*.v"))
        modules = re.findall(r"(?ms)^module\s+datapath_[A-Za-z0-9_$]+\s*\(.*?^endmodule", rtl)
        candidates = [body for body in modules if module_name in body]
        if len(candidates) != 1:
            raise RuntimeError(f"non-Stallable probe expected one multiplier datapath, found {len(candidates)}")
        body = candidates[0]
        multipliers = re.findall(
            r"(?ms)^\s*" + re.escape(module_name) + r"\s*#\(.*?\)\s*([A-Za-z0-9_$]+)\s*\((.*?)\);", body)
        if len(multipliers) != 1:
            raise RuntimeError(f"non-Stallable probe expected one multiplier instance, found {len(multipliers)}")
        _, connections = multipliers[0]
        if not re.search(r"\.stall_port\s*\(\s*1'b0\s*\)", connections):
            raise RuntimeError("Stallable=false multiplier does not tie its stall_port low")
        result_ports = re.findall(r"\.out1\s*\(\s*([A-Za-z_][A-Za-z0-9_$]*)\s*\)", connections)
        if len(result_ports) != 1:
            raise RuntimeError(f"non-Stallable multiplier has no unique result signal: {result_ports}")
        captures = re.findall(
            r"(?ms)^\s*TIMED_CAPTURE\s*#\(.*?\)\s*[A-Za-z0-9_$]+\s*\((.*?)\);", body)
        if not any(re.search(r"\.in1\s*\(\s*" + re.escape(result_ports[0]) + r"\s*\)", capture)
                   for capture in captures):
            raise RuntimeError("Stallable=false multiplier result is not connected to TIMED_CAPTURE")


def run_bad_metadata_probe(template, mutation, root_dir):
    bambu = shutil.which("bambu")
    if not bambu:
        raise RuntimeError("bambu is required for malformed Stallable metadata probes")
    diagnostics = {
        "missing_type": "Stallable property is missing its value_type",
        "missing_value": "Stallable property is missing its boolean value",
    }
    with tempfile.TemporaryDirectory(prefix=f"bad-stallable-{mutation}-", dir=root_dir) as temp:
        work = Path(temp)
        overlay = deepcopy(template)
        attribute = overlay.find("attribute[@name='Stallable']")
        if attribute is None or mutation not in diagnostics:
            raise ValueError(f"unknown Stallable metadata probe: {mutation}")
        if mutation == "missing_type":
            attribute.attrib.pop("value_type", None)
        else:
            attribute.text = None
        overlay_path = work / "malformed_stallable.xml"
        write_stall_port_overlay(overlay, overlay_path)
        output_dir = work / "output"
        output_dir.mkdir()
        env = os.environ.copy()
        env["TMPDIR"] = str(work)
        env["TMP"] = env["TMPDIR"]
        env["TEMP"] = env["TMPDIR"]
        result = subprocess.run(
            [bambu, "--configuration-name=LP-ARRAY-CHANNEL-MODELSIM", "--generate-interface=INFER",
             str(ROOT / "panda_regressions/hls/bambu_lp_array_channel/lp_mul_array.cpp"),
             "--top-fname=mul_stage", f"--benchmark-name=bad_stallable_metadata_{mutation}",
             "--clock-period=5", "--device-name=xc7z020,-1,clg484", f"--output-directory={output_dir}",
             str(overlay_path)],
            cwd=work, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False,
        )
        if result.returncode == 0 or diagnostics[mutation] not in result.stdout:
            raise RuntimeError(
                f"Release Bambu did not reject malformed Stallable metadata {mutation} "
                f"with the expected diagnostic (status {result.returncode}):\n{result.stdout}"
            )


def run_frp_capture_probe(root_dir):
    bambu = shutil.which("bambu")
    if not bambu:
        raise RuntimeError("bambu is required for the FRP capture fallback probe")
    with tempfile.TemporaryDirectory(prefix="frp-capture-", dir=root_dir) as temp:
        work = Path(temp)
        source = work / "frp_mul.cpp"
        source.write_text('''#include "ac_channel.h"
void frp_mul(int a, int b, ac_channel<int>& out)
{
#pragma HLS pipeline II = 1 style = frp
   out.write(a * b);
}
''')
        output_dir = work / "output"
        output_dir.mkdir()
        env = os.environ.copy()
        env["TMPDIR"] = str(work)
        env["TMP"] = env["TMPDIR"]
        env["TEMP"] = env["TMPDIR"]
        run([bambu, "--configuration-name=LP-ARRAY-CHANNEL-MODELSIM", "--generate-interface=INFER",
             str(source), "--top-fname=frp_mul", "--benchmark-name=frp_stallable_capture",
             "--clock-period=5", "--device-name=xc7z020,-1,clg484", f"--output-directory={output_dir}"], work, env)
        rtl = "\n".join(path.read_text(encoding="utf-8", errors="replace")
                         for path in output_dir.glob("*.v"))
        modules = re.findall(r"(?ms)^module\s+datapath_[A-Za-z0-9_$]+\s*\(.*?^endmodule", rtl)
        candidates = [body for body in modules if "ui_mul_node_FU" in body]
        if len(candidates) != 1:
            raise RuntimeError(f"FRP probe expected one multiplier datapath, found {len(candidates)}")
        body = candidates[0]
        multipliers = re.findall(
            r"(?ms)^\s*ui_mul_node_FU\s*#\(.*?\)\s*([A-Za-z0-9_$]+)\s*\((.*?)\);", body)
        if len(multipliers) != 1:
            raise RuntimeError(f"FRP probe expected one multiplier instance, found {len(multipliers)}")
        _, connections = multipliers[0]
        if not re.search(r"\.stall_port\s*\(\s*1'b0\s*\)", connections):
            raise RuntimeError("FRP multiplier is not tied to stall=0")
        result_ports = re.findall(r"\.out1\s*\(\s*([A-Za-z_][A-Za-z0-9_$]*)\s*\)", connections)
        if len(result_ports) != 1:
            raise RuntimeError(f"FRP multiplier has no unique result signal: {result_ports}")
        captures = re.findall(
            r"(?ms)^\s*TIMED_CAPTURE\s*#\(.*?\)\s*[A-Za-z0-9_$]+\s*\((.*?)\);", body)
        if not any(re.search(r"\.in1\s*\(\s*" + re.escape(result_ports[0]) + r"\s*\)", capture)
                   for capture in captures):
            raise RuntimeError("FRP multiplier result is not connected to a TIMED_CAPTURE instance")


def resolve_modelsim_bin(env):
    vsim = shutil.which("vsim", path=env.get("PATH"))
    if vsim:
        return str(Path(vsim).parent)
    for root in env.get("BAMBU_HLS_BACKEND_PATH", "").split(os.pathsep):
        if not root:
            continue
        result = subprocess.run(
            ["find", "-L", root, "-type", "f", "-path", "*/bin/vsim", "-print", "-quit"],
            text=True, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, check=False,
        )
        if result.stdout.strip():
            return str(Path(result.stdout.strip()).parent)
    raise SystemExit("ModelSim vsim not found in PATH or BAMBU_HLS_BACKEND_PATH")


def run_parallel_cases(cases, jobs, root_dir):
    def run_case_isolated(name, operation):
        with tempfile.TemporaryDirectory(prefix=f"{name}-", dir=root_dir) as temp:
            return operation(Path(temp))

    if not cases:
        return []

    with ThreadPoolExecutor(max_workers=min(jobs, len(cases))) as executor:
        futures = [executor.submit(run_case_isolated, name, operation) for name, operation in cases]
        results = []
        for (name, _), future in zip(cases, futures):
            try:
                results.append((name, future.result(), None))
            except Exception as error:
                detail = str(error) or repr(error) or type(error).__name__
                results.append((name, None, f"{type(error).__name__}: {detail}"))
    return results


def report_case_results(results):
    failed = False
    for name, _, error in results:
        if error is None:
            print(f"PASS: {name}")
        else:
            failed = True
            print(f"FAIL: {name}: {error}", file=sys.stderr)
    if failed:
        raise SystemExit(1)


def run(command, cwd, env):
    result = subprocess.run(
        command, cwd=cwd, env=env, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False,
    )
    if result.returncode:
        raise RuntimeError(
            f"Command failed ({result.returncode}): {' '.join(map(str, command))}\n{result.stdout}"
        )
    return result.stdout


def verilog_model(name, body, signed):
    sign = " signed" if signed else ""
    return f"""module {name} #(parameter integer BITSIZE_in1=8, BITSIZE_in2=8,
  BITSIZE_out1=16, PIPE_PARAMETER=2) (
  input wire clock, stall_port,
  input wire{sign} [BITSIZE_in1-1:0] in1,
  input wire{sign} [BITSIZE_in2-1:0] in2,
  output wire{sign} [BITSIZE_out1-1:0] out1
);
{body}
endmodule
"""


def lattice_pmi_stub():
    return """module pmi_dsp_mult #(
  parameter integer pmi_dataa_width=8, pmi_datab_width=8,
  parameter integer pmi_additional_pipeline=0,
  parameter pmi_input_reg="off", pmi_output_reg="off",
  parameter pmi_family="", pmi_gsr="", pmi_source_control_a="", pmi_source_control_b="",
  parameter pmi_reg_inputa_clk="CLK0", pmi_reg_inputa_ce="CE0", pmi_reg_inputa_rst="RST0",
  parameter pmi_reg_inputb_clk="CLK0", pmi_reg_inputb_ce="CE0", pmi_reg_inputb_rst="RST0",
  parameter pmi_reg_pipeline_clk="CLK0", pmi_reg_pipeline_ce="CE0", pmi_reg_pipeline_rst="RST0",
  parameter pmi_reg_output_clk="CLK0", pmi_reg_output_ce="CE0", pmi_reg_output_rst="RST0",
  parameter pmi_reg_signeda_clk="CLK0", pmi_reg_signeda_ce="CE0", pmi_reg_signeda_rst="RST0",
  parameter pmi_reg_signedb_clk="CLK0", pmi_reg_signedb_ce="CE0", pmi_reg_signedb_rst="RST0",
  parameter pmi_pipelined_mode="off", parameter module_type="pmi_dsp_mult"
) (
  input wire [pmi_dataa_width-1:0] A,
  input wire [pmi_datab_width-1:0] B,
  input wire [17:0] SRIA, SRIB,
  input wire CLK0, CLK1, CLK2, CLK3,
  input wire CE0, CE1, CE2, CE3,
  input wire RST0, RST1, RST2, RST3,
  input wire SignA, SignB, SourceA, SourceB,
  output wire [pmi_dataa_width+pmi_datab_width-1:0] P,
  output wire [17:0] SROA, SROB
);
localparam integer STAGES = ((pmi_input_reg == "on") ? 1 : 0) +
                            ((pmi_output_reg == "on") ? 1 : 0) + pmi_additional_pipeline;
localparam integer PRODUCT_WIDTH = pmi_dataa_width + pmi_datab_width;
wire [PRODUCT_WIDTH-1:0] unsigned_product = A * B;
wire signed [PRODUCT_WIDTH-1:0] signed_product = $signed(A) * $signed(B);
wire [PRODUCT_WIDTH-1:0] product = (SignA || SignB) ? signed_product : unsigned_product;
assign SROA = 18'b0;
assign SROB = 18'b0;
generate
  if (STAGES == 0) begin : g_comb
    assign P = product;
  end else begin : g_pipe
    reg [PRODUCT_WIDTH-1:0] stage [0:STAGES-1];
    integer i;
    always @(posedge CLK0) begin
      if (CE0) begin
        stage[0] <= product;
        for (i=1; i<STAGES; i=i+1) stage[i] <= stage[i-1];
      end
    end
    assign P = stage[STAGES-1];
  end
endgenerate
endmodule
"""


def lattice_verilog_model(name, template, signed):
    return lattice_pmi_stub() + "\n" + verilog_model(
        name, functionality(template, "VERILOG_PROVIDED"), signed)


def vhdl_model(name, body, signed):
    dtype = "signed" if signed else "unsigned"
    component = ""
    if name in ("widen_mul_node_FU", "ui_widen_mul_node_FU"):
        component_name = "mul_node_FU" if signed else "ui_mul_node_FU"
        component_type = "signed" if signed else "unsigned"
        component = f"""  component {component_name} is
    generic (BITSIZE_in1 : positive := 8; BITSIZE_in2 : positive := 8;
             BITSIZE_out1 : positive := 16; PIPE_PARAMETER : natural := 2);
    port (clock, stall_port : in std_logic;
          in1 : in {component_type}(BITSIZE_in1-1 downto 0);
          in2 : in {component_type}(BITSIZE_in2-1 downto 0);
          out1 : out {component_type}(BITSIZE_out1-1 downto 0));
  end component;
"""
    resize_helper = """  function resize_signed(value : signed; new_size : natural) return signed is
  begin
    return resize(value, new_size);
  end function;
""" if signed else ""
    return f"""library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
entity {name} is
  generic (BITSIZE_in1 : positive := 8; BITSIZE_in2 : positive := 8;
           BITSIZE_out1 : positive := 16; PIPE_PARAMETER : natural := 2);
  port (clock, stall_port : in std_logic;
        in1 : in {dtype}(BITSIZE_in1-1 downto 0);
        in2 : in {dtype}(BITSIZE_in2-1 downto 0);
        out1 : out {dtype}(BITSIZE_out1-1 downto 0));
end entity;
architecture rtl of {name} is
{component}{resize_helper}{body}
end architecture;
"""


def verilog_testbench(name, signed):
    sign = " signed" if signed else ""
    return f"""module tb;
parameter integer P=2;
reg clock=0;
always #5 clock=~clock;
reg stall=0;
reg{sign} [7:0] in1=0, in2=0;
wire{sign} [15:0] out1;
localparam integer MAX_PIPE_STAGES=7;
reg [15:0] stages [0:MAX_PIPE_STAGES-1];
reg [15:0] product;
integer valid_count=0, i, j;
{name} #(.BITSIZE_in1(8), .BITSIZE_in2(8), .BITSIZE_out1(16), .PIPE_PARAMETER(P)) dut(
  .clock(clock), .stall_port(stall), .in1(in1), .in2(in2), .out1(out1));

task step;
  input blocked;
  input integer a_value, b_value;
  begin
    @(negedge clock);
    stall=blocked;
    in1=a_value;
    in2=b_value;
    product=in1*in2;
    @(posedge clock); #1;
    if (P == 0) begin
      if (out1 !== product) begin
        $display("STALL_TEST_FAIL P=%0d combinational got=%h expected=%h", P, out1, product);
        $finish;
      end
    end else begin
      if (!blocked) begin
        if (P > 0) begin
          for (j=P-1; j>0; j=j-1) stages[j]=stages[j-1];
          stages[0]=product;
        end
        if (valid_count < P) valid_count=valid_count+1;
      end
      if (P > 0 && valid_count >= P) begin
        if (out1 !== stages[P-1]) begin
          $display("STALL_TEST_FAIL P=%0d got=%h expected=%h", P, out1, stages[P-1]);
          $finish;
        end
      end
    end
  end
endtask

initial begin
  for (i=0; i<10; i=i+1) step(0, i+3, i+11);
  step(1, 71, 19);
  for (i=0; i<6; i=i+1) step(1, i+83, i+23);
  for (i=0; i<14; i=i+1) step(i%2, i+101, i+37);
  for (i=0; i<P+5; i=i+1) step(0, i+129, i+53);
  step(1, 211, 61);
  $display("STALL_TEST_PASS P=%0d", P);
  $finish;
end
endmodule
"""


def vhdl_testbench(name, signed):
    dtype = "signed" if signed else "unsigned"
    a_expr = "to_signed(a_value, 8)" if signed else "to_unsigned(a_value mod 256, 8)"
    b_expr = "to_signed(b_value, 8)" if signed else "to_unsigned(b_value mod 256, 8)"
    product_expr = f"std_logic_vector({a_expr} * {b_expr})"
    a_drive = "to_signed(a_value, 8)" if signed else "to_unsigned(a_value mod 256, 8)"
    b_drive = "to_signed(b_value, 8)" if signed else "to_unsigned(b_value mod 256, 8)"
    return f"""library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.env.all;
entity tb is
  generic (PIPE_PARAMETER : natural := 2);
end entity;
architecture test of tb is
  signal clock : std_logic := '0';
  signal stall : std_logic := '0';
  signal in1, in2 : {dtype}(7 downto 0) := (others => '0');
  signal out1 : {dtype}(15 downto 0);
begin
  clock <= not clock after 5 ns;
  dut : entity work.{name}
    generic map (BITSIZE_in1 => 8, BITSIZE_in2 => 8, BITSIZE_out1 => 16,
                 PIPE_PARAMETER => PIPE_PARAMETER)
    port map (clock => clock, stall_port => stall, in1 => in1, in2 => in2, out1 => out1);

  stimulus : process
    constant MAX_PIPE_STAGES : natural := 7;
    type stage_array is array (0 to MAX_PIPE_STAGES-1) of std_logic_vector(15 downto 0);
    variable stages : stage_array := (others => (others => '0'));
    variable valid_count : natural := 0;
    variable product : std_logic_vector(15 downto 0);
    variable expected : std_logic_vector(15 downto 0);
    variable i : integer;
    procedure step(constant blocked : std_logic; constant a_value, b_value : integer) is
    begin
      wait until falling_edge(clock);
      stall <= blocked;
      in1 <= {a_drive};
      in2 <= {b_drive};
      product := {product_expr};
      wait until rising_edge(clock);
      wait for 1 ns;
      if PIPE_PARAMETER = 0 then
        if std_logic_vector(out1) /= product then
          report "STALL_TEST_FAIL combinational result" severity error;
          finish;
        end if;
      else
        if blocked = '0' then
          if PIPE_PARAMETER > 0 then
            for i in PIPE_PARAMETER-1 downto 1 loop
              stages(i) := stages(i-1);
            end loop;
            stages(0) := product;
          end if;
          if valid_count < PIPE_PARAMETER then valid_count := valid_count + 1; end if;
        end if;
        if PIPE_PARAMETER > 0 and valid_count >= PIPE_PARAMETER then
          expected := stages(PIPE_PARAMETER-1);
          if std_logic_vector(out1) /= expected then
            report "STALL_TEST_FAIL ordered result/frozen pipeline mismatch" severity error;
            finish;
          end if;
        end if;
      end if;
    end procedure;
  begin
    for i in 0 to 9 loop step('0', i+3, i+11); end loop;
    step('1', 71, 19);
    for i in 0 to 5 loop step('1', i+83, i+23); end loop;
    for i in 0 to 13 loop
      if i mod 2 = 0 then step('0', i+101, i+37);
      else step('1', i+101, i+37); end if;
    end loop;
    for i in 0 to PIPE_PARAMETER+4 loop step('0', i+129, i+53); end loop;
    step('1', 211, 61);
    report "STALL_TEST_PASS P=" & natural'image(PIPE_PARAMETER) severity note;
    finish;
    wait;
  end process;
end architecture;
"""


def simulate_model(model_name, model_text, tb_text, params, language, bin_dir, root_dir):
    work = root_dir / f"{language}_{model_name}"
    work.mkdir()
    env = os.environ.copy()
    env["PATH"] = bin_dir + os.pathsep + env.get("PATH", "")
    env["TMPDIR"] = str(root_dir)
    env["TMP"] = env["TMPDIR"]
    env["TEMP"] = env["TMPDIR"]
    if language == "verilog":
        model_path = work / "model.v"
        tb_path = work / "tb.v"
        model_path.write_text(model_text)
        tb_path.write_text(tb_text)
        run(["vlib", "work"], work, env)
        run(["vlog", str(model_path), str(tb_path)], work, env)
        for parameter in params:
            output = run(["vsim", "-c", f"-gP={parameter}", "tb", "-do", "run -all; quit -f"], work, env)
            if f"STALL_TEST_PASS P={parameter}" not in output:
                raise RuntimeError(f"Verilog {model_name} P={parameter} failed:\n{output}")
    else:
        model_path = work / "model.vhd"
        tb_path = work / "tb.vhd"
        model_path.write_text(model_text)
        tb_path.write_text(tb_text)
        run(["vlib", "work"], work, env)
        run(["vcom", "-2008", str(model_path), str(tb_path)], work, env)
        for parameter in params:
            output = run(["vsim", "-c", f"-gPIPE_PARAMETER={parameter}", "tb", "-do", "run -all; quit -f"], work, env)
            if f"STALL_TEST_PASS P={parameter}" not in output:
                raise RuntimeError(f"VHDL {model_name} P={parameter} failed:\n{output}")


def compile_wrappers(c_hls, c_vec, bin_dir, root_dir):
    work = root_dir / "wrapper_compile"
    work.mkdir()
    env = os.environ.copy()
    env["PATH"] = bin_dir + os.pathsep + env.get("PATH", "")
    env["TMPDIR"] = str(root_dir)
    env["TMP"] = env["TMPDIR"]
    env["TEMP"] = env["TMPDIR"]

    vhdl_models = []
    for name, is_signed in (("mul_node_FU", True), ("ui_mul_node_FU", False),
                            ("widen_mul_node_FU", True), ("ui_widen_mul_node_FU", False)):
        template = c_hls[name]
        assert_stallable_contract(template)
        vhdl_models.append(vhdl_model(name, functionality(template, "VHDL_PROVIDED"), is_signed))
    (work / "hls.vhd").write_text("\n".join(vhdl_models))
    run(["vlib", "work"], work, env)
    run(["vcom", "-2008", str(work / "hls.vhd")], work, env)

    cores = []
    for name, is_signed in (("mul_node_FU", True), ("ui_mul_node_FU", False),
                            ("widen_mul_node_FU", True), ("ui_widen_mul_node_FU", False)):
        template = c_hls[name]
        assert_stallable_contract(template)
        cores.append(verilog_model(name, functionality(template, "VERILOG_PROVIDED"), is_signed))
    vector_modules = []
    for name, template in c_vec.items():
        if "mul_node_FU" not in name and "widen_mult_" not in name:
            continue
        assert_stallable_contract(template)
        body = functionality(template, "VERILOG_PROVIDED")
        sign = " signed" if name.startswith("vec_") else ""
        in_width = "2*NUM_ELEM_out1*BITSIZE_in1" if "widen_mult_" in name else "NUM_ELEM_out1*BITSIZE_in1"
        in2_width = "2*NUM_ELEM_out1*BITSIZE_in2" if "widen_mult_" in name else "NUM_ELEM_out1*BITSIZE_in2"
        out_width = "NUM_ELEM_out1*BITSIZE_out1"
        vector_modules.append(f"""module {name} #(parameter BITSIZE_in1=8, BITSIZE_in2=8,
  BITSIZE_out1=16, NUM_ELEM_out1=2, PIPE_PARAMETER=2) (
  input wire clock, stall_port,
  input wire{sign} [{in_width}-1:0] in1,
  input wire{sign} [{in2_width}-1:0] in2,
  output wire{sign} [{out_width}-1:0] out1
);
{body}
endmodule
""")
    (work / "wrappers.v").write_text("\n".join(cores + vector_modules))
    run(["vlog", str(work / "wrappers.v")], work, env)


def run_mutation_check(template, tb_text, bin_dir, root_dir):
    body = functionality(template, "VERILOG_PROVIDED")
    mutated = body.replace("if (stall_port == 1'b0)", "if (1'b1)")
    if mutated == body:
        raise RuntimeError("Could not construct the stall-gate mutation")
    model_text = verilog_model("mul_node_FU", mutated, True)
    work = root_dir / "mutation"
    work.mkdir()
    env = os.environ.copy()
    env["PATH"] = bin_dir + os.pathsep + env.get("PATH", "")
    env["TMPDIR"] = str(root_dir)
    env["TMP"] = env["TMPDIR"]
    env["TEMP"] = env["TMPDIR"]
    (work / "model.v").write_text(model_text)
    (work / "tb.v").write_text(tb_text)
    run(["vlib", "work"], work, env)
    run(["vlog", str(work / "model.v"), str(work / "tb.v")], work, env)
    output = run(["vsim", "-c", "-gP=2", "tb", "-do", "run -all; quit -f"], work, env)
    if "STALL_TEST_FAIL" not in output or "STALL_TEST_PASS" in output:
        raise RuntimeError(f"Stall-gate mutation was not detected:\n{output}")


def main(argv=None):
    jobs = resolve_parallelism(sys.argv[1:] if argv is None else argv)
    assert_lattice_stallable_contract()
    c_hls = templates("etc/libtech/C_HLS_IPs.xml")
    c_vec = templates("etc/libtech/C_VEC_IPs.xml")
    model_names = ("mul_node_FU", "ui_mul_node_FU")
    for name in model_names:
        assert_stallable_contract(c_hls[name])

    parent = ROOT / "documentation/tmp/pr436-stallable/release-port-probe"
    parent_created = not parent.exists()
    parent.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    bin_dir = resolve_modelsim_bin(env)
    with tempfile.TemporaryDirectory(prefix="run-", dir=parent) as temp:
        work_root = Path(temp)
        model_cases = []
        for name, is_signed in (("mul_node_FU", True), ("ui_mul_node_FU", False)):
            template = c_hls[name]
            for language, model, tb in (
                    ("verilog", verilog_model(name, functionality(template, "VERILOG_PROVIDED"), is_signed),
                     verilog_testbench(name, is_signed)),
                    ("vhdl", vhdl_model(name, functionality(template, "VHDL_PROVIDED"), is_signed),
                     vhdl_testbench(name, is_signed))):
                case_name = f"{language}_{name}"
                model_cases.append((case_name,
                                    lambda case_root, name=name, model=model, tb=tb, language=language:
                                        simulate_model(name, model, tb, MODEL_PARAMS, language, bin_dir, case_root)))
        lattice_e3 = templates(f"etc/libtech/lattice/{LATTICE_DEVICES[0]}.xml")
        for name, is_signed in (("mul_node_FU", True), ("ui_mul_node_FU", False)):
            lattice_name = f"lattice_{name}"
            model = lattice_verilog_model(name, lattice_e3[name], is_signed)
            tb = verilog_testbench(name, is_signed)
            model_cases.append((lattice_name,
                                lambda case_root, lattice_name=lattice_name, model=model, tb=tb:
                                    simulate_model(lattice_name, model, tb, MODEL_PARAMS, "verilog", bin_dir,
                                                   case_root)))
        model_cases.extend(lattice_widening_cases(c_hls, bin_dir))
        model_cases.extend([
            ("wrapper_compile", lambda case_root: compile_wrappers(c_hls, c_vec, bin_dir, case_root)),
            ("stall_gate_mutation", lambda case_root:
                run_mutation_check(c_hls["mul_node_FU"], verilog_testbench("mul_node_FU", True), bin_dir,
                                   case_root)),
        ])
        report_case_results(run_parallel_cases(model_cases, jobs, work_root))

        bambu_cases = []
        for malformed_port in ("missing", "wide"):
            case_name = f"bad_stall_{malformed_port}"
            bambu_cases.append((case_name,
                                lambda case_root, malformed_port=malformed_port:
                                    run_bad_port_probe(c_hls["ui_mul_node_FU"], malformed_port, case_root)))
        for placement in ("before", "between"):
            case_name = f"stall_order_{placement}"
            bambu_cases.append((case_name,
                                lambda case_root, placement=placement:
                                    run_stall_port_order_probe(c_hls["ui_mul_node_FU"], placement, case_root)))
        for malformed_metadata in ("missing_type", "missing_value"):
            case_name = f"bad_stallable_metadata_{malformed_metadata}"
            bambu_cases.append((case_name,
                                lambda case_root, malformed_metadata=malformed_metadata:
                                    run_bad_metadata_probe(c_hls["ui_mul_node_FU"], malformed_metadata, case_root)))
        bambu_cases.append(("frp_capture", run_frp_capture_probe))
        bambu_cases.extend((f"lattice_device_{device}",
                            lambda case_root, device=device:
                                run_lattice_device_resolution_probe(case_root, device))
                           for device in LATTICE_DEVICES)
        bambu_cases.extend([
            ("stall_argument_cosimulation", run_stall_argument_cosimulation),
            ("nonstallable_capture_fallback",
             lambda case_root: run_nonstallable_capture_probe(c_hls["ui_mul_node_FU"], case_root)),
        ])
        report_case_results(run_parallel_cases(bambu_cases, jobs, work_root))

    if parent_created and not any(parent.iterdir()):
        parent.rmdir()
    print("stallable multiplier tests passed: signed/unsigned Verilog and VHDL P=0/1/2/3/7; "
          "wrappers compile; mutations detected; malformed Stallable ports and metadata rejected; "
          "stall port ordering passed; FRP capture retained; C argument stall preserved; "
          "Stallable=false capture fallback retained")


if __name__ == "__main__":
    main()
