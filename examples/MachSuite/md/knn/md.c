/*
Copyright (c) 2011, UT-Battelle, LLC
Copyright (c) 2014, the President and Fellows of Harvard College
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Oak Ridge National Laboratory, nor UT-Battelle, LLC, nor
  the names of its contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Implemenataion based on:
A. Danalis, G. Marin, C. McCurdy, J. S. Meredith, P. C. Roth, K. Spafford, V. Tipparaju, and J. S. Vetter. 
The scalable heterogeneous computing (shoc) benchmark suite. 
In Proceedings of the 3rd Workshop on General-Purpose Computation on Graphics Processing Units, 2010.
*/

#include "md.h"

void md_kernel(TYPE d_force_x[nAtoms], 
               TYPE d_force_y[nAtoms], 
               TYPE d_force_z[nAtoms], 
               TYPE position_x[nAtoms], 
               TYPE position_y[nAtoms], 
               TYPE position_z[nAtoms], 
               TYPE NL[nAtoms*maxNeighbors]) 
{
    TYPE delx, dely, delz, r2inv;
    TYPE r6inv, potential, force, j_x, j_y, j_z;
    TYPE i_x, i_y, i_z, fx, fy, fz;

    int i, j, jidx;

loop_i : for (i = 0; i < nAtoms; i++){
             i_x = position_x[i]; 
             i_y = position_y[i]; 
             i_z = position_z[i]; 
             fx = 0;
             fy = 0;
             fz = 0;
loop_j : for( j = 0; j < maxNeighbors; j++){
             // Get neighbor
             jidx = NL[i*maxNeighbors + j];
             // Look up x,y,z positions
             j_x = position_x[jidx]; 
             j_y = position_y[jidx]; 
             j_z = position_z[jidx];
             // Calc distance
             delx = i_x - j_x; 
             dely = i_y - j_y; 
             delz = i_z - j_z;
             r2inv = 1.0/( delx*delx + dely*dely + delz*delz );
             // Assume no cutoff and aways account for all nodes in area
             r6inv = r2inv * r2inv * r2inv;
             potential = r6inv*(lj1*r6inv - lj2);
             // Sum changes in force
             force = r2inv*potential;
             fx += delx * force; 
             fy += dely * force; 
             fz += delz * force;
         }
         //Update forces after all neighbors accounted for.
         d_force_x[i] = fx; 
         d_force_y[i] = fy; 
         d_force_z[i] = fz;
         //printf("dF=%lf,%lf,%lf\n", fx, fy, fz);
         } 
}
