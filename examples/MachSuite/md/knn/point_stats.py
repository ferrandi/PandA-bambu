#!/usr/bin/env python

import sys
import numpy as np

points = np.array( [(float(x),float(y),float(z)) for (x,y,z) in map(lambda L: L.strip().split(), sys.stdin.readlines())] )

N = points.shape[0]

dists = np.zeros((N,N))
for (i,A) in enumerate(points):
  for (j,B) in enumerate(points):
    if i!=j:
      dists[i,j] = np.linalg.norm(A-B)
    else:
      dists[i,j] = 13;

print np.mean(dists)
print np.var(dists)
minpair = np.unravel_index(np.argmin(dists), (N,N))
print minpair, np.min(dists)
print points[minpair[0]]
print points[minpair[1]]
maxpair = np.unravel_index(np.argmax(dists), (N,N))
print maxpair, np.max(dists), 20.0*np.sqrt(3.)
print points[maxpair[0]]
print points[maxpair[1]]
