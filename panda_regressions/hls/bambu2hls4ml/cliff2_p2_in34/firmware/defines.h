#ifndef DEFINES_H_
#define DEFINES_H_

#include "ap_fixed.h"
#include "ap_int.h"
#include "nnet_utils/nnet_types.h"
#include <array>
#include <cstddef>
#include <cstdio>
#include <tuple>
#include <tuple>


// hls-fpga-machine-learning insert numbers

// hls-fpga-machine-learning insert layer-precision
struct input_t {
    typedef ap_fixed<32,9> value_type;
    static const unsigned size = 5*1;
    ap_fixed<32,9> data[5*1];
    ap_fixed<32,9> &operator[](size_t pos) { return data[pos]; }
    const ap_fixed<32,9> &operator[](size_t pos) const { return data[pos]; }
    input_t &operator=(const input_t &other) {
        if (&other == this) return *this;
        #pragma clang loop unroll(full)
        for (unsigned i = 0; i < size; i++) data[i] = other.data[i];
        return *this;
    }
    bool operator==(const input_t &other) const {
        for (unsigned i = 0; i < size; i++)
            if (data[i] != other.data[i]) return false;
        return true;
    }
    bool operator!=(const input_t &other) const { return !(*this == other); }
};
typedef ap_fixed<32,9> model_default_t;
struct result_t {
    typedef ap_fixed<32,9> value_type;
    static const unsigned size = 5*1;
    ap_fixed<32,9> data[5*1];
    ap_fixed<32,9> &operator[](size_t pos) { return data[pos]; }
    const ap_fixed<32,9> &operator[](size_t pos) const { return data[pos]; }
    result_t &operator=(const result_t &other) {
        if (&other == this) return *this;
        #pragma clang loop unroll(full)
        for (unsigned i = 0; i < size; i++) data[i] = other.data[i];
        return *this;
    }
    bool operator==(const result_t &other) const {
        for (unsigned i = 0; i < size; i++)
            if (data[i] != other.data[i]) return false;
        return true;
    }
    bool operator!=(const result_t &other) const { return !(*this == other); }
};

// hls-fpga-machine-learning insert emulator-defines


#endif
