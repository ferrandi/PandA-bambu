#include <cstdio>
#include <ac_channel.h>

struct Data16 {
    unsigned long long val[16];
};

extern "C" void reduce16(ac_channel<Data16>& in, ac_channel<Data16>& out);

int main()
{
    ac_channel<Data16> inCh, outCh;
    Data16 inputData, resultData;

    // Initialise input array with values 1..16
    for (int i = 0; i < 16; i++)
        inputData.val[i] = (unsigned long long)(i + 1);

    inCh.write(inputData);

    // Call the kernel under test
    reduce16(inCh, outCh);

    resultData = outCh.read();

    // Verify pairwise reduction: val[i] = inputData[i] + inputData[i+1] for even i
    int pass = 1;
    for (int i = 0; i < 16; i++) {
        if (i % 2 == 0) {
            unsigned long long expected = (unsigned long long)(i + 1) + (unsigned long long)(i + 2);
            if (resultData.val[i] != expected) {
                printf("FAIL at [%d]: got %llu, expected %llu\n",
                       i, resultData.val[i], expected);
                pass = 0;
            }
        } else {
            if (resultData.val[i] != 0) {
                printf("FAIL at [%d]: got %llu, expected 0\n", i, resultData.val[i]);
                pass = 0;
            }
        }
    }

    if (pass)
        printf("PASS\n");

    return pass ? 0 : 1;
}
