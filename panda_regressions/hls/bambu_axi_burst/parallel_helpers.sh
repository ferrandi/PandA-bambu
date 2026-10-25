#!/usr/bin/env bash

# Parse the parallelism options accepted by mantis.py and normalize them to a
# single --parallel=N argument. Non-parallel arguments retain their order.
normalize_parallel_args() {
  local jobs= explicit_seen=0
  local -a passthrough=()
  local saw_separator=0 value
  while (($#)); do
    value=$1
    shift
    if (( saw_separator )); then
      passthrough+=("$value")
      continue
    fi
    if [[ $value == -- ]]; then
      saw_separator=1
      passthrough+=(--)
      continue
    fi

    local requested=
    case $value in
      -j|--parallel)
        if (($# == 0)) || [[ $1 == -- ]]; then
          echo "missing value for $value" >&2
          return 2
        fi
        requested=$1
        shift
        ;;
      -j=*) requested=${value#-j=} ;;
      --parallel=*) requested=${value#--parallel=} ;;
      -j?*) requested=${value#-j} ;;
      *) passthrough+=("$value"); continue ;;
    esac
    if [[ ! $requested =~ ^[0-9]+$ ]]; then
      echo "parallelism must be a positive integer (got '$requested')" >&2
      return 2
    fi
    while [[ $requested == 0* && ${#requested} -gt 1 ]]; do requested=${requested#0}; done
    if [[ $requested == 0 || ${#requested} -gt 10 ]] || { ((${#requested} == 10)) && [[ $requested > 2147483647 ]]; }; then
      echo "parallelism must be between 1 and 2147483647 (got '$requested')" >&2
      return 2
    fi
    requested=$((10#$requested))
    if (( ! explicit_seen )); then
      jobs=$requested
      explicit_seen=1
    elif (( requested < jobs )); then
      jobs=$requested
    fi
  done

  if (( ! explicit_seen )); then
    jobs=${J:-1}
    if [[ ! $jobs =~ ^[0-9]+$ ]]; then
      echo "J must be a positive integer (got '$jobs')" >&2
      return 2
    fi
    while [[ $jobs == 0* && ${#jobs} -gt 1 ]]; do jobs=${jobs#0}; done
    if [[ $jobs == 0 || ${#jobs} -gt 10 ]] || { ((${#jobs} == 10)) && [[ $jobs > 2147483647 ]]; }; then
      echo "J must be between 1 and 2147483647 (got '${J:-1}')" >&2
      return 2
    fi
    jobs=$((10#$jobs))
  fi

  BURST_PARALLELISM=$jobs
  BURST_NORMALIZED_ARGS=()
  local inserted=0 arg
  for arg in "${passthrough[@]}"; do
    if [[ $arg == -- ]] && (( ! inserted )); then
      BURST_NORMALIZED_ARGS+=("--parallel=$jobs" --)
      inserted=1
    else
      BURST_NORMALIZED_ARGS+=("$arg")
    fi
  done
  (( inserted )) || BURST_NORMALIZED_ARGS+=("--parallel=$jobs")
}

# Normalize the options the wrapper owns: the output root and the parallelism.
# The wrapper reserves one fresh output root, so Mantis receives exactly one
# resolved -o. Every other option is passed through to Mantis unchanged.
normalize_regression_args() {
  normalize_parallel_args "$@" || return $?
  BURST_RESTART=
  local -a args=() normalized=()
  local output="${PWD}/out_${OUT_SUFFIX:-bambu_axi_burst}"
  local i=0 value saw_separator=0
  args=("${BURST_NORMALIZED_ARGS[@]}")
  while (( i < ${#args[@]} )); do
    value=${args[i]}
    if (( saw_separator )); then
      normalized+=("$value")
      ((i += 1))
      continue
    fi
    if [[ $value == -- ]]; then
      normalized+=(--)
      saw_separator=1
      ((i += 1))
      continue
    fi
    case $value in
      --restart) BURST_RESTART=1; normalized+=("$value"); ((i += 1));;
      -o|--output)
        if (( i + 1 >= ${#args[@]} )) || [[ ${args[i+1]} == -- ]]; then
          echo "missing value for $value" >&2; return 2
        fi
        output=${args[i+1]}; ((i += 2));;
      --output=*) output=${value#--output=}; ((i += 1));;
      -o=*) output=${value#-o=}; ((i += 1));;
      -o?*) output=${value#-o}; ((i += 1));;
      *) normalized+=("$value"); ((i += 1));;
    esac
  done
  if [[ -z $output ]]; then
    echo "output directory must not be empty" >&2; return 2
  fi
  if [[ -L $output ]]; then
    echo "output path must not be a symlink: $output" >&2
    return 2
  fi
  BURST_OUTPUT=$(realpath -m -- "$output") || return 2
  BURST_NORMALIZED_ARGS=("${normalized[@]}")
}

cleanup_mantis_cases() {
  local out=$1
  python3 - "$out" <<'PY'
import os
import sys

root = sys.argv[1]
keep = {"execution.log", "timeout.log", "failure.log", "return_value", "bambu_results.xml", "test_report.xml"}
for current, dirs, files in os.walk(root, topdown=False):
    for name in files:
        if name not in keep:
            os.unlink(os.path.join(current, name))
    for name in dirs:
        path = os.path.join(current, name)
        if not os.listdir(path):
            os.rmdir(path)
PY
}

# Terminate callback descendants before their shells, including children started
# by nested runners. Process-tree inspection is only needed when a caller cancels.
_burst_cancel_job_tree() {
  local root pid ppid
  local -a tree=()
  local -A children=()
  ((${#pids[@]})) || return 0
  while read -r pid ppid; do
    children[$ppid]="${children[$ppid]-} $pid"
  done < <(ps -eo pid=,ppid=)

  _burst_collect_cancel_tree() {
    local parent=$1 nested
    for nested in ${children[$parent]-}; do
      _burst_collect_cancel_tree "$nested"
    done
    tree+=("$parent")
  }
  for root in "${pids[@]}"; do _burst_collect_cancel_tree "$root"; done

  for pid in "${tree[@]}"; do kill -TERM "$pid" 2>/dev/null || true; done
  sleep 1
  for pid in "${tree[@]}"; do kill -KILL "$pid" 2>/dev/null || true; done
  for root in "${pids[@]}"; do wait "$root" 2>/dev/null || true; done
}

# Run named jobs with a strict upper bound. callback receives each job name;
# logs are isolated and every started job is waited for, even after a failure.
run_bounded_jobs() {
  local limit=$1 callback=$2
  shift 2
  local -a pending=("$@") pids=()
  local -A labels=() log_paths=()
  local next=0 active=0 failed=0 pid done_pid status label log running_jobs
  local cancel_status=0 previous_int_trap previous_term_trap

  previous_int_trap=$(trap -p INT)
  previous_term_trap=$(trap -p TERM)
  trap 'if (( ! cancel_status )); then cancel_status=130; fi' INT
  trap 'if (( ! cancel_status )); then cancel_status=143; fi' TERM

  if [[ ${BURST_QUIET_JOB_STATUS:-0} != 1 ]]; then
    printf 'Starting %d burst helper jobs (parallel limit %s); details: %s/\n' \
      "${#pending[@]}" "$limit" "$BURST_JOB_LOG_DIR"
  fi

  while (( next < ${#pending[@]} || active > 0 )); do
    while (( ! failed && ! cancel_status && next < ${#pending[@]} && active < limit )); do
      label=${pending[next]}
      log="$BURST_JOB_LOG_DIR/$label.log"
      "$callback" "$label" >"$log" 2>&1 &
      pid=$!
      pids+=("$pid")
      labels[$pid]=$label
      log_paths[$pid]=$log
      ((next += 1))
      ((active += 1))
    done

    if (( cancel_status )); then
      break
    fi

    ((${#pids[@]})) || break
    # Bash 5.0 lacks wait's PID-capture option, so find a completed child
    # through the job table and then wait for that exact PID.
    # Include stopped jobs in the live set: wait must not mistake a stop for
    # completion. Poll briefly instead of waiting on the oldest PID, which
    # would serialize jobs that finish out of order.
    running_jobs=$(jobs -pr; jobs -ps)
    done_pid=
    for pid in "${pids[@]}"; do
      if [[ $'\n'${running_jobs}$'\n' != *$'\n'"$pid"$'\n'* ]]; then
        # A child may have changed state between the two `jobs` queries. Keep
        # it active if it still exists; a terminated child will be reaped by
        # Bash and become waitable on the next scan.
        if kill -0 "$pid" 2>/dev/null; then
          continue
        fi
        done_pid=$pid
        break
      fi
    done
    if [[ -z $done_pid ]]; then
      if ! sleep 0.05; then
        if (( ! cancel_status )); then failed=1; fi
      fi
      (( cancel_status )) && break
      continue
    fi
    if wait "$done_pid"; then
      status=0
    else
      status=$?
      failed=1
    fi
    (( cancel_status )) && break
    label=${labels[$done_pid]}
    log=${log_paths[$done_pid]}
    if (( status != 0 )); then
      printf 'FAIL: burst job %s (status %d), log: %s\n' "$label" "$status" "$log" >&2
      tail -n 80 "$log" >&2 || true
    elif [[ ${BURST_QUIET_JOB_STATUS:-0} != 1 ]]; then
      printf 'PASS: burst job %s\n' "$label"
    fi
    unset 'labels[$done_pid]' 'log_paths[$done_pid]'
    local -a remaining=()
    for pid in "${pids[@]}"; do
      [[ $pid == "$done_pid" ]] || remaining+=("$pid")
    done
    pids=("${remaining[@]}")
    active=$((active - 1))
  done

  if (( cancel_status )); then
    trap '' INT TERM
    _burst_cancel_job_tree
  fi
  if [[ -n $previous_int_trap ]]; then eval "$previous_int_trap"; else trap - INT; fi
  if [[ -n $previous_term_trap ]]; then eval "$previous_term_trap"; else trap - TERM; fi
  if (( cancel_status == 130 )) && [[ -n $previous_int_trap ]]; then
    kill -INT "$BASHPID"
  elif (( cancel_status == 143 )) && [[ -n $previous_term_trap ]]; then
    kill -TERM "$BASHPID"
  fi
  (( cancel_status )) && return "$cancel_status"
  return "$failed"
}
