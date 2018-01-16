/**
 * puts primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * September, 11 2013.
 *
*/
int puts(const char * __restrict s)
{
    while ((*s) != '\0') {
        putchar(*s);
        ++s;
    }
    putchar('\n');
    return 0;
}

