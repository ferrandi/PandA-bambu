#!/usr/bin/env bash
set -Eeuo pipefail

usage() {
  echo 'Usage: run_t4b_matched.sh B O D L N Q FRESH_OUT [--fixture sum|independent|mulxor] [--mode burst|legacy|scheduler-off] [--axi-burst-type INCR]' >&2
}
fail() { echo "T4B_FAIL: $*" >&2; exit 2; }
is_uint() { [[ $1 =~ ^(0|[1-9][0-9]*)$ ]]; }
range_ok() { is_uint "$1" && ((${#1} <= ${#3})) && (( $1 >= $2 && $1 <= $3 )); }

TESTS=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
ROOT=$(git -C "$TESTS" rev-parse --show-toplevel)
LAUNCH_DIR=$PWD
OUT_ARG_ABSOLUTE=
cd "$ROOT"
if [[ ${1:-} == -h || ${1:-} == --help ]]; then usage; exit 0; fi
(( $# >= 7 )) || { usage; exit 2; }
B=$1 O=$2 D=$3 L=$4 N=$5 Q=$6 OUT_ARG=$7
if [[ $OUT_ARG == /* ]]; then OUT_ARG_ABSOLUTE=$OUT_ARG; else OUT_ARG_ABSOLUTE="$LAUNCH_DIR/$OUT_ARG"; fi
shift 7
FIXTURE=sum MODE=burst AXI_TYPE=INCR
while (( $# )); do
  case $1 in
    --fixture) (( $# >= 2 )) || fail '--fixture requires a value'; FIXTURE=$2; shift 2 ;;
    --mode) (( $# >= 2 )) || fail '--mode requires a value'; MODE=$2; shift 2 ;;
    --axi-burst-type) (( $# >= 2 )) || fail '--axi-burst-type requires a value'; AXI_TYPE=$2; shift 2 ;;
    *) fail "unknown option: $1" ;;
  esac
done
[[ $AXI_TYPE == INCR ]] || fail 'only AXI INCR bursts are supported'
range_ok "$B" 1 256 || fail 'B must be an unsigned decimal integer in 1..256'
range_ok "$O" 1 16 || fail 'O must be an unsigned decimal integer in 1..16'
range_ok "$D" 1 4096 || fail 'D must be an unsigned decimal integer in 1..4096'
[[ $L == 64 ]] || fail 'L must be 64 for the matched T4B environment'
range_ok "$N" 1 4096 || fail 'N must be an unsigned decimal integer in 1..4096'
range_ok "$Q" 0 4095 || fail 'Q must be an unsigned decimal integer in 0..4095'
(( Q + 1 > O )) || fail 'Q+1 must exceed O'
case $FIXTURE in sum|independent|mulxor) ;; *) fail "unknown fixture: $FIXTURE" ;; esac
case $MODE in burst|legacy|scheduler-off) ;; *) fail "unknown mode: $MODE" ;; esac

[[ ! -L $OUT_ARG_ABSOLUTE ]] || fail "refusing symlink output path: $OUT_ARG_ABSOLUTE"
OUT=$(realpath -m -- "$OUT_ARG_ABSOLUTE")
[[ ! -e $OUT ]] || fail "refusing to reuse existing output: $OUT"
[[ ! -L $(dirname "$OUT") ]] || fail 'output parent may not be a symlink'

PROFILE_TOOL="$TESTS/t4b_profile.py"
CHECKER="$TESTS/t4b_vcd.py"
PARAM_CHECKER="$TESTS/check_t4b_effective_params.py"
TB="$TESTS/tb_t4b_witness.cpp"
[[ -r $PROFILE_TOOL && -r $TB ]] || fail 'T4B profile tool or testbench missing'
if [[ ${T4B_TEST_MODE:-0} == 1 && -n ${T4B_TEST_CHECKER:-} ]]; then CHECKER=$T4B_TEST_CHECKER; fi
[[ $MODE == scheduler-off || -r $CHECKER ]] || fail "H2 checker missing: $CHECKER"
MODELSIM_ROOT=${MODELSIM_ROOT:-/opt/mentor}
VSIM="$MODELSIM_ROOT/questasim/bin/vsim"
if [[ $MODE != scheduler-off ]]; then
  [[ ${BURST_MODELSIM_OUTSIDE_SANDBOX:-0} == 1 ]] || fail 'ModelSim modes require BURST_MODELSIM_OUTSIDE_SANDBOX=1'
  [[ -x $VSIM ]] || fail "ModelSim/Questa executable unavailable: $VSIM"
fi

# Invalid or unsupported inputs fail before output creation or build activity.
mkdir -p "$(dirname "$OUT")"
mkdir "$OUT"
mkdir -p "$OUT/run" "$OUT/preflight"
mkdir -p "$OUT/tmp"
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp"
PROFILE_CPP="$OUT/profile.cpp"
PROFILE_JSON="$OUT/profile.json"
python3 -B "$PROFILE_TOOL" --fixture "$FIXTURE" --b "$B" --o "$O" --d "$D" --l "$L" --output "$PROFILE_CPP" --manifest "$PROFILE_JSON" >"$OUT/profile.stdout.log" 2>"$OUT/profile.stderr.log" || fail 'profile generation failed'

# Mandatory complete build and install before any Bambu command.
if ! cmake --build build --parallel 8 >"$OUT/preflight/build.log" 2>&1; then fail "full build failed; see $OUT/preflight/build.log"; fi
if ! cmake --install build >"$OUT/preflight/install.log" 2>&1; then fail "install failed; see $OUT/preflight/install.log"; fi
INSTALL_PREFIX="$ROOT/install"
if [[ ${T4B_TEST_MODE:-0} == 1 && -n ${T4B_TEST_INSTALL_PREFIX:-} ]]; then INSTALL_PREFIX=$T4B_TEST_INSTALL_PREFIX; fi
SETTINGS="$INSTALL_PREFIX/settings.sh"
[[ -r $SETTINGS ]] || fail "installed settings.sh missing: $SETTINGS"
# shellcheck disable=SC1090
source "$SETTINGS"
IFS=$' \t\n'
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp"
BAMBU="$INSTALL_PREFIX/bin/bambu"
[[ -x $BAMBU ]] || fail "installed Bambu executable missing: $BAMBU"
if [[ $MODE != scheduler-off ]]; then export PATH="$(dirname "$VSIM"):$PATH"; fi
BAMBU_VERSION=$($BAMBU --version 2>&1 | head -n 3 || true)
SIMULATOR_VERSION='not-used'
if [[ $MODE != scheduler-off ]]; then SIMULATOR_VERSION=$($VSIM -version 2>&1 | head -n 4 || true); fi

TOP=kernel FIXTURE_DEFINE=-DT4B_FIXTURE_SUM=1
case $FIXTURE in
  independent) TOP=first_read_independent_work; FIXTURE_DEFINE=-DT4B_FIXTURE_INDEPENDENT=1 ;;
  mulxor) TOP=first_read_mulxor_work; FIXTURE_DEFINE=-DT4B_FIXTURE_MULXOR=1 ;;
esac
ARGS=("$PROFILE_CPP" "--top-fname=$TOP" "--generate-tb=$TB" "--tb-arg=$N"
  --compiler=I386_CLANG16 --generate-interface=INFER --clock-period=3.33
  --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL
  --mem-delay-read="$L" --mem-delay-write=2 "--tb-queue-size=$Q" --memory-allocation-policy=NO_BRAM
  --distram-threshold=0 --generate-vcd --no-clean --extra-cc-options=-DNOCACHE
  --extra-cc-options=-DBAMBU "--extra-cc-options=$FIXTURE_DEFINE"
  "--extra-cc-options=-DT4B_DEFAULT_N=$N" --extra-cc-options=-DT4B_SEED=0x13579bdfu)
if [[ $FIXTURE == independent || $FIXTURE == mulxor ]]; then
  # The pointer-only witness signature is inferred as a 4-byte region by the
  # architecture XML. Its testbench owns 4096 uint32 words, so size the MDPI
  # wrapper argument to the complete 16 KiB allocation for every N.
  ARGS+=(--tb-param-size=src:16384)
fi
if [[ $MODE == scheduler-off ]]; then
  ARGS+=(--bambu-parameter=experimental-m-axi-burst=1 --bambu-parameter=print-ir-manager=1
    --debug-classes=InterfaceInfer,BuildVirtualPhi,parametric_list_based --print-dot)
else
  ARGS+=(--simulate --simulator=MODELSIM "--mentor-root=$MODELSIM_ROOT")
  if [[ $MODE == burst ]]; then ARGS+=(--bambu-parameter=experimental-m-axi-burst=1)
  else ARGS+=(--bambu-parameter=experimental-m-axi-burst=0); fi
fi

printf '%q ' "$BAMBU" "${ARGS[@]}" >"$OUT/command.txt"
echo >>"$OUT/command.txt"
set +e
(cd "$OUT/run" && "$BAMBU" "${ARGS[@]}") >"$OUT/bambu.stdout.log" 2>"$OUT/bambu.stderr.log"
BAMBU_RC=$?
set -e
echo "$BAMBU_RC" >"$OUT/bambu.exit_code"
RESULT_JSON="$OUT/checker.json"
CHECKER_RC=not-run
READ_DELAY_DERIVED=not-checked
if [[ $MODE != scheduler-off ]]; then READ_DELAY_DERIVED=$((L - 1)); fi
# Write an initial result record before inspecting simulator outputs, so even
# missing VCDs and failed transcripts leave complete provenance beside logs.
python3 -B - "$OUT" "$ROOT" "$TESTS" "$BAMBU" "$SETTINGS" "$VSIM" "$MODE" \
  "$FIXTURE" "$B" "$O" "$D" "$L" "$N" "$Q" "$AXI_TYPE" "$BAMBU_RC" \
  "$BAMBU_VERSION" "$SIMULATOR_VERSION" "$CHECKER" "$READ_DELAY_DERIVED" "$PROFILE_JSON" "$TB" <<'PY'
import hashlib, json, pathlib, shlex, subprocess, sys
(out, root, tests, bambu, settings, vsim, mode, fixture, b, o, d, latency,
 n, q, axi_type, bambu_rc, version, simulator_version, checker, read_delay,
 profile_json, tb) = sys.argv[1:]
def sha(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""): h.update(block)
    return h.hexdigest()
p = json.loads(pathlib.Path(profile_json).read_text(encoding="utf-8"))
files = []
for path in sorted(pathlib.Path(out).rglob("*")):
    if path.is_file() and path.name != "manifest.json":
        files.append({"path": str(path.relative_to(out)), "bytes": path.stat().st_size, "sha256": sha(str(path))})
pathlib.Path(out, "manifest.json").write_text(json.dumps({**p,
  "status": "INCOMPLETE", "mode": mode, "bambu": {"path": bambu, "sha256": sha(bambu), "version": version.strip()},
  "settings": {"path": settings, "sha256": sha(settings), "sourced": True},
  "testbench": {"path": tb, "sha256": sha(tb)},
  "simulator": {"path": vsim if mode != "scheduler-off" else None, "version": simulator_version.strip(), "sha256": sha(vsim) if mode != "scheduler-off" else None},
  "checker": {"path": checker if mode != "scheduler-off" else None, "sha256": sha(checker) if mode != "scheduler-off" else None},
  "read_delay_derived": int(read_delay) if read_delay.isdecimal() else read_delay,
  "repository": {"root": root, "revision": subprocess.check_output(["git", "-C", root, "rev-parse", "HEAD"], text=True).strip()},
  "device": "xcu250,-2L,figd2104", "parameters": {"B": int(b), "O": int(o), "D": int(d), "L": int(latency), "N": int(n), "Q": int(q),
    "AXI_burst_type": axi_type, "clock_period_ns": 3.33, "mem_delay_read": int(latency), "mem_delay_write": 2,
    "memory_allocation_policy": "NO_BRAM", "distram_threshold": 0, "experimental_m_axi_burst": 0 if mode == "legacy" else 1},
  "bambu_argv": shlex.split(pathlib.Path(out, "command.txt").read_text()), "bambu_exit_code": int(bambu_rc),
  "testbench_parameter_sizes_bytes": {"src": 16384} if fixture in ("independent", "mulxor") else {},
  "occupancy_source": "axi_handshake_reconstruction",
  "descriptor_count_dumped": None, "descriptor_count_width": None,
  "artifacts": files}, indent=2) + "\n", encoding="utf-8")
PY
if (( BAMBU_RC == 0 )); then
  if [[ $MODE == scheduler-off ]]; then
    find "$OUT/run" -type f \( -name '*.bambuir' -o -name '*.ir' -o -name '*.dot' -o -name '*.gv' \) -print >"$OUT/ir_dot_artifacts.txt"
    rg -q '\.bambuir$' "$OUT/ir_dot_artifacts.txt" || fail 'scheduler-off capture produced no .bambuir artifact'
    rg -q '\.(dot|gv)$' "$OUT/ir_dot_artifacts.txt" || fail 'scheduler-off capture produced no DOT artifact'
  else
    SIM_DIR="$OUT/run/HLS_output/simulation"
    TEST_LOG="$SIM_DIR/testbench.log"
    BACKEND_LOG="$SIM_DIR/modelsim_backend/launch.log"
    RESULTS="$SIM_DIR/modelsim_backend/bambu_results.xml"
    VCD="$SIM_DIR/test.vcd"
    for artifact in "$TEST_LOG" "$BACKEND_LOG" "$RESULTS" "$VCD"; do [[ -s $artifact ]] || fail "required simulation artifact missing or empty: $artifact"; done
    if ! python3 -B - "$VCD" "$OUT/manifest.json" <<'PY'
import json, pathlib, sys

vcd, manifest_path = map(pathlib.Path, sys.argv[1:])
scopes = []
declarations = []
ended = False
try:
    with vcd.open(encoding="ascii", errors="strict") as stream:
        for line in stream:
            parts = line.strip().split()
            if not parts:
                continue
            if parts[0] == "$scope" and len(parts) >= 4:
                scopes.append(parts[2])
            elif parts[0] == "$upscope":
                if not scopes:
                    raise ValueError("unbalanced $upscope")
                scopes.pop()
            elif parts[0] == "$var" and len(parts) >= 6:
                if parts[4] == "descriptor_count" and scopes and scopes[-1] == "burst_engine":
                    width = int(parts[2])
                    if width <= 0:
                        raise ValueError("descriptor_count has a non-positive width")
                    declarations.append((parts[3], width))
            elif parts[0] == "$enddefinitions":
                ended = True
                break
    if not ended:
        raise ValueError("VCD has no $enddefinitions")
    widths = {width for _, width in declarations}
    if len(widths) > 1:
        raise ValueError(f"conflicting descriptor_count widths: {sorted(widths)}")
except (OSError, UnicodeError, ValueError) as exc:
    print(f"invalid descriptor_count VCD evidence: {exc}", file=sys.stderr)
    raise SystemExit(1)

manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
manifest["descriptor_count_dumped"] = bool(declarations)
manifest["descriptor_count_width"] = next(iter(widths)) if widths else None
manifest["descriptor_count_vcd_scope"] = "burst_engine" if declarations else None
manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
PY
    then fail 'descriptor_count VCD evidence is malformed or has conflicting declarations'; fi
    rg -q 'T4B_GOLDEN_PASS' "$TEST_LOG" || fail 'testbench did not report golden-result PASS'
    rg -q 'Errors[[:space:]]*=[[:space:]]*0([,[:space:]]|$)' "$TEST_LOG" || fail 'testbench transcript reports errors'
    rg -q '^# End time:' "$BACKEND_LOG" || fail 'ModelSim transcript lacks completion marker'
    rg -q '<simulation return_value="0">' "$RESULTS" || fail 'Bambu simulation result is not PASS'
    RTL_FILE="$OUT/run/$TOP.v"
    [[ -s $RTL_FILE ]] || fail "generated RTL for top '$TOP' is missing: $RTL_FILE"
    [[ -s $OUT/run/HLS_output/simulation/bambu_testbench.v ]] || fail 'generated simulation RTL is missing'
    LATENCY_JSON="$OUT/generated_latency.json"
    if ! python3 -B - "$OUT/run" "$L" "$LATENCY_JSON" <<'PY'
import json, pathlib, re, sys, xml.etree.ElementTree as ET
run = pathlib.Path(sys.argv[1])
expected = int(sys.argv[2])
output = pathlib.Path(sys.argv[3])
matches = []
for path in sorted(run.rglob('architecture.xml')):
    try:
        root = ET.parse(path).getroot()
    except (ET.ParseError, OSError) as exc:
        raise SystemExit(f"cannot parse generated architecture XML {path}: {exc}")
    for element in root.iter():
        attrs = element.attrib
        if attrs.get('mode', '').lower() == 'm_axi' and 'latency' in attrs:
            matches.append({'path': str(path), 'kind': 'architecture_xml_bundle',
                            'bundle': attrs.get('name'), 'value': attrs['latency']})
for path in sorted(run.rglob('technology_out.xml')):
    try:
        root = ET.parse(path).getroot()
    except (ET.ParseError, OSError) as exc:
        raise SystemExit(f"cannot parse generated functional-unit XML {path}: {exc}")
    for element in root.iter('attribute'):
        if element.attrib.get('name') == 'bambu_axi_latency':
            matches.append({'path': str(path), 'kind': 'functional_unit_attribute',
                            'value': ''.join(element.itertext()).strip()})
if not matches:
    raise SystemExit('no generated architecture.xml m_axi latency or technology_out.xml bambu_axi_latency artifact found')
for match in matches:
    value = match['value']
    if not re.fullmatch(r'[0-9]+', value) or int(value) != expected:
        raise SystemExit(f"generated AXI latency mismatch/invalid value in {match['path']}: {value!r}, expected {expected}")
output.write_text(json.dumps({'expected': expected, 'values': matches}, indent=2) + '\n', encoding='utf-8')
PY
    then fail 'generated AXI latency metadata is absent, malformed, or differs from the requested profile'; fi
    if [[ $MODE == burst ]]; then
      if ! python3 -B - "$RTL_FILE" "$B" "$O" "$D" <<'PY'
import pathlib, re, sys

path = pathlib.Path(sys.argv[1])
expected = {"B_MAX": sys.argv[2], "MAX_OUTSTANDING": sys.argv[3], "FIFO_DEPTH": sys.argv[4]}
source = path.read_text(encoding="utf-8", errors="strict")
source = re.sub(r"/\*.*?\*/", "", source, flags=re.S)
source = re.sub(r"//[^\n]*", "", source)
instances = list(re.finditer(r"\bMinimalAXI4MasterPipelined\b", source))
if len(instances) != 1:
    raise SystemExit(f"expected exactly one MinimalAXI4MasterPipelined instance, found {len(instances)}")
position = instances[0].end()
match = re.match(r"\s*#\s*\(", source[position:])
if match is None:
    raise SystemExit("burst engine has no parameter association list")
opening = position + match.end() - 1
depth = 0
closing = None
for index in range(opening, len(source)):
    if source[index] == "(":
        depth += 1
    elif source[index] == ")":
        depth -= 1
        if depth == 0:
            closing = index
            break
if closing is None:
    raise SystemExit("burst engine parameter list is unterminated")
if re.match(r"\s+burst_engine\s*\(", source[closing + 1:]) is None:
    raise SystemExit("burst engine instance name is not burst_engine")
parameters = source[opening + 1:closing]
for name, wanted in expected.items():
    associations = list(re.finditer(rf"\.{re.escape(name)}\s*\(\s*([^()]*)\s*\)", parameters))
    if len(associations) != 1:
        raise SystemExit(f"expected exactly one {name} association, found {len(associations)}")
    actual = associations[0].group(1).strip()
    if not re.fullmatch(r"[0-9]+", actual) or actual != wanted:
        raise SystemExit(f"{name} is {actual!r}, expected {wanted}")
PY
      then fail "generated burst engine in $RTL_FILE is absent, ambiguous, or has a mismatched B/O/D profile"; fi
    else
      if rg -q 'MinimalAXI4MasterPipelined' "$RTL_FILE"; then fail "legacy mode unexpectedly generated a burst engine in $RTL_FILE"; fi
    fi
    if ! python3 -B "$PARAM_CHECKER" --vcd "$VCD" --manifest "$OUT/manifest.json" --latency "$L" --queue-size "$Q" \
      >"$OUT/effective-params.stdout.log" 2>"$OUT/effective-params.stderr.log"; then
      cat "$OUT/effective-params.stderr.log" >&2
      fail "effective AXI parameters in VCD do not match requested latency/queue; evidence preserved in $OUT"
    fi
    READ_DELAY_DERIVED=$((L - 1))
    CHECKER_MODE=legacy
    [[ $MODE == burst ]] && CHECKER_MODE=generated_burst
    if python3 -B "$CHECKER" --mode "$CHECKER_MODE" --vcd "$VCD" --json-out "$RESULT_JSON" \
      --n "$N" --b "$B" --o "$O" --d "$D" --declared-latency "$L" --read-delay "$READ_DELAY_DERIVED" \
      >"$OUT/checker.stdout.log" 2>"$OUT/checker.stderr.log"; then CHECKER_RC=0; else CHECKER_RC=$?; fi
    python3 -B - "$OUT/manifest.json" "$RESULT_JSON" "$CHECKER_RC" <<'PY'
import hashlib, json, pathlib, re, sys, xml.etree.ElementTree as ET
manifest_path, report_path, rc = sys.argv[1:]
manifest = json.loads(pathlib.Path(manifest_path).read_text(encoding="utf-8"))
report_file = pathlib.Path(report_path)
if report_file.is_file():
    report = json.loads(report_file.read_text(encoding="utf-8"))
    manifest["checker_results"] = report
    metrics = report.get("metrics", {})
    latency = metrics.get("L_axi")
    required = ("first_ar_accept_cycle", "last_rlast_accept_cycle", "C_stream",
                "first_start_cycle", "first_done_cycle")
    if manifest.get("mode") == "burst":
        required += ("C_configure_drain",)
    values = {key: metrics.get(key) for key in required}
    endpoint_scope = metrics.get("endpoint_scope")
    valid = report.get("status") == "PASS" and isinstance(latency, int) and 63 <= latency <= 65
    valid = valid and all(type(value) is int and value >= 0 for value in values.values())
    valid = valid and isinstance(endpoint_scope, str) and endpoint_scope.split("/")[-1] == "DUT"
    if valid:
        valid = values["first_ar_accept_cycle"] <= values["last_rlast_accept_cycle"]
        valid = valid and values["C_stream"] == values["last_rlast_accept_cycle"] - values["first_ar_accept_cycle"]
        valid = valid and values["first_start_cycle"] <= values["first_done_cycle"]
    stderr_path = pathlib.Path(manifest_path).parent / "bambu.stderr.log"
    try:
        stderr_text = stderr_path.read_text(encoding="utf-8", errors="strict")
        total_lines = [line for line in stderr_text.splitlines()
                       if re.fullmatch(r"\s*Total cycles\s*:\s*[0-9]+\s+cycles\s*", line)]
        totals = [int(re.search(r"Total cycles\s*:\s*([0-9]+)", line).group(1))
                  for line in total_lines]
        total = totals[0] if len(totals) == 1 else None
    except (OSError, UnicodeError, ValueError):
        total = None
    results_path = pathlib.Path(manifest_path).parent / "run/HLS_output/simulation/modelsim_backend/bambu_results.xml"
    try:
        sim = ET.parse(results_path).find(".//simulation")
        run_nodes = sim.findall("run") if sim is not None else []
        xml_total = int((run_nodes[0].text or "").strip()) if len(run_nodes) == 1 else None
    except (ET.ParseError, OSError, ValueError):
        xml_total = None
    endpoints_are_valid = (type(values.get("first_start_cycle")) is int and
                           type(values.get("first_done_cycle")) is int)
    vcd_total_inclusive = (values["first_done_cycle"] - values["first_start_cycle"] + 1
                           if endpoints_are_valid else None)
    valid = valid and endpoints_are_valid and isinstance(total, int) and total == vcd_total_inclusive
    if isinstance(xml_total, int):
        valid = valid and xml_total == total
    manifest["C_total"] = total
    manifest["C_total_source"] = str(stderr_path.relative_to(pathlib.Path(manifest_path).parent))
    manifest["C_total_xml_crosscheck"] = xml_total
    manifest["C_total_xml_matches"] = xml_total == total if isinstance(xml_total, int) and isinstance(total, int) else None
    manifest["C_total_vcd_inclusive_window"] = vcd_total_inclusive
    manifest["C_stream"] = values.get("C_stream")
    manifest["cycle_metrics"] = values
    latency_path = pathlib.Path(manifest_path).parent / "generated_latency.json"
    if latency_path.is_file():
        manifest["generated_latency"] = json.loads(latency_path.read_text(encoding="utf-8"))
    manifest["status"] = "CHECKER_PASS" if valid and rc == "0" else "INVALID_ENVIRONMENT_OR_CHECKER_FAILURE"
    manifest["checker_exit"] = int(rc)
    manifest["read_delay_derived"] = manifest["parameters"]["mem_delay_read"] - 1
    manifest["artifacts"].append({"path": str(report_file.relative_to(pathlib.Path(manifest_path).parent)),
                                  "bytes": report_file.stat().st_size,
                                  "sha256": hashlib.sha256(report_file.read_bytes()).hexdigest()})
pathlib.Path(manifest_path).write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
PY
    (( CHECKER_RC == 0 )) || fail "VCD checker failed; evidence preserved in $OUT"
    python3 -B - "$RESULT_JSON" <<'PY'
import json, sys
from pathlib import Path
report = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
latency = report.get("metrics", {}).get("L_axi")
if report.get("status") != "PASS" or not isinstance(latency, int) or not 63 <= latency <= 65:
    raise SystemExit(f"invalid checker result: status={report.get('status')!r}, L_axi={latency!r}, errors={report.get('errors')!r}")
PY
if ! python3 -B - "$OUT/manifest.json" <<'PY'
import json, pathlib, sys
manifest = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
if manifest.get("status") != "CHECKER_PASS":
    raise SystemExit("runner cycle-window or generated-latency cross-check failed")
PY
    then CHECKER_RC=4; fi
  fi
fi

# Preserve run provenance and every available output, even when Bambu itself
# returned a failure. The checker report is included when it was produced.
python3 -B - "$OUT" "$ROOT" "$TESTS" "$BAMBU" "$SETTINGS" "$MODELSIM_ROOT" \
  "$VSIM" "$CHECKER" "$SIMULATOR_VERSION" "$READ_DELAY_DERIVED" "$MODE" "$FIXTURE" "$B" "$O" "$D" "$L" "$N" "$Q" \
  "$AXI_TYPE" "$BAMBU_RC" "$CHECKER_RC" "$BAMBU_VERSION" "$PROFILE_JSON" \
  "$RESULT_JSON" <<'PY'
import hashlib, json, os, pathlib, shlex, subprocess, sys
(out, root, tests, bambu, settings, modelsim_root, vsim, checker, simulator_version,
 read_delay, mode, fixture, b, o, d, latency, n, q, axi_type, bambu_rc, checker_rc,
 bambu_version, profile_json, result_json) = sys.argv[1:]
def sha(path):
    digest = hashlib.sha256()
    with open(path, "rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()
profile = json.loads(pathlib.Path(profile_json).read_text(encoding="utf-8"))
command_text = pathlib.Path(out, "command.txt").read_text(encoding="utf-8")
argv = shlex.split(command_text)
prior_manifest = pathlib.Path(out, "manifest.json")
prior = json.loads(prior_manifest.read_text(encoding="utf-8")) if prior_manifest.is_file() else {}
payload = {
    **profile,
    "status": ("INVALID_ENVIRONMENT_OR_CHECKER_FAILURE"
                if prior.get("status") == "INVALID_ENVIRONMENT_OR_CHECKER_FAILURE"
                else "PASS" if bambu_rc == "0" and (mode == "scheduler-off" or checker_rc == "0")
                else "FAIL_OR_INCOMPLETE"),
    "mode": mode,
    "bambu": {"path": bambu, "sha256": sha(bambu), "version": bambu_version.strip()},
    "settings": {"path": settings, "sha256": sha(settings), "sourced": True},
    "simulator": {"root": modelsim_root, "path": vsim if mode != "scheduler-off" else None,
                  "version": simulator_version.strip(),
                  "sha256": sha(vsim) if mode != "scheduler-off" else None},
    "read_delay_derived": int(read_delay) if read_delay.isdecimal() else read_delay,
    "checker": {"path": checker if mode != "scheduler-off" else None,
                "sha256": sha(checker) if mode != "scheduler-off" else None},
    "repository": {"root": root, "revision": subprocess.check_output(["git", "-C", root, "rev-parse", "HEAD"], text=True).strip()},
    "device": "xcu250,-2L,figd2104",
    "parameters": {"B": int(b), "O": int(o), "D": int(d), "L": int(latency), "N": int(n), "Q": int(q),
                    "AXI_burst_type": axi_type, "clock_period_ns": 3.33,
                    "mem_delay_read": int(latency), "mem_delay_write": 2,
                    "memory_allocation_policy": "NO_BRAM", "distram_threshold": 0,
                    "experimental_m_axi_burst": 0 if mode == "legacy" else 1},
    "scratch": {key: os.environ.get(key) for key in ("TMPDIR", "TMP", "TEMP")},
    "testbench_parameter_sizes_bytes": {"src": 16384} if fixture in ("independent", "mulxor") else {},
    "occupancy_source": "axi_handshake_reconstruction",
    "bambu_argv": argv,
    "bambu_exit_code": int(bambu_rc), "checker_exit": checker_rc,
    "artifacts": [],
}
prior_manifest = pathlib.Path(out, "manifest.json")
if prior_manifest.is_file():
    for key in ("C_total", "C_total_source", "C_total_xml_crosscheck", "C_total_xml_matches",
                "C_total_vcd_inclusive_window",
                "C_stream", "cycle_metrics", "generated_latency", "descriptor_count_dumped",
                "descriptor_count_width", "descriptor_count_vcd_scope", "effective_hdl_parameters"):
        if key in prior:
            payload[key] = prior[key]
for path in sorted(pathlib.Path(out).rglob("*")):
    if path.is_file() and path.name != "manifest.json":
        payload["artifacts"].append({"path": str(path.relative_to(out)), "bytes": path.stat().st_size,
                                     "sha256": sha(str(path))})
if pathlib.Path(result_json).is_file():
    payload["checker_results"] = json.loads(pathlib.Path(result_json).read_text(encoding="utf-8"))
if payload["status"] != "INVALID_ENVIRONMENT_OR_CHECKER_FAILURE":
    payload["status"] = "PASS" if payload["status"] == "PASS" and (mode == "scheduler-off" or payload.get("checker_results", {}).get("status") == "PASS") else "FAIL_OR_INCOMPLETE"
pathlib.Path(out, "manifest.json").write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
PY

(( BAMBU_RC == 0 )) || fail "Bambu failed with rc=$BAMBU_RC; evidence preserved in $OUT"
if [[ $MODE != scheduler-off ]]; then
  [[ $CHECKER_RC == 0 ]] || fail "VCD validation failed; evidence preserved in $OUT"
fi
echo "T4B_${MODE}_PASS output=$OUT manifest=$OUT/manifest.json"
