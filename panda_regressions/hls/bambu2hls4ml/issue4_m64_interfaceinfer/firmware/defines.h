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
    typedef ap_fixed<16,6> value_type;
    static const unsigned size = 2*1;
    ap_fixed<16,6> data[2*1];
    ap_fixed<16,6> &operator[](size_t pos) { return data[pos]; }
    const ap_fixed<16,6> &operator[](size_t pos) const { return data[pos]; }
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
typedef ap_fixed<34,14> fc1_accum_t;
struct fc1_result_t {
    typedef ap_fixed<34,14> value_type;
    static const unsigned size = 4*1;
    ap_fixed<34,14> data[4*1];
    ap_fixed<34,14> &operator[](size_t pos) { return data[pos]; }
    const ap_fixed<34,14> &operator[](size_t pos) const { return data[pos]; }
    fc1_result_t &operator=(const fc1_result_t &other) {
        if (&other == this) return *this;
        #pragma clang loop unroll(full)
        for (unsigned i = 0; i < size; i++) data[i] = other.data[i];
        return *this;
    }
    bool operator==(const fc1_result_t &other) const {
        for (unsigned i = 0; i < size; i++)
            if (data[i] != other.data[i]) return false;
        return true;
    }
    bool operator!=(const fc1_result_t &other) const { return !(*this == other); }
};
typedef ap_fixed<16,6> fc1_weight_t;
typedef ap_fixed<16,6> fc1_bias_t;
typedef ap_uint<1> layer2_index;
struct layer4_t {
    typedef ap_fixed<16,6> value_type;
    static const unsigned size = 4*1;
    ap_fixed<16,6> data[4*1];
    ap_fixed<16,6> &operator[](size_t pos) { return data[pos]; }
    const ap_fixed<16,6> &operator[](size_t pos) const { return data[pos]; }
    layer4_t &operator=(const layer4_t &other) {
        if (&other == this) return *this;
        #pragma clang loop unroll(full)
        for (unsigned i = 0; i < size; i++) data[i] = other.data[i];
        return *this;
    }
    bool operator==(const layer4_t &other) const {
        for (unsigned i = 0; i < size; i++)
            if (data[i] != other.data[i]) return false;
        return true;
    }
    bool operator!=(const layer4_t &other) const { return !(*this == other); }
};
typedef ap_fixed<18,8> relu1_table_t;
typedef ap_fixed<35,15> out_accum_t;
struct result_t {
    typedef ap_fixed<35,15> value_type;
    static const unsigned size = 2*1;
    ap_fixed<35,15> data[2*1];
    ap_fixed<35,15> &operator[](size_t pos) { return data[pos]; }
    const ap_fixed<35,15> &operator[](size_t pos) const { return data[pos]; }
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
typedef ap_fixed<16,6> out_weight_t;
typedef ap_fixed<16,6> out_bias_t;
typedef ap_uint<1> layer5_index;

// hls-fpga-machine-learning insert emulator-defines


#endif
