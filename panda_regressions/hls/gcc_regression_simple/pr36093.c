extern void abort (void);

typedef struct Bar {
      char c[129];
} Bar
 #ifndef __clang__
 __attribute__((__aligned__(128)))
 #endif
 ;

typedef struct Foo {
      Bar bar[4];
} Foo;

Foo foo[4];

int main()
{
   int i, j;
   Foo *foop = &foo[0];

   for (i=0; i < 4; i++) {
      Bar *bar = &foop->bar[i];
      for (j=0; j < 129; j++) {
         bar->c[j] = 'a' + i;
      }
   }

   if (foo[0].bar[3].c[128] != 'd')
     abort ();
   return 0;
}
