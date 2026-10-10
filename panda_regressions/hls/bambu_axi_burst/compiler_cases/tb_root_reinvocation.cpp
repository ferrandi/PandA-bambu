#include <cstdlib>
extern "C" int root_reinvocation(const int *src, int n);

// Exercise runtime counts around B=16 and across repeated top-level invocations
// without resetting the synthesized root. The last call also changes the base
// pointer, checking that a drained region does not leak into the next call.
int main() {
  int src[64];
  for (int i = 0; i < 64; ++i) src[i] = i + 1;
  if (root_reinvocation(src, 0) != 0) return 1;
  if (root_reinvocation(src, -1) != 0) return 2;
  if (root_reinvocation(src, 1) != 1) return 3;
  if (root_reinvocation(src, 15) != 120) return 4;
  if (root_reinvocation(src, 16) != 136) return 5;
  if (root_reinvocation(src, 17) != 153) return 6;
  if (root_reinvocation(src, 38) != 741) return 7;
  if (root_reinvocation(src + 2, 3) != 12) return 8;
  return 0;
}
