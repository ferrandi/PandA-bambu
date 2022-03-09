#include "module_lib.h"

void my_ip(uint8_t command, uint32_t param1, uint32_t param2) {
	static module1_output_t module1_output;
	static module2_output_t module2_output;

	switch(command) {
		case 0:
			module1(param1, param2 >> 16, &module1_output);
			break;
		case 1:
			module2(param1, &module2_output);
			break;
		case 2:
			printer1(module1_output.output1, module1_output.output2, module1_output.output3, module1_output.output4);
			break;
		case 3:
			printer2(module2_output.output1, module2_output.output2, module2_output.output3);
			break;
		default:
			break;
	}
}
