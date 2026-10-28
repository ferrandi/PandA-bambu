#!/usr/bin/env bash
SWD="$(dirname $(readlink -e $0))"

. "${SWD}/setup.sh"

INCLUDE="${LATTICE_ROOT}/cae_library"
report_file="${SWD}/bambu_results.xml"

if ! echo "$(bambu_results /application/outputs/include)" | grep -q "${INCLUDE}"; then
  cat > "${report_file}" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<application>
  <outputs>
    <include>${INCLUDE}</include>
  </outputs>
</application>
EOF
fi
