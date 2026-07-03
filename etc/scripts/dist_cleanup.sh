#!/bin/sh
DIST_DIR="$1"

if test -e "${DIST_DIR}/compilers"; then
   IFS=$'\n'; for bin in `find ${DIST_DIR}/compilers -maxdepth 3 -type l -path '*clang*'`; do
      NO_DELETE="${NO_DELETE}${NO_DELETE:+ -o }-name $(basename ${bin})"
   done
   IFS=$'\n'; for bin in `find ${DIST_DIR}/compilers -maxdepth 3 -type l -path '*clang*'|sed -E 's/-[0-9]+//g'|sort|uniq`; do
      NO_DELETE="${NO_DELETE}${NO_DELETE:+ -o }-name $(basename ${bin})"
   done

   rm -f `IFS=' '; find ${DIST_DIR}/compilers '(' -type f -o -type l ')' -path '*clang+llvm*/bin/*' -not '(' ${NO_DELETE} ')'`
   rm -f `find ${DIST_DIR}/compilers -path '*clang*' -name '*.a'`
fi
