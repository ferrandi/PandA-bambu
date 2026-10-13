#!/usr/bin/env bash
# Serial N4b witness/parity matrix. Each case delegates Bambu lifecycle and
# evidence capture to run_t4b_matched.sh.
set -euo pipefail
usage() { echo 'Usage: run_t4b_n4b_matrix.sh FRESH_OUT' >&2; exit 2; }
fail() { echo "run_t4b_n4b_matrix: $*" >&2; exit 2; }
[[ $# == 1 ]] || usage
[[ ${BURST_MODELSIM_OUTSIDE_SANDBOX:-0} == 1 ]] || fail 'ModelSim matrix requires BURST_MODELSIM_OUTSIDE_SANDBOX=1 (run outside the sandbox)'
HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
ROOT=$(git -C "$HERE" rev-parse --show-toplevel)
TMP_ROOT="$ROOT/documentation/tmp"
mkdir -p "$TMP_ROOT"
TMP_ROOT=$(realpath "$TMP_ROOT")
OUT_ARG=$1
OUT_PARENT=$(realpath -m "$(dirname "$OUT_ARG")")
OUT="$OUT_PARENT/$(basename "$OUT_ARG")"
case "$OUT/" in "$TMP_ROOT/"*) ;; *) fail "output must be below $TMP_ROOT" ;; esac
[[ "$OUT" != "$TMP_ROOT" ]] || fail 'output must be a fresh child directory of documentation/tmp'
[[ ! -e "$OUT" ]] || fail "refusing existing output: $OUT"
mkdir -p "$OUT_PARENT"
CASE_RUNNER=${T4B_N4B_CASE_RUNNER:-$HERE/run_t4b_matched.sh}
[[ -x "$CASE_RUNNER" ]] || fail "case runner is not executable: $CASE_RUNNER"
export TMPDIR="$TMP_ROOT" TMP="$TMP_ROOT" TEMP="$TMP_ROOT"
mkdir "$OUT"
mkdir "$OUT/cases"
MANIFEST="$OUT/manifest.tsv"
SUMMARY="$OUT/summary.tsv"
HEADER=$'case_id\tfixture\tN\tcomparison\tmode\tstatus\tB_requested\tO_requested\tD_requested\tB_applied\tO_applied\tD_applied\tL\tQ\tC_total\tC_stream\tchannel_latency\tcorrectness\tfirst_wait_cycles\texposed_wait_cycles\tstate_stall_cycles\toutstanding_peak\tn_desc\tIR_index\tDOT_index\tXML_index'
printf '%s\n' "$HEADER" > "$MANIFEST"
printf '%s\n' "$HEADER" > "$SUMMARY"
run_matrix() {
case_num=0
for fixture in independent mulxor; do
  for n in 1024 2048; do
    for o in 1 4 5 8 16; do
      case_num=$((case_num + 1))
      run_case "${fixture}_N${n}_burst_O${o}" "$fixture" "$n" burst "$o"
    done
    case_num=$((case_num + 1))
    run_case "${fixture}_N${n}_legacy_O16" "$fixture" "$n" legacy 16
    for o in 5 8 16; do
      case_num=$((case_num + 1))
      run_case "${fixture}_N${n}_scheduler-off_O${o}" "$fixture" "$n" scheduler-off "$o"
    done
  done
done
[[ $case_num == 36 ]] || fail "internal matrix error: expected 36 cases, ran $case_num"
echo "T4B_N4B_PASS cases=$case_num output=$OUT"
}

