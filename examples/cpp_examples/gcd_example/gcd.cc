#include <algorithm>
int gcd(int x, int y )
 {
       if( x < y )
          std::swap( x, y );

       while( y > 0 )
       {
          int f = x % y;
          x = y;
          y = f;
       }
       return x;
 }


