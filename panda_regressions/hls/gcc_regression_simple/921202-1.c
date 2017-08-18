#ifndef STACK_SIZE
#define	VLEN	2055
#else
#define VLEN ((STACK_SIZE/16) - 1)
#endif
void foo (char p1[VLEN], int p2, int p3){}
int mpn_mul_1(long p1[VLEN], long p2[VLEN], int p3){return 0;}
void mpn_print (long p1[VLEN], int p2){}
void mpn_random2(long p1[VLEN], int p2){}
int mpn_cmp(long * p1, long * p2, int p3){return 0;}
void exxit(){exit(0);}
main ()
{
  long dx[VLEN+1];
  long dy[VLEN+1];
  long s1[VLEN];
  int cyx, cyy;
  int i;
  long size;

  for (;;)
    {
      size = VLEN;
      mpn_random2 (s1, size);

      for (i = 0; i < 1; i++)
	;

      dy[size] = 0x12345678;

      for (i = 0; i < 1; i++)
	cyy = mpn_mul_1 (dy, s1, size);

      if (cyx != cyy || mpn_cmp (dx, dy, size + 1) != 0 || dx[size] != 0x12345678)
	{
	  foo ("", 8, cyy); mpn_print (dy, size);
	}
      exxit();
    }
}


