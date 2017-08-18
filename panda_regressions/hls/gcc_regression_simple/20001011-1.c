extern void abort(void);
extern int strcmp(const char *, const char *);

int foo(const char *a)
{
    return strcmp(a, "test");
}

int test(void)
{
    if(foo(__FUNCTION__))
        abort();
    return 0;
}

int main(void)
{
    test();
    return 0;
}
