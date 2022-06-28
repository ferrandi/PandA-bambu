extern __attribute__((fastcall,noinline)) int sub2values(int a, int b);

int check(int a, int b)
{
    int res1;
#if 1
    asm(
        "{"
         "  movl	%2, %%edx\n\t"
         "  movl	%1, %%ecx\n\t"
         "  call sub2values\n\t"
         "  movl	%%eax, %0\n\t"
         "|"
         "  mov	esi, %2\n\t"
         "  mov	edi, %1\n\t"
         "  call sub2values\n\t"
         "  mov	eax, %0\n\t"
         "|"
         "/* Here Verilog is considered as third assembler dialect..*/\n"
         "reg done_port;\n"
         "// synthesis translate_off\n"
         "  always @(negedge clock)\n"
         "    if(start_port == 1'b1)\n"
         "    begin\n"
         "      $display(\"hello world %%h %%d\", in1, in2);\n"
         "    end\n"
         "  // synthesis translate_on\n"
         "  always @(posedge clock) done_port <= start_port;\n"
         "  assign out1 = in1 - in2;"
         "}"
        : "=r" (res1)
        : "r" (a), "r" (b) /*two inputs*/
       );
#else
    res1 = sub2values(a,b);
#endif
    return a + b + res1;
}

int main()
{
  volatile int a= 7;
  volatile int b= 5;
  return check(a,b)!=14;
}
