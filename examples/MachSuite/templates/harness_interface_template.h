////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct bench_args_t {
  int a[N];
  int b[N];
  int bucket[BUCKETSIZE];
  int sum[SCAN_RADIX];
};
int INPUT_SIZE = sizeof(struct bench_args_t);

void ss_sort(int a[N], int b[N], int bucket[BUCKETSIZE], int sum[SCAN_RADIX]);

void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  ss_sort( args->a, args->b, args->bucket, args->sum );
}

////////////////////////////////////////////////////////////////////////////////
