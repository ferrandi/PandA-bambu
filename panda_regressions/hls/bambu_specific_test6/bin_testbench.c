typedef struct c_t {
    unsigned nrow;
    unsigned nnz;
    float value[20];
    unsigned cindex[20];
    unsigned rowstart[20];
} complex_t;

unsigned kernel(complex_t*a_ptr)
{
  return a_ptr->value[1];
}
