/* Trivially making sure IPA-SRA does not introduce segfaults where they should
   not be.  */

struct bovid
{
  float red;
  int green;
  void *blue;
};

static int
__attribute__((noinline))
ox (int fail, struct bovid *cow)
{
  int r;
  if (fail)
    r = cow->red;
  else
    r = 0;
  return r;
}

int main ()
{
  int r;

  r = ox (0, (void *) 0);
  return r;
}
