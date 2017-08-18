#!/usr/bin/env python

# Changes a file in MatrixMarket "symmetric" format into one in "general" format
# This makes it easier to parse without changing the input.

import sys

for line in sys.stdin.readlines()[2:]:
  (row,col,value) = line.strip().split()
  if row==col:
    print row,col,value
  else:
    print row,col,value
    print col,row,value
