
extern int plus(int x, int y);
extern int minus(int x, int y);
extern int times(int x, int y);
extern int times2(int x);
extern int divide(int x, int y);

// computes the expression (2 * (((a - b) + c) * d) - ((e / f) * g)) and
// compares it with the value in expected
int compute (int a, int b, int c, int d, int e, int f, int g, int expected)
{
	return expected != minus(
			times2 (
				times(
					plus(
						minus(a, b),
						c
					),
					d
				)
			),
			times(
				divide(e, f),
				g
			)
		);

}
