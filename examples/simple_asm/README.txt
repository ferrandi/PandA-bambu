This simple example show how to integrate small snippet of Verilog in the HLS flow.
The idea is that bambu uses as third assembler dialect VERILOG.
Currently only single output asm instructions are supported. In case outputs are included to pass the simulation the intel and the att asm should be included. For asm having only inputs, such asm string could be safely left empty.
A detailed reference on how asm statements are considered by GCC could be found at this link:https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html.

