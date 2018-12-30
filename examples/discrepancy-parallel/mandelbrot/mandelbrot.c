// Mandelbrot kernel
// Author: Andrew Canis
// Date: July 1, 2012

#include <stdio.h>

#define DECIMAL_PLACES 28
#define int2fixed(num) ((num) << DECIMAL_PLACES)
#define fixedmul(a, b) ((((long long)a) * ((long long)b)) >> DECIMAL_PLACES)
#define fixed2int(num) ((num) >> DECIMAL_PLACES)

#define WIDTH 128
#define HEIGHT 128
//#define MAX_ITER 10
//#define MAX_ITER 2
#define MAX_ITER 50

#define LEGUP 1

volatile unsigned char img[WIDTH][HEIGHT];

int mandelbrot() {
    int count = 0;
#pragma omp parallel for reduction(+:count)
	for (int j = 0; j < HEIGHT; j++) {
		for (int i = 0; i < WIDTH; i++) {

			int x_0 = int2fixed(-2) + ((((3 << 20) * i/WIDTH) ) << 8);
			int y_0 = int2fixed(-1) + ((((2 << 20) * j/HEIGHT) ) << 8);

			int x = 0;
			int y = 0;
			int xtmp;
			unsigned char fiter = 0;

            for (int iter = 0; iter < MAX_ITER; iter++) {
                long long abs_squared = fixedmul(x,x) + fixedmul(y,y);

				xtmp = fixedmul(x,x) - fixedmul(y,y) + x_0;
				y = fixedmul(int2fixed(2), fixedmul(x,y)) + y_0;
				x = xtmp;
				fiter  +=  abs_squared <= int2fixed(4);
			}

			//get black or white
            unsigned char colour = (fiter >= MAX_ITER) ? 0 : 1;
			//accumulate colour
            count += colour;
			//update image
			img[i][j] = colour;
		}
	}
	return count;
}


int main() {
	int count = mandelbrot();
	int res = 0;

    if (count == 12013) {
        printf("PASS\n");
    } else {
        printf("FAIL\n");
	res = 1;
    }

	return res;
}
