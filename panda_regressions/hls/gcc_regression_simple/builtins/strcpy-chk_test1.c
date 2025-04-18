/* Copyright (C) 2004, 2005  Free Software Foundation.

   Ensure builtin __strcpy_chk performs correctly.  */

extern void abort (void);
#include <stddef.h>
extern size_t strlen(const char *);
extern void *memcpy (void *, const void *, size_t);
extern char *strcpy (char *, const char *);
extern int memcmp (const void *, const void *, size_t);

#include "chk.h"

LOCAL const char s1[] = "123";
char p[32] = "";
char *s2 = "defg";
char *s3 = "FGH";
char *s4;
size_t l1 = 1;

void
__attribute__((noinline))
test1 (void)
{
  chk_calls = 0;
#ifndef __OPTIMIZE_SIZE__
  strcpy_disallowed = 1;
#else
  strcpy_disallowed = 0;
#endif

  if (strcpy (p, "abcde") != p || memcmp (p, "abcde", 6))
    abort ();
  if (strcpy (p + 16, "vwxyz" + 1) != p + 16 || memcmp (p + 16, "wxyz", 5))
    abort ();
  if (strcpy (p + 1, "") != p + 1 || memcmp (p, "a\0cde", 6))
    abort ();  
  if (strcpy (p + 3, "fghij") != p + 3 || memcmp (p, "a\0cfghij", 9))
    abort ();

  /* Test at least one instance of the __builtin_ style.  We do this
     to ensure that it works and that the prototype is correct.  */
  if (__builtin_strcpy (p, "abcde") != p || memcmp (p, "abcde", 6))
    abort ();

  strcpy_disallowed = 0;
  if (chk_calls)
    abort ();
}

#ifndef MAX_OFFSET
#define MAX_OFFSET (sizeof (long long))
#endif

#ifndef MAX_COPY
#define MAX_COPY (10 * sizeof (long long))
#endif

#ifndef MAX_EXTRA
#define MAX_EXTRA (sizeof (long long))
#endif

#define MAX_LENGTH (MAX_OFFSET + MAX_COPY + 1 + MAX_EXTRA)

/* Use a sequence length that is not divisible by two, to make it more
   likely to detect when words are mixed up.  */
#define SEQUENCE_LENGTH 31

static union {
  char buf[MAX_LENGTH];
  long long align_int;
  long double align_fp;
} u1, u2;

void
__attribute__((noinline))
test2 (void)
{
  int off1, off2, len, i;
  char *p, *q, c;

  for (off1 = 0; off1 < MAX_OFFSET; off1++)
    for (off2 = 0; off2 < MAX_OFFSET; off2++)
      for (len = 1; len < MAX_COPY; len++)
	{
	  for (i = 0, c = 'A'; i < MAX_LENGTH; i++, c++)
	    {
	      u1.buf[i] = 'a';
	      if (c >= 'A' + SEQUENCE_LENGTH)
		c = 'A';
	      u2.buf[i] = c;
	    }
	  u2.buf[off2 + len] = '\0';

	  p = strcpy (u1.buf + off1, u2.buf + off2);
	  if (p != u1.buf + off1)
	    abort ();

	  q = u1.buf;
	  for (i = 0; i < off1; i++, q++)
	    if (*q != 'a')
	      abort ();

	  for (i = 0, c = 'A' + off2; i < len; i++, q++, c++)
	    {
	      if (c >= 'A' + SEQUENCE_LENGTH)
		c = 'A';
	      if (*q != c)
		abort ();
	    }

	  if (*q++ != '\0')
	    abort ();
	  for (i = 0; i < MAX_EXTRA; i++, q++)
	    if (*q != 'a')
	      abort ();
	}
}

/* Test whether compile time checking is done where it should
   and so is runtime object size checking.  */
