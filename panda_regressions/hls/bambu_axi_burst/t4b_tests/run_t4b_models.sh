#!/usr/bin/env bash
set -Eeuo pipefail

TESTS=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
ROOT=$(git -C "$TESTS" rev-parse --show-toplevel)
OUT_ARG=""
PREPARE_ONLY=0

usage() {
  cat <<'EOF'
Usage: run_t4b_models.sh [--prepare-only] FRESH_OUT

Extracts the original MinimalAXI4MasterPipelined and TestbenchMEMAXI cells,
then under ModelSim runs the DUT-less N1 memory characterization and N1b
engine-plus-matched-slave floor/knee matrix. N1b uses B in {1,16}, D=256,
N in {1024,2048}, two repeats, and O in {1,2,3,4,5,6,8,12,16} for B=16.

FRESH_OUT must be a new directory named t4b-* below <repo>/documentation/tmp.
ModelSim execution must be launched outside the Codex sandbox with
BURST_MODELSIM_OUTSIDE_SANDBOX=1. MODELSIM_ROOT defaults to /opt/mentor.
--prepare-only extracts and hashes the HDL and writes a run plan without
invoking ModelSim; it is intended for offline review/tests.
EOF
}

while (($#)); do
  case "$1" in
    --prepare-only) PREPARE_ONLY=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *)
      [[ -z $OUT_ARG ]] || { usage >&2; exit 2; }
      OUT_ARG=$1; shift ;;
  esac
done
[[ -n $OUT_ARG ]] || { usage >&2; exit 2; }

[[ ! -L $OUT_ARG ]] || { echo "refusing symlink output path: $OUT_ARG" >&2; exit 2; }
OUT=$(realpath -m -- "$OUT_ARG")
case "$OUT" in
  "$ROOT/documentation/tmp/t4b-"*) ;;
  *) echo "FRESH_OUT must be named t4b-* under $ROOT/documentation/tmp" >&2; exit 2 ;;
esac
[[ ! -e $OUT ]] || { echo "refusing to reuse existing output: $OUT" >&2; exit 2; }
mkdir -p "$ROOT/documentation/tmp"
mkdir "$OUT"
mkdir -p "$OUT/tmp" "$OUT/hdl" "$OUT/logs"
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp"

python3 "$TESTS/extract_t4b_cells.py" --outdir "$OUT/hdl" >"$OUT/extract.log"

MODELSIM_ROOT=${MODELSIM_ROOT:-/opt/mentor}
VSIM="$MODELSIM_ROOT/questasim/bin/vsim"
VLOG="$MODELSIM_ROOT/questasim/bin/vlog"
VLIB="$MODELSIM_ROOT/questasim/bin/vlib"
VMAP="$MODELSIM_ROOT/questasim/bin/vmap"

python3 - "$ROOT" "$TESTS" "$OUT" "$MODELSIM_ROOT" "$PREPARE_ONLY" <<'PY'
import hashlib
import json
import sys
from pathlib import Path

root, tests, out, modelsim_root, prepare_only = map(str, sys.argv[1:])
root, tests, out = Path(root), Path(tests), Path(out)
inputs = [
    root / "etc/libtech/NC_AXI_IPs.xml",
    root / "etc/libtech/NC_TESTBENCH_IPs.xml",
    tests / "extract_t4b_cells.py",
    tests / "t4b_memory_utils.sv",
    tests / "tb_t4b_slave.sv",
    tests / "tb_t4b_floor.sv",
    tests / "run_t4b_models.sh",
]
hashes = {}
for path in inputs + sorted((out / "hdl").glob("*.sv")):
    hashes[str(path.relative_to(root)) if path.is_relative_to(root) else str(path.relative_to(out))] = hashlib.sha256(path.read_bytes()).hexdigest()

matrix_o = [1, 2, 3, 4, 5, 6, 8, 12, 16]
matrix = []
for n in (1024, 2048):
    for b in (1, 16):
        os = matrix_o if b == 16 else [1, 4, 5, 8, 16]
        for o in os:
            for repeat in (1, 2):
                matrix.append({"B": b, "O": o, "D": 256, "N": n,
                               "L": 64, "READ_DELAY": 63, "QUEUE_SIZE": 33,
                               "repeat": repeat})

