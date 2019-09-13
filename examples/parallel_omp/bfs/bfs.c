#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <omp.h>

#include "graph.h"


unsigned map[NUM_VERTICES]={0};


__attribute__((noinline))
unsigned cas(unsigned * addr, unsigned old, unsigned new)
{
   unsigned ret = 1;
#pragma omp atomic
   {
      if(*addr==old)
      {
         *addr=new;
         ret = 0;
      }
   }
   return ret;
}


__attribute__((noinline))
unsigned ifa(unsigned * addr, unsigned val)
{
   unsigned temp;
#pragma omp atomic
   {
  temp = *addr;
  *addr=temp+val;
   }
  return temp;
}


void kernel(unsigned vertex, unsigned *p_Qnext, unsigned *Qnext_N, unsigned * map) 
{
  unsigned i;
  for ( i = offset[vertex]; i < offset[vertex+1]; i++ ) {
    if ( cas ( &map[edges[i]],0,1 ) == 0 ) {
      p_Qnext[ifa( Qnext_N, 1 )] = edges[i];
    }
  }
}


__attribute__((noinline))
void kernel_wrapper(unsigned int i, unsigned * p_Q, unsigned * p_Qnext, unsigned * Qnext_N)
{
  unsigned vertex = p_Q[i];
  kernel(vertex, p_Qnext, Qnext_N, map);
}


__attribute__((noinline))
void parallel(unsigned start, unsigned end,unsigned * p_Q, unsigned * p_Qnext, unsigned * Qnext_N)
{
  unsigned i;
#pragma omp parallel for
  for (i = start; i < end; ++i)
    kernel_wrapper(i, p_Q, p_Qnext, Qnext_N);
}


int main () {
  unsigned root = 0;

  map[root] = 1;

  unsigned Q[NUM_VERTICES]={0};
  unsigned Qnext[NUM_VERTICES]={0};
  unsigned Qnext_N = 0;

  unsigned Q_N = 0;

  Q_N = 1;
  Q[0] = root;
  Qnext_N = 0;

  unsigned * p_Q     = ( unsigned * ) Q;
  unsigned * p_Qnext = ( unsigned * ) Qnext;

  unsigned level = 0;

  unsigned totExplored = 0;

  while ( Q_N != 0 ) {
    totExplored += Q_N;

    parallel(0, Q_N, p_Q, p_Qnext, &Qnext_N);

    unsigned * p_tmp;

    p_tmp = p_Q;
    p_Q = p_Qnext;
    p_Qnext = p_tmp;

    Q_N = Qnext_N;
    Qnext_N = 0;

    level++;
  }

  return level;
}

