/**
 * puts primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * September, 11 2013.
 *
*/
int __builtin_puts(const char * __restrict s)
{
    while ((*s) != '\0') {
        __builtin_putchar(*s);
        ++s;
    }
    __builtin_putchar('\n');
    return 0;
}

