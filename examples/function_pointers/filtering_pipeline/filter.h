int add_filter(unsigned int pos, int (*filter)(unsigned char *, unsigned char *, unsigned int, unsigned int));

void execute(unsigned char *in, unsigned char *out, unsigned int x_size, unsigned int y_size);
