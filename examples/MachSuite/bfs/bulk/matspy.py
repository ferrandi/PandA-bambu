#!/usr/bin/env python

import matplotlib.pyplot as plt
import numpy as np
from mat import mat, mat2

#(fig,ax)=plt.subplots()
plt.imshow(mat);
plt.savefig('mat.png')
plt.imshow(mat2);
plt.savefig('mat2.png')

print sum(map(sum,mat))
print sum(map(sum,mat2))

#d=dict(zip(list('ABCD'),[0,0,0,0]))
#for c in mat:
#  d[c] += 1
#s=sum(d.values())
#for k in d:
#  print 100*d[k]/s
