
#include <ac_channel.h>
#include <ac_int.h>

// Struct with 16 elements of ac_int<64, false>
struct Data16_64bit {
    ac_int<64, false> val[16];
};

// Kernel: reads a struct via ac_channel, sums in pairs (first reduction step),
// stores results back into the same array, and returns via ac_channel.
extern "C" void __attribute__((noinline)) reduce16_64bit(ac_channel<Data16_64bit>& in,
                                                      ac_channel<Data16_64bit>& out) {
    Data16_64bit inData, outData;

    inData = in.read();

    // Pairwise reduction: sum elements [i] + [i+1], store at [i] for i = 0, 2, ..., 14
    for (int i = 0; i < 15; i ++) {
        auto a0 = inData.val[i];
        auto a1 = inData.val[i + 1];
        outData.val[i] = i%2==0 ? a0 + a1 : 0;
    }
    outData.val[15] =  0;

    out.write(outData);
}
