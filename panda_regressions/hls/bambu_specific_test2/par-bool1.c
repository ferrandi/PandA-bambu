int exam(int a, int b, int c, int d, int e, int f, _Bool cond)
{
    int tmp_1, tmp_2, tmp_3, tmp_4, tmp_5, tmp_6, tmp_7;
    int out1;
    tmp_1 = a - b;
    tmp_2 = a * b;
    tmp_3 = c - d;
    tmp_4 = tmp_1 * tmp_3;
    tmp_5 = tmp_1 - tmp_2;
    tmp_6 = tmp_5 + tmp_3 + tmp_4;
    if(cond)
        tmp_7= tmp_6*c;
    else
        tmp_7= tmp_6*d;
    out1 = tmp_7 * a;
    return out1;
}

