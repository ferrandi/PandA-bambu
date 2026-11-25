#!/bin/bash

script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/generic_getopt.sh

BATCH_ARGS=("-lm" "--simulate" "--experimental-setup=BAMBU" "--expose-globals" "--channels-type=MEM_ACC_NN" "--memory-allocation-policy=EXT_PIPELINED_BRAM" "--std=gnu89")
OUT_SUFFIX="${compiler}_eg_extp"

# --- Phase-C keep-cell mode (t4, owner: test-worker) --------------
# With --keep-cell=NAME the authoritative keep-list cell decides which
# benchmarks run and which single -O arm runs: the keep-list is the
# single source of truth, and matrix args that disagree are refused.
# The coverage-lane per-test watchdog (cov_run.py --timeout), the
# mantis per-tool-execution -t, and GitHub per-job timeout-minutes are
# different axes; none is conflated here. Without --keep-cell the
# behavior is byte-identical to the pre-block script.
KEEP_CELL=""
KEEP_LIST="$script_dir/../../coverage-analysis/keep_lists_draft.json"
KEEP_EXPECT=""
KEEP_ARGS=()
IFS=' ' read -r -a RAW_ARGS <<< "$ARGS"
for (( i=0; i<${#RAW_ARGS[@]}; i++ )); do
   a="${RAW_ARGS[i]}"
   case "$a" in
      --keep-cell=*) KEEP_CELL="${a#--keep-cell=}";;
      --keep-cell)   KEEP_CELL="${RAW_ARGS[++i]}";;
      --keep-list=*) KEEP_LIST="${a#--keep-list=}";;
      --keep-list)   KEEP_LIST="${RAW_ARGS[++i]}";;
      --expect-sha256=*) KEEP_EXPECT="${a#--expect-sha256=}";;
      --expect-sha256)   KEEP_EXPECT="${RAW_ARGS[++i]}";;
      *) KEEP_ARGS+=("$a");;
   esac
done
ARGS="${KEEP_ARGS[*]}"

if [[ -n "$KEEP_CELL" ]]; then
   LISTFILE="${PWD}/keeplist_${KEEP_CELL}.txt"
   HELPER_ARGS=(--keep-list "$KEEP_LIST" --cell "$KEEP_CELL"
                --out "$LISTFILE" --prefix "gcc_regression_simple/")
   if [[ -n "$KEEP_EXPECT" ]]; then
      HELPER_ARGS+=(--expect-sha256 "$KEEP_EXPECT")
   fi
   SEL_OUT=$(python3 "$script_dir/keep_cell_select.py" "${HELPER_ARGS[@]}")
   SEL_RC=$?
   printf '%s\n' "$SEL_OUT"
   if [[ $SEL_RC -ne 0 ]]; then exit 2; fi
   OPT=$(printf '%s\n' "$SEL_OUT" | sed -n 's/^CELL .* opt=\([^ ]*\).*/\1/p')
   C_SETUP=$(printf '%s\n' "$SEL_OUT" | sed -n 's/^CELL .* setup=\([^ ]*\).*/\1/p')
   C_SIM=$(printf '%s\n' "$SEL_OUT" | sed -n 's/^CELL .* simulator=\([^ ]*\).*/\1/p')
   M_SETUP=$(printf '%s' "${compiler}" | sed 's/^I386_//;s/^CLANG/clang-/')
   # Version-axis equivalence: I386_CLANG4 is the matrix name of cell
   # setup clang-4.0 (same for 6.0); the .0 suffix is normalized before
   # the digit comparison, nothing else is forgiven.
   C_SETUP_N=$(printf '%s' "$C_SETUP" | sed 's/\.0$//')
   M_SETUP_N=$(printf '%s' "$M_SETUP" | sed 's/\.0$//')
   if [[ -n "$C_SETUP" && "$C_SETUP" != "$M_SETUP" \
         && "$C_SETUP_N" != "$M_SETUP_N" ]]; then
      echo "REFUSE: matrix compiler=${compiler} (->$M_SETUP) != cell setup=$C_SETUP"
      exit 2
   fi
   M_SIM=$(printf '%s\n' "$ARGS" | sed -n 's/.*-c=--simulator=\([^ ]*\).*/\1/p')
   if [[ -n "$M_SIM" && -n "$C_SIM" && "$M_SIM" != "$C_SIM" ]]; then
      echo "REFUSE: matrix simulator=$M_SIM != cell simulator=$C_SIM"
      exit 2
   fi
   if [[ -z "$OPT" ]]; then
      echo "REFUSE: opt unparsed for cell $KEEP_CELL"
      exit 2
   fi
   MANTIS_CMD=(python3 $script_dir/../../etc/scripts/mantis.py --tool=bambu
                 --benchmarks-list="$LISTFILE"
                 --args="--configuration-name=${compiler}_${OPT#-} ${OPT} ${BATCH_ARGS[*]}"
                 -o "out_${OUT_SUFFIX}" -b$script_dir)
   if [[ -n "$ARGS" ]]; then MANTIS_CMD+=($ARGS); fi
   LIST_TOKENS=0
   for tok in "${MANTIS_CMD[@]}"; do
      case "$tok" in
         --benchmarks-list*|-l*) LIST_TOKENS=$((LIST_TOKENS+1));;
      esac
   done
   if [[ $LIST_TOKENS -ne 1 ]]; then
      echo "REFUSE: benchmarks-list token count=$LIST_TOKENS expect"
      echo "REFUSE: 1 -- argparse -l/--benchmarks-list is ONE store,"
      echo "REFUSE: last token wins (Lead meter, wave 226)"
      exit 2
   fi
   if [[ "${KEEP_CELL_SELFCHECK:-0}" == "1" ]]; then
   echo "SELF-CHECK mantis ${MANTIS_CMD[*]}"
      exit 0
   fi
   "${MANTIS_CMD[@]}"
   exit $?
fi


python3 $script_dir/../../etc/scripts/mantis.py gcc_regression_simple --tool=bambu \
   --args="--configuration-name=${compiler}_O0 -O0 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${compiler}_O1 -O1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${compiler}_O2 -O2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${compiler}_O3 -O3 ${BATCH_ARGS[*]}" \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   $ARGS
