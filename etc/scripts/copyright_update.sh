#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
root_dir="$(readlink -e ${script_dir}/../..)"

year=$(date +'%Y')
copyright_regex="s/((?<!\d)(?!${year})\d{4}(?!\d))(-\d\d\d\d){0,1}( Politecnico di Milano)/\$1-${year}\$3/g"

find ${root_dir}/${path} -maxdepth 1 -type f -print0 | xargs -0 perl -i -pe "${copyright_regex}"
for path in '.github' 'documentation' 'etc' 'examples' 'panda_regressions' 'panda_unit' 'src' 'style'
do
   find ${root_dir}/${path} \( -type d -name .git -prune \) -o \( -type d -wholename ${root_dir}/ext -prune \) -o -type f -print0 \
       | xargs -0 perl -i -pe "${copyright_regex}"
done
