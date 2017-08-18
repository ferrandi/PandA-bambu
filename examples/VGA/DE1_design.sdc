create_generated_clock -divide_by 2 -source [get_ports clock] -name {clock_25} [get_registers *|frequency_divider_byX:frequency_divider_byX_i|clk_div_by_X]