run_case() {
  local case_id=$1 fixture=$2 n=$3 mode=$4 o=$5
  local b=16 d=256 l=64 q=32 case_dir="$OUT/cases/$case_id"
  local -a args=("$b" "$o" "$d" "$l" "$n" "$q" "$case_dir" --fixture "$fixture" --mode "$mode" --axi-burst-type INCR)
  local child_rc=0
  if "$CASE_RUNNER" "${args[@]}" >"$OUT/${case_id}.wrapper.stdout.log" 2>"$OUT/${case_id}.wrapper.stderr.log"; then :; else child_rc=$?; fi
  if (( child_rc != 0 )); then
    append_failure "$case_id" "$fixture" "$n" "$mode" "$o" FAIL
    fail "case $case_id failed with rc=$child_rc; partial evidence preserved in $OUT"
  fi
  if [[ ! -s "$case_dir/manifest.json" ]]; then
    append_failure "$case_id" "$fixture" "$n" "$mode" "$o" INVALID
    fail "case $case_id has no manifest.json; partial evidence preserved in $OUT"
  fi
  local row_file="$OUT/$case_id.row.tsv"
  if ! python3 -B - "$case_dir/manifest.json" "$mode" "$fixture" "$n" "$o" "$row_file" <<'PY'
import csv, json, pathlib, sys
manifest_path, mode, fixture, n, o, row_path = sys.argv[1:]
try:
    doc = json.loads(pathlib.Path(manifest_path).read_text(encoding="utf-8"))
except (OSError, UnicodeError, json.JSONDecodeError) as exc:
    raise SystemExit(f"cannot read case manifest: {exc}")
if doc.get("mode") != mode or doc.get("status") != "PASS":
    raise SystemExit(f"unexpected manifest mode/status: {doc.get('mode')!r}/{doc.get('status')!r}")
if doc.get("fixture") != fixture:
    raise SystemExit(f"manifest fixture {doc.get('fixture')!r} does not match requested {fixture!r}")
p = doc.get("parameters", {})
for key, expected in (("B", 16), ("O", int(o)), ("D", 256), ("L", 64), ("N", int(n)), ("Q", 32)):
    if type(p.get(key)) is not int or p[key] != expected:
        raise SystemExit(f"manifest parameter {key}={p.get(key)!r}, expected {expected}")
for key, expected in (("AXI_burst_type", "INCR"), ("mem_delay_read", 64),
                      ("mem_delay_write", 2), ("memory_allocation_policy", "NO_BRAM"),
                      ("distram_threshold", 0),
                      ("experimental_m_axi_burst", 0 if mode == "legacy" else 1)):
    if p.get(key) != expected or (isinstance(expected, int) and type(p.get(key)) is not int):
        raise SystemExit(f"manifest parameter {key}={p.get(key)!r}, expected {expected!r}")
clock = p.get("clock_period_ns")
if isinstance(clock, bool) or not isinstance(clock, (int, float)) or clock != 3.33:
    raise SystemExit(f"manifest clock_period_ns={clock!r}, expected 3.33")
checker = doc.get("checker_results") or {}
metrics = checker.get("metrics") or {}
def required_uint(obj, key, *, allow_none=False):
    if key not in obj:
        raise SystemExit(f"required metric {key} is missing")
    value = obj.get(key)
    if allow_none and value is None:
        return None
    if type(value) is not int or value < 0:
        raise SystemExit(f"required metric {key} is missing or invalid: {value!r}")
    return value
if mode == "burst":
    if checker.get("status") != "PASS": raise SystemExit("burst case lacks PASS checker evidence")
    c_total = required_uint(doc, "C_total")
    c_stream = required_uint(doc, "C_stream")
    latency = required_uint(metrics, "L_axi")
    first_wait = required_uint(metrics, "first_wait_cycles")
    exposed_wait = required_uint(metrics, "exposed_wait_cycles")
    state_stall = required_uint(metrics, "state_stall_cycles", allow_none=True)
    peak = required_uint(metrics, "outstanding_peak")
    descriptors = required_uint(metrics, "n_desc")
    if not 63 <= latency <= 65: raise SystemExit(f"unexpected channel latency: {latency}")
    vals = [c_total, c_stream, latency, "PASS", first_wait, exposed_wait,
            "unavailable" if state_stall is None else state_stall, peak, descriptors]
elif mode == "legacy":
    if checker.get("status") != "PASS": raise SystemExit("legacy case lacks PASS checker/correctness evidence")
    c_total = required_uint(doc, "C_total")
    c_stream = required_uint(doc, "C_stream")
    latency = required_uint(metrics, "L_axi")
    if not 63 <= latency <= 65: raise SystemExit(f"unexpected channel latency: {latency}")
    vals = [c_total, c_stream, latency, "PASS", "", "", "", "", ""]
else:
    if checker: raise SystemExit("scheduler-off manifest unexpectedly contains checker metrics")
    vals = ["", "", "", "NOT_SIMULATED", "", "", "", "", ""]
def cell(v):
    if v is None: return ""
    return str(v).replace("\t", " ").replace("\r", " ").replace("\n", " ")
applied = mode != "legacy"
row = [f"{fixture}_N{n}_{mode}_O{o}", fixture, n,
       "burst-versus-legacy" if mode == "legacy" else mode, mode, "PASS",
       16, o, 256, 16 if applied else "not_applied", o if applied else "not_applied",
       256 if applied else "not_applied", 64, 32, *vals, "", "", ""]
with pathlib.Path(row_path).open("w", encoding="utf-8", newline="") as stream:
    csv.writer(stream, delimiter="\t", lineterminator="\n").writerow([cell(v) for v in row])
PY
  then
    append_failure "$case_id" "$fixture" "$n" "$mode" "$o" INVALID
    fail "case $case_id manifest validation failed; partial evidence preserved in $OUT"
  fi
  if [[ $mode == scheduler-off ]]; then
    local index="$case_dir/capture_index.tsv"
    if ! python3 -B - "$case_dir/run" "$index" <<'PY'
import pathlib, sys
root, output = map(pathlib.Path, sys.argv[1:])
groups = {"IR": ("*.bambuir", "*.ir"), "DOT": ("*.dot", "*.gv"), "XML": ("*.xml",)}
found = {kind: sorted({p.relative_to(root).as_posix() for pattern in pats for p in root.rglob(pattern) if p.is_file()})
         for kind, pats in groups.items()}
missing = [kind for kind, paths in found.items() if not paths]
if missing: raise SystemExit("scheduler-off capture lacks index entries for " + ", ".join(missing))
with output.open("w", encoding="utf-8") as stream:
    stream.write("kind\tpath\n")
    for kind in ("IR", "DOT", "XML"):
        for path in found[kind]: stream.write(f"{kind}\t{path}\n")
PY
    then
      append_failure "$case_id" "$fixture" "$n" "$mode" "$o" INVALID
      fail "scheduler-off case $case_id lacks required IR/DOT/XML indices; partial evidence preserved in $OUT"
    fi
    python3 -B - "$row_file" "$index#IR" "$index#DOT" "$index#XML" <<'PY'
import csv, pathlib, sys
row_path, *items = sys.argv[1:]
with pathlib.Path(row_path).open(encoding="utf-8", newline="") as stream: row = next(csv.reader(stream, delimiter="\t"))
row[-3:] = items
with pathlib.Path(row_path).open("w", encoding="utf-8", newline="") as stream:
    csv.writer(stream, delimiter="\t", lineterminator="\n").writerow(row)
PY
  fi
  cat "$row_file" >> "$MANIFEST"
  cat "$row_file" >> "$SUMMARY"
}

append_failure() {
  local id=$1 fixture=$2 n=$3 mode=$4 o=$5 status=$6
  local comparison=$mode b_applied=16 o_applied=$o d_applied=256
  if [[ $mode == legacy ]]; then comparison=burst-versus-legacy; b_applied=not_applied; o_applied=not_applied; d_applied=not_applied; fi
  printf '%s\t%s\t%s\t%s\t%s\t%s\t16\t%s\t256\t%s\t%s\t%s\t64\t32\t\t\t\t\t\t\t\t\t\t\t\n' \
    "$id" "$fixture" "$n" "$comparison" "$mode" "$status" "$o" "$b_applied" "$o_applied" "$d_applied" >> "$MANIFEST"
  tail -n 1 "$MANIFEST" >> "$SUMMARY"
}

run_matrix
