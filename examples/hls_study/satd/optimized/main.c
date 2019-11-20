#include <stdlib.h>

#define abs(x) ((x)>0?(x):-(x))

#pragma map call_hw VIRTEX5 0
void
__attribute__ ((noinline))  
satd( unsigned char pix[32], int* i_satd_result)
{
  
  int16_t tmp[4][4];
  int16_t diff[4][4]; 
  int d;  

  diff[0][0] = pix[0] - pix[16];
  diff[0][1] = pix[1] - pix[17];
  diff[0][2]=  pix[2] - pix[18];
  diff[0][3] = pix[3] - pix[19];

  diff[1][0] = pix[4] - pix[20];
  diff[1][1] = pix[5] - pix[21];
  diff[1][2] = pix[6] - pix[22];
  diff[1][3] = pix[7] - pix[23];

  diff[2][0] = pix[8] - pix[24];
  diff[2][1] = pix[9] - pix[25];
  diff[2][2] = pix[10] - pix[26];
  diff[2][3] = pix[11] - pix[27];

  diff[3][0] = pix[12] - pix[28]; 
  diff[3][1] = pix[13] - pix[29]; 
  diff[3][2] = pix[14] - pix[30]; 
  diff[3][3] = pix[15] - pix[31];

  for( d = 0; d < 4; d++ )
    {
      int s01, s23;
      int d01, d23;

      s01 = diff[d][0] + diff[d][1]; s23 = diff[d][2] + diff[d][3];
      d01 = diff[d][0] - diff[d][1]; d23 = diff[d][2] - diff[d][3];

      tmp[d][0] = s01 + s23;
      tmp[d][1] = s01 - s23;
      tmp[d][2] = d01 - d23;
      tmp[d][3] = d01 + d23;
    }

  for( d = 0; d < 4; d++ )
    {
      int s01, s23;
      int d01, d23;

      s01 = tmp[0][d] + tmp[1][d]; s23 = tmp[2][d] + tmp[3][d];
      d01 = tmp[0][d] - tmp[1][d]; d23 = tmp[2][d] - tmp[3][d];

      *i_satd_result += abs( s01 + s23 ) + abs( s01 - s23 ) + abs( d01 - d23 ) + abs( d01 + d23 );
    }
}

int main() {
   char *a;
   int *b;
   int i;
   
   a=malloc(32*sizeof(char));
   b=malloc(sizeof(int));

   for(i=0;i<32;i++) {
     a[i] = i%3;
   }
   *b=0;
   
   satd(a,b);
   
   return 0;
}
