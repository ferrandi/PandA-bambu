#include <cstdio>
#include <ac_channel.h>

// Struct with 16 elements of unsigned long long (64-bit)
struct Data16 {
    unsigned long long val[16];
};

// Kernel: reads a struct via ac_channel, sums in pairs (first reduction step),
// stores results back into the same array, and returns via ac_channel.
extern "C" void __attribute__((noinline)) reduce16(ac_channel<Data16>& in,
                                                    ac_channel<Data16>& out) {
    Data16 inData, outData;

    inData = in.read();

    // Pairwise reduction: sum elements [i] + [i+1], store at [i] for i = 0, 2, ..., 14
    for (int i = 0; i < 16; i ++) {
        unsigned long long a0 = inData.val[i];
        unsigned long long a1 = inData.val[i + 1];
        outData.val[i] = i%2==0 ? a0 + a1 : 0;
    }
    outData.val[15] =  0;

    out.write(outData);
}
