#ifndef MODULE_LIB_H
#define MODULE_LIB_H

#include <stdint.h>

typedef struct {
	uint64_t output1;
	uint64_t output2;
	uint16_t output3;
	uint32_t output4;
} module1_output_t;

extern void module1(uint32_t input1, uint16_t input2, module1_output_t *outputs);



typedef struct {
	uint64_t output1;
	uint64_t output2;
	uint16_t output3;
} module2_output_t;

extern void module2(uint32_t input1, module2_output_t *outputs);


extern void printer1(uint64_t value1, uint64_t value2, uint16_t value3, uint32_t value4);

extern void printer2(uint64_t value1, uint64_t value2, 	uint16_t value3);

extern void my_ip(uint8_t command, uint32_t param1, uint32_t param2);

#endif