manifest = {
    "purpose": "T4B H1 N1/N1b ModelSim characterization",
    "simulator": str(Path(modelsim_root) / "questasim/bin/vsim"),
    "prepare_only": prepare_only == "1",
    "n1": {"READ_DELAY": 63, "QUEUE_SIZE": 33, "N_REQUESTS": 16,
           "required_ar_before_r_handshake": 16, "measured_latency_cycles": [63, 65]},
    "n1b": {"L": 64, "READ_DELAY": 63, "QUEUE_SIZE": 33,
            "matrix": matrix, "repeats": 2,
            "knee_o_sweep": matrix_o, "base_address": "0x00001000"},
    "input_sha256": hashes,
}
(out / "run_manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
with (out / "planned_runs.tsv").open("w") as stream:
    stream.write("kind\tB\tO\tD\tN\tL\tREAD_DELAY\tQUEUE_SIZE\trepeat\n")
    stream.write("N1\t-\t-\t-\t16\t64\t63\t33\t1\n")
    for row in matrix:
        stream.write("N1b\t{B}\t{O}\t{D}\t{N}\t{L}\t{READ_DELAY}\t{QUEUE_SIZE}\t{repeat}\n".format(**row))
PY

if ((PREPARE_ONLY)); then
  echo "prepared; ModelSim was not invoked: $OUT"
  exit 0
fi

[[ ${BURST_MODELSIM_OUTSIDE_SANDBOX:-0} == 1 ]] || {
  echo "refusing ModelSim execution without BURST_MODELSIM_OUTSIDE_SANDBOX=1; rerun outside the sandbox" >&2
  exit 2
}
for tool in "$VSIM" "$VLOG" "$VLIB" "$VMAP"; do
  [[ -x $tool ]] || { echo "ModelSim tool unavailable: $tool" >&2; exit 2; }
done

{
  printf 'vsim=%s\nvlog=%s\n' "$VSIM" "$VLOG"
  "$VSIM" -version 2>&1 | head -n 4
  sha256sum "$VSIM" "$VLOG" "$VLIB" "$VMAP"
} > "$OUT/modelsim_provenance.txt"

cd "$OUT"
"$VLIB" work >"$OUT/logs/vlib.log" 2>&1
"$VMAP" work "$OUT/work" >"$OUT/logs/vmap.log" 2>&1
"$VLOG" -sv \
  "$OUT/hdl/MinimalAXI4MasterPipelined.sv" \
  "$OUT/hdl/TestbenchMEMAXI.sv" \
  "$TESTS/t4b_memory_utils.sv" \
  "$TESTS/tb_t4b_slave.sv" \
  "$TESTS/tb_t4b_floor.sv" >"$OUT/logs/vlog.log" 2>&1 || {
    tail -n 120 "$OUT/logs/vlog.log" >&2
    exit 1
  }

run_sim() {
  local top=$1 log=$2; shift 2
  "$VSIM" -c "$@" "work.$top" -do 'run -all; quit -f' >"$log" 2>&1 || {
    tail -n 100 "$log" >&2
    return 1
  }
  rg -q 'T4B_(N1|N1B) PASS' "$log" || {
    echo "missing PASS summary in $log" >&2
    tail -n 100 "$log" >&2
    return 1
  }
}

run_sim tb_t4b_slave "$OUT/logs/n1-slave.log" \
  -gREAD_DELAY=63 -gQUEUE_SIZE=33 -gN_REQUESTS=16

run_floor_case() {
  local b=$1 o=$2 n=$3 repeat=$4
  local label="n1b-B${b}-O${o}-D256-N${n}-L64-r${repeat}"
  run_sim tb_t4b_floor "$OUT/logs/${label}.log" \
    -gB_MAX="$b" -gMAX_OUTSTANDING="$o" -gFIFO_DEPTH=256 \
    -gN="$n" -gAXI_LATENCY=64 -gREAD_DELAY=63 -gQUEUE_SIZE=33
}

for n in 1024 2048; do
  for o in 1 2 3 4 5 6 8 12 16; do
    for repeat in 1 2; do run_floor_case 16 "$o" "$n" "$repeat"; done
  done
  for o in 1 4 5 8 16; do
    for repeat in 1 2; do run_floor_case 1 "$o" "$n" "$repeat"; done
  done
done

python3 - "$OUT" <<'PY'
import re
import sys
from pathlib import Path

out = Path(sys.argv[1])
records = {}
pattern = re.compile(r"T4B_N1B PASS B=16 O=(\d+) D=256 N=(\d+) L=64 READ_DELAY=63 Q=33 C_consume=(\d+) C_drain=(\d+)")
for log in (out / "logs").glob("n1b-B16-*.log"):
    match = pattern.search(log.read_text(errors="replace"))
    if match:
        o, n, _consume, drain = map(int, match.groups())
        records.setdefault((n, o), []).append(drain)
per_n_knees = {}
for n in (1024, 2048):
    stats = {o: (sum(records[(n, o)]) / 2, max(records[(n, o)]) - min(records[(n, o)]))
             for o in (1, 2, 3, 4, 5, 6, 8, 12, 16)}
    knee = None
    for o in sorted(value for value in stats if value < 16):
        larger = [(other, mean) for other, (mean, _spread) in stats.items() if other > o]
        best = min(mean for _other, mean in larger)
        best_o = max(other for other, mean in larger if mean == best)
        if abs(stats[o][0] - best) <= max(stats[o][1], stats[best_o][1]):
            knee = o
            break
    per_n_knees[n] = knee
additions = []
if all(value is not None for value in per_n_knees.values()):
    common_knee = max(per_n_knees.values())
    for n in (1024, 2048):
        for b, base_os in ((1, {1, 4, 5, 8, 16}), (16, {1, 2, 3, 4, 5, 6, 8, 12, 16})):
            required = {common_knee}
            if common_knee > 1:
                required.add(common_knee - 1)
            for o in sorted(required - base_os):
                additions.append((b, n, o))
with (out / "knee_extension.tsv").open("w") as stream:
    stream.write("B\tN\tO\n")
    for b, n, o in additions:
        stream.write(f"{b}\t{n}\t{o}\n")
PY

while IFS=$'\t' read -r b n o; do
  [[ $b == B ]] && continue
  for repeat in 1 2; do
    run_floor_case "$b" "$o" "$n" "$repeat"
    printf 'N1b\t%s\t%s\t256\t%s\t64\t63\t33\t%s\n' "$b" "$o" "$n" "$repeat" >> "$OUT/planned_runs.tsv"
  done
done < "$OUT/knee_extension.tsv"

python3 - "$OUT" <<'PY'
import json
import re
import sys
from pathlib import Path

out = Path(sys.argv[1])
records = []
for log in sorted((out / "logs").glob("n1b-*.log")):
    match = re.search(
        r"T4B_N1B PASS B=(\d+) O=(\d+) D=(\d+) N=(\d+) L=(\d+) READ_DELAY=(\d+) Q=(\d+) "
        r"C_consume=(\d+) C_drain=(\d+) C_engine_floor=(\d+) n_desc_min=(\d+) n_desc=(\d+) "
        r"outstanding_peak=(\d+) L_axi=(\d+) reservations=(\d+)",
        log.read_text(errors="replace"))
    if not match:
        raise SystemExit(f"missing result fields in {log}")
    fields = list(map(int, match.groups()))
    records.append(dict(zip(("B", "O", "D", "N", "L", "READ_DELAY", "Q", "C_consume",
                             "C_drain", "C_engine_floor", "n_desc_min", "n_desc", "outstanding_peak",
                             "L_axi", "reservations"), fields)) | {"log": log.name})

with (out / "n1b_results.tsv").open("w") as stream:
    stream.write("B\tO\tD\tN\tL\tREAD_DELAY\tQ\trepeat_log\tC_consume\tC_drain\tC_engine_floor\tn_desc_min\tn_desc\toutstanding_peak\tL_axi\treservations\n")
    for row in records:
        stream.write("{B}\t{O}\t{D}\t{N}\t{L}\t{READ_DELAY}\t{Q}\t{log}\t{C_consume}\t{C_drain}\t{C_engine_floor}\t{n_desc_min}\t{n_desc}\t{outstanding_peak}\t{L_axi}\t{reservations}\n".format(**row))

knees = {}
for n in (1024, 2048):
    by_o = {}
    for row in records:
        if row["B"] == 16 and row["N"] == n and row["O"] in {1, 2, 3, 4, 5, 6, 8, 12, 16}:
            by_o.setdefault(row["O"], []).append(row["C_drain"])
    if any(len(vals) != 2 for vals in by_o.values()) or set(by_o) != {1, 2, 3, 4, 5, 6, 8, 12, 16}:
        raise SystemExit(f"incomplete knee sweep N={n}: {by_o}")
    stats = {o: (sum(vals) / len(vals), max(vals) - min(vals)) for o, vals in by_o.items()}
    knee = None
    for o in sorted(value for value in stats if value < 16):
        larger = [(other, mean) for other, (mean, _spread) in stats.items() if other > o]
        best_mean = min(mean for _other, mean in larger)
        best_o = max(other for other, mean in larger if mean == best_mean)
        if abs(stats[o][0] - best_mean) <= max(stats[o][1], stats[best_o][1]):
            knee = o
            break
    knees[str(n)] = {"O_knee": knee, "stats": {str(o): {"mean_C_drain": mean, "repeat_spread": spread}
                                               for o, (mean, spread) in stats.items()}}
common_plateau = all(item["O_knee"] is not None for item in knees.values())
measured_knee = max(item["O_knee"] for item in knees.values()) if common_plateau else None
summary = {"status": "PASS", "runs": len(records), "N1": (out / "logs/n1-slave.log").name,
           "knees": knees, "common_plateau": common_plateau,
           "O_knee_measured": measured_knee}
(out / "n1b_knee.json").write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n")
manifest_path = out / "run_manifest.json"
manifest = json.loads(manifest_path.read_text())
manifest["n1b"]["knee_results"] = knees
manifest["n1b"]["O_knee_measured"] = summary["O_knee_measured"]
manifest["n1b"]["common_plateau"] = common_plateau
manifest["n1b"]["knee_bracket_extension"] = [
    {"B": int(row.split("\t")[0]), "N": int(row.split("\t")[1]), "O": int(row.split("\t")[2])}
    for row in (out / "knee_extension.tsv").read_text().splitlines()[1:] if row.strip()
]
manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
print(json.dumps(summary, sort_keys=True))
PY

echo "H1 ModelSim harness complete: $OUT"
