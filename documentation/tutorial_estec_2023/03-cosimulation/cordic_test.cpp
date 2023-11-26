/************************************************
Copyright (c) 2016, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/
#include "read_file.h"
#include <stdio.h>
#include <stdlib.h>

#include "cordic.h"

#define MAX_TEST_COUNT 110465
#ifndef TEST_COUNT
#define TEST_COUNT MAX_TEST_COUNT
#else
#if TEST_COUNT > MAX_TEST_COUNT
#error TEST_COUNT is above maximum
#endif
#endif

coord_t input_data[TEST_COUNT][2];
phase_t ref_results[TEST_COUNT][1];

int main(int argc, char** argv)
{
   printf("argc: %d\n", argc);
   if(argc < 3)
   {
      printf("usage: %s <input_data> <ref_results>\n", argv[0]);
      exit(1);
   }
   coord_t inp_x, inp_y, out_x, out_y;
   phase_t inp_rad, out_rad;
   float res, ref_res;

   float diff, total_error = 0;

   int i;
   FILE *fp, *fm;

   // const float angle = M_PI_2/N;

   fp = fopen("./vector.dat", "w");
   if(fp == NULL)
   {
      fprintf(stderr, "unable to open output file\n");
   }

   // read files
   read_file<TEST_COUNT, 2, coord_t>(argv[1], input_data);
   read_file<TEST_COUNT, 1, phase_t>(argv[2], ref_results);

   // vectoring mode
   inp_rad = 0;
   for(i = 0; i < TEST_COUNT; i++)
   {
      inp_y = (coord_t)input_data[i][0];
      inp_x = (coord_t)input_data[i][1];
      ref_res = (phase_t)ref_results[i][0];

      // void cordic (   bool mode,   coord_t x0,   coord_t y0,   phase_t z0,   coord_t *xn,   coord_t *yn,   phase_t
      // *zn); if ( (inp_y>0) & (inp_x>0) )
      {
         // REF DESIGN
         float c_atan2 = atan2f((float)inp_y, (float)inp_x);
         // DUT
         top_atan2(inp_y, inp_x, &out_rad);

         // check results
         diff = out_rad - ref_results[i][0];
         if(diff < 0)
            diff = 0 - diff; // take absolute value
         total_error += diff;
         fprintf(fp, "%20.10f %20.10f %20.10f %20.10f %20.10f\n", (float)inp_y, (float)inp_x, (float)out_rad,
                 (float)ref_results[i][0], (float)c_atan2);
         // fprintf(stderr, "%20.10f %20.10f %20.10f %20.10f\n", (float) inp_y, (float) inp_x, (float) out_rad, (float)
         // ref_results[i][0]);
      }
   }

   fprintf(stderr, "total    error = %f\n", total_error);
   fprintf(stderr, "relative error = %f\n", total_error / TEST_COUNT);

   fclose(fp);

   return 0;
}
