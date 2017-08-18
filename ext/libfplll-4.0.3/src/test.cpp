#include <fplll.h>

using namespace std;
using namespace fplll;

int main(void) {
  ZZ_mat<mpz_t> M(3, 3);
  M.gen_uniform(3);
  cout << M << endl;
  lllReduction(M, 0.99, 0.51, LM_WRAPPER);
  cout << M << endl;
  return 0;
}
