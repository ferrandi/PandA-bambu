#!/usr/bin/env bash

print_errror() {
  >&2 echo "$@"
}

usage() {
  print_errror "Usage: $0 [-o output_file] file1 file2 ..."
  exit 1
}

find_line() {
  echo `cat "$1"| grep "$2" -n | cut -f1 -d:`
}

find_tech_start() {
  find_line "$1" "<technology>"
}

find_tech_end() {
  find_line "$1" "</technology>"
}

output_file="/dev/stdout"

while getopts ":o:" opt; do
  case ${opt} in
    o )
      output_file="$OPTARG"
      ;;
    \? )
      print_errror "Invalid option: -$OPTARG" >&2
      usage
      ;;
    : )
      print_errror "Option -$OPTARG requires an argument." >&2
      usage
      ;;
  esac
done
shift $((OPTIND -1))

if [ $# -lt 1 ]; then
  print_errror "Error: No input files provided."
  usage
fi

base_file="$1"
shift

if test "${base_file}" -ef "${output_file}"; then
  print_errror "Error: Input and output file must be different."
  exit -1
fi

# Get initial insertion point
base_tech_body=`find_tech_start ${base_file}`

# Copy base file header
head -n${base_tech_body} "${base_file}" > "${output_file}"

for lib_xml in "$@"; do
    lib_start=$(( `find_tech_start ${lib_xml}` + 1 ))
    lib_end=`find_tech_end ${lib_xml}`
    line_count=`wc -l ${lib_xml}|cut -f1 -d' '`
    pad_newline=`if [[ $(tail -c1 "${lib_xml}") == "" ]]; then echo "0"; else echo "1"; fi`
    lib_rem=$(( ${line_count} - ${lib_end} + ${pad_newline} + 1 ))

    # Insert <technology> tag contents from lib_xml file
    tail -n +${lib_start} "${lib_xml}"|head -n-${lib_rem} >> "${output_file}"
done

# Copy base file reminder
tail -n +$(( ${base_tech_body} + 1 )) "${base_file}" >> "${output_file}"