void
__attribute__((noinline))
test3 (void)
{
  struct A { char buf1[10]; char buf2[10]; } a;
  char *r = l1 == 1 ? &a.buf1[5] : &a.buf2[4];
  char buf3[20];
  int i;
  const char *l;

  /* The following calls should do runtime checking
     - source length is not known, but destination is.  */
  chk_calls = 0;
  strcpy (a.buf1 + 2, s3 + 3);
  strcpy (r, s3 + 2);
  r = l1 == 1 ? malloc (4) : &a.buf2[7];
  strcpy (r, s2 + 2);
  strcpy (r + 2, s3 + 3);
  r = buf3;
  for (i = 0; i < 4; ++i)
    {
      if (i == l1 - 1)
	r = &a.buf1[1];
      else if (i == l1)
	r = &a.buf2[7];
      else if (i == l1 + 1)
	r = &buf3[5];
      else if (i == l1 + 2)
	r = &a.buf1[9];
    }
  strcpy (r, s2 + 4);
  if (chk_calls != 0)
    abort ();

  /* Following have known destination and known source length,
     so if optimizing certainly shouldn't result in the checking
     variants.  */
  chk_calls = 0;
  strcpy (a.buf1 + 2, "");
  strcpy (r, "a");
  r = l1 == 1 ? malloc (4) : &a.buf2[7];
  strcpy (r, s1 + 1);
  r = buf3;
  l = "abc";
  for (i = 0; i < 4; ++i)
    {
      if (i == l1 - 1)
	r = &a.buf1[1], l = "e";
      else if (i == l1)
	r = &a.buf2[7], l = "gh";
      else if (i == l1 + 1)
	r = &buf3[5], l = "jkl";
      else if (i == l1 + 2)
	r = &a.buf1[9], l = "";
    }
  strcpy (r, "");
  /* Here, strlen (l) + 1 is known to be at most 4 and
     __builtin_object_size (&buf3[16], 0) is 4, so this doesn't need
     runtime checking.  */
  strcpy (&buf3[16], l);
  /* Unknown destination and source, no checking.  */
  strcpy (s4, s3);
  if (chk_calls)
    abort ();
  chk_calls = 0;
}
#if 0
/* Test whether runtime and/or compile time checking catches
   buffer overflows.  */
void
__attribute__((noinline))
test4 (void)
{
  struct A { char buf1[10]; char buf2[10]; } a;
  char buf3[20];

  chk_fail_allowed = 1;
  /* Runtime checks.  */
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      strcpy (&a.buf2[9], s2 + 3);
      abort ();
    }
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      strcpy (&a.buf2[7], s3 + strlen (s3) - 3);
      abort ();
    }
  /* This should be detectable at compile time already.  */
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      strcpy (&buf3[19], "a");
      abort ();
    }
  chk_fail_allowed = 0;
}
#endif
void *chk_fail_buf[];
volatile int chk_fail_allowed, chk_calls;
volatile int memcpy_disallowed, mempcpy_disallowed, memmove_disallowed;
volatile int memset_disallowed, strcpy_disallowed, stpcpy_disallowed;
volatile int strncpy_disallowed, strcat_disallowed, strncat_disallowed;
volatile int sprintf_disallowed, vsprintf_disallowed;
volatile int snprintf_disallowed, vsnprintf_disallowed;

void
main_test (void)
{
#ifndef __OPTIMIZE__
  /* Object size checking is only intended for -O[s123].  */
  return;
#endif
#ifndef __clang__
  __asm ("{||assign out1 = in1;\nassign done_port = start_port;|out1 <= std_logic_vector(resize(unsigned(in1), BITSIZE_out1));\ndone_port <= start_port;}" : "=r" (s2) : "0" (s2));
  __asm ("{||assign out1 = in1;\nassign done_port = start_port;|out1 <= std_logic_vector(resize(unsigned(in1), BITSIZE_out1));\ndone_port <= start_port;}" : "=r" (s3) : "0" (s3));
  __asm ("{||assign out1 = in1;\nassign done_port = start_port;|out1 <= std_logic_vector(resize(in1, BITSIZE_out1));\ndone_port <= start_port;}" : "=r" (l1) : "0" (l1));
#endif
  test1 ();
  //test2 ();
  s4 = p;
  //test3 ();
  //test4 ();
}
int main()
{
#ifndef __clang__
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
  main_test();
#endif
#endif
  return 0;
}
