/*
Copyright (c) 2014, the President and Fellows of Harvard College.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of Harvard University nor the names of its contributors may
  be used to endorse or promote products derived from this software without
  specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "md.h"

#define MIN(x,y) ( (x)<(y) ? (x) : (y) )
#define MAX(x,y) ( (x)>(y) ? (x) : (y) )

void md( int n_points[blockSide][blockSide][blockSide],
         dvector_t d_force[blockSide][blockSide][blockSide][densityFactor],
         dvector_t position[blockSide][blockSide][blockSide][densityFactor] )
{
  ivector_t b0, b1; // b0 is the current block, b1 is b0 or a neighboring block
  dvector_t p, q; // p is a point in b0, q is a point in either b0 or b1
  int p_idx, q_idx;
  double dx, dy, dz, r2inv, r6inv, potential, f;

  // Iterate over the grid, block by block
  loop_grid0_x: for( b0.x=0; b0.x<blockSide; b0.x++ ) {
  loop_grid0_y: for( b0.y=0; b0.y<blockSide; b0.y++ ) {
  loop_grid0_z: for( b0.z=0; b0.z<blockSide; b0.z++ ) {
  // Iterate over the 3x3x3 (modulo boundary conditions) cube of blocks around b0
  loop_grid1_x: for( b1.x=MAX(0,b0.x-1); b1.x<MIN(blockSide,b0.x+2); b1.x++ ) {
  loop_grid1_y: for( b1.y=MAX(0,b0.y-1); b1.y<MIN(blockSide,b0.y+2); b1.y++ ) {
  loop_grid1_z: for( b1.z=MAX(0,b0.z-1); b1.z<MIN(blockSide,b0.z+2); b1.z++ ) {
  // For all points in b0
  dvector_t *base_q = position[b1.x][b1.y][b1.z];
  int q_idx_range = n_points[b1.x][b1.y][b1.z];
  loop_p: for( p_idx=0; p_idx<n_points[b0.x][b0.y][b0.z]; p_idx++ ) {
    p = position[b0.x][b0.y][b0.z][p_idx];
    double sum_x = d_force[b0.x][b0.y][b0.z][p_idx].x;
    double sum_y = d_force[b0.x][b0.y][b0.z][p_idx].y;
    double sum_z = d_force[b0.x][b0.y][b0.z][p_idx].z;
    // For all points in b1
    loop_q: for( q_idx=0; q_idx< q_idx_range ; q_idx++ ) {
      q = *(base_q + q_idx);

      // Don't compute our own
      if( q.x!=p.x || q.y!=p.y || q.z!=p.z ) {
        // Compute the LJ-potential
        dx = p.x - q.x;
        dy = p.y - q.y;
        dz = p.z - q.z;
        r2inv = 1.0/( dx*dx + dy*dy + dz*dz );
        r6inv = r2inv*r2inv*r2inv;
        potential = r6inv*(lj1*r6inv - lj2);
        // Update forces
        f = r2inv*potential;
        sum_x += f*dx;
        sum_y += f*dy;
        sum_z += f*dz;
      }
    } // loop_q
    d_force[b0.x][b0.y][b0.z][p_idx].x = sum_x ;
    d_force[b0.x][b0.y][b0.z][p_idx].y = sum_y ;
    d_force[b0.x][b0.y][b0.z][p_idx].z = sum_z ;   
  } // loop_p
  }}} // loop_grid1_*
  }}} // loop_grid0_*
}

