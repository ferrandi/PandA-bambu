#!/bin/sh -e

WORKDIR="$1"
shift
CMD="$@"

export J="${J:-1}"
export OMP_NUM_THREADS="${OMP_NUM_THREADS:-1}"
echo "Parallel jobs    : ${J}"
echo "Parallel threads : ${OMP_NUM_THREADS}"
echo "Working directory: ${WORKDIR}"
echo "User command line: ${CMD}"

cd "${WORKDIR}"
eval "${CMD}"
