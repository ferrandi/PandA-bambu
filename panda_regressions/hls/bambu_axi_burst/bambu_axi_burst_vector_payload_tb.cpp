#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

#ifndef VECTOR_BITS
#error "VECTOR_BITS must be selected for the testbench"
#endif
#ifndef VECTOR_IN_PLACE
#define VECTOR_IN_PLACE 0
#endif

typedef unsigned char vector_payload __attribute__((vector_size(VECTOR_BITS / 8)));

extern "C" void kernel(const vector_payload *, vector_payload *, int);

static const int max_elements = 128;
static const int elements = 67;
static const int payload_bytes = VECTOR_BITS / 8;

static uint8_t input_byte(int element, int lane) {
  static const uint8_t patterns[] = {
      0x00, 0x80, 0xff, 0x7f, 0x01, 0x55, 0xaa, 0xfe,
      0x3f, 0xc0, 0x81, 0x18, 0x24, 0x42, 0x96, 0x69};
  return patterns[(element * 7 + lane * 11) % (sizeof(patterns) / sizeof(patterns[0]))];
}

static uint8_t canary_byte(int element, int lane) {
  return static_cast<uint8_t>(0xd3u ^ (element * 13u) ^ (lane * 29u));
}

int main() {
  static vector_payload source[max_elements];
  static vector_payload destination[max_elements];
  uint8_t expected[max_elements * (VECTOR_BITS / 8)];
  uint8_t *source_bytes = reinterpret_cast<uint8_t *>(source);
  uint8_t *destination_bytes = reinterpret_cast<uint8_t *>(destination);

  for (int element = 0; element < max_elements; ++element) {
    for (int lane = 0; lane < payload_bytes; ++lane) {
      const int offset = element * payload_bytes + lane;
      source_bytes[offset] = input_byte(element, lane);
      destination_bytes[offset] = canary_byte(element, lane);
#if VECTOR_IN_PLACE
      expected[offset] = input_byte(element, lane);
#else
      expected[offset] = element < elements ? input_byte(element, lane)
                                             : canary_byte(element, lane);
#endif
    }
  }

#ifdef __BAMBU_SIM__
  m_param_alloc(0, sizeof(source));
  m_param_alloc(1, sizeof(destination));
#endif

#if VECTOR_IN_PLACE
  // Forward overlap makes scalar ordering observable: each later read can see
  // the earlier store, so burst prefetch must fall back for this shared range.
  for (int element = 0; element < elements; ++element)
    for (int lane = 0; lane < payload_bytes; ++lane)
      expected[(element + 1) * payload_bytes + lane] = expected[element * payload_bytes + lane];
  kernel(source, source + 1, elements);
  if (memcmp(source_bytes, expected, sizeof(expected)) != 0) {
    fprintf(stderr, "AXI_BURST_VECTOR_PAYLOAD_FAIL in-place payload changed\n");
    return 1;
  }
  printf("AXI_BURST_VECTOR_PAYLOAD_PASS WIDTH=%d MODE=in-place-overlap N=%d\n", VECTOR_BITS, elements);
#else
  kernel(source, destination, elements);
  if (memcmp(destination_bytes, expected, sizeof(expected)) != 0) {
    for (int offset = 0; offset < sizeof(expected); ++offset) {
      if (destination_bytes[offset] != expected[offset]) {
        fprintf(stderr, "AXI_BURST_VECTOR_PAYLOAD_FAIL width=%d byte=%d expected=%02x actual=%02x\n",
                VECTOR_BITS, offset, expected[offset], destination_bytes[offset]);
        return 1;
      }
    }
  }
  if (memcmp(source_bytes, expected, elements * payload_bytes) != 0) {
    fprintf(stderr, "AXI_BURST_VECTOR_PAYLOAD_FAIL source mutated\n");
    return 1;
  }
  printf("AXI_BURST_VECTOR_PAYLOAD_PASS WIDTH=%d MODE=copy N=%d\n", VECTOR_BITS, elements);
#endif
  return 0;
}
