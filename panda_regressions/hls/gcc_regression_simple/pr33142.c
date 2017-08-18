int abs(int j);
extern void abort(void);

__attribute__((noinline)) int lisp_atan2(long dy, long dx) {
    if (dx <= 0)
        if (dy > 0)
            return abs(dx) <= abs(dy);
    return 0;
}

int main() {   
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 5)
#else
    volatile long dy = 63, dx = -77;
    if (lisp_atan2(dy, dx))
        abort();
#endif
    return 0;
}
