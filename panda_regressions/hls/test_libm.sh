#!/bin/bash -l
script_dir="$(dirname $(readlink -e $0))"
out_dir="${PWD}/output_test_libm"
export OMP_NUM_THREADS="${J:-1}"

TEST_FUNCTIONS=${TEST_FUNCTIONS:-"hls_ext/expf hls_ext/logf hls_ext/powf hls/sinecosinef hls/sqrtf hls/sqrt"}

CC="${CC:-gcc}"
CPPFLAGS="-I${script_dir}/../../etc/libbambu/"
CFLAGS="-fopenmp -O3"
LDFLAGS="-lm -lmpfr -lgmp"

function cleanup {
   rm -rf "${out_dir}"
}
trap cleanup EXIT

rm -rf "${out_dir}"
mkdir -p "${out_dir}"
IFS=' '; for mfunc in ${TEST_FUNCTIONS}; do
    echo "Testing ${mfunc}"
    test_dir="${out_dir}/${mfunc}"
    testbench_src="${mfunc/\//\/test_}.c"
    mkdir -p "${test_dir}"
    ${CC} ${CPPFLAGS} ${CFLAGS} -o "${test_dir}/test" \
        "${script_dir}/../../etc/libbambu/libm/${testbench_src}" ${LDFLAGS}
    return_value=$?
    if test $return_value -ne 0; then
        echo "ERROR: Unable to compile C-based test for ${mfunc} function!"
        touch "${test_dir}/failure_${mfunc/\//_}"
        continue
    fi
    ${test_dir}/test
    return_value=$?
    if test $return_value -ne 0; then
        echo "ERROR: C-based test of ${mfunc} function not passed!"
        touch "${test_dir}/failure_${mfunc/\//_}"
        continue
    fi
    echo "  PASSED";
done

if test "x`find ${test_dir} -name 'failure_*'`" != "x"; then
    echo "ERROR: Test suite failed!"
    exit 1
fi
