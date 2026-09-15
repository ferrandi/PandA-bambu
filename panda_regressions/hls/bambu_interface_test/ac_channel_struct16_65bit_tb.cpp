#include <cstdio>
#include <ac_channel.h>
#include <ac_int.h>

struct Data16_65bit {
    ac_int<65, false> val[16];
};

extern "C" void reduce16_65bit(ac_channel<Data16_65bit>& in, ac_channel<Data16_65bit>& out);

int main()
{
    ac_channel<Data16_65bit> inCh, outCh;
    Data16_65bit inputData, resultData;

    // Initialise input array with values 1..16 (fits in ac_int<65>)
    for (int i = 0; i < 16; i++)
        inputData.val[i] = ac_int<65, false>((long long)(i + 1));

    inCh.write(inputData);

    // Call the kernel under test
    reduce16_65bit(inCh, outCh);

    resultData = outCh.read();

    // Verify pairwise reduction: val[i] = inputData[i] + inputData[i+1] for even i
    int pass = 1;
    for (int i = 0; i < 16; i++) {
        if (i % 2 == 0) {
            ac_int<65, false> expected = (ac_int<65, false>)(i + 1) + (ac_int<65, false>)(i + 2);
            ac_int<65, false> got = resultData.val[i];
            if (got != expected) {
                printf("FAIL at [%d]: got %s, expected %s\n", i, got.to_string(AC_BIN).c_str(), expected.to_string(AC_BIN).c_str());
                pass = 0;
            }
        } else {
            ac_int<65, false> got = resultData.val[i];
            if (got != 0) {
                printf("FAIL at [%d]: got %s, expected 0\n", i, got.to_string(AC_BIN).c_str());
                pass = 0;
            }
        }
    }

    if (pass)
        printf("PASS\n");

    return pass ? 0 : 1;
}
