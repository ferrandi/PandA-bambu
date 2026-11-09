// Deterministic local replacement for the DPI-backed simulation utility.
// The memory-cell implementation remains the implementation extracted from
// NC_TESTBENCH_IPs.xml; reads return the word address as their golden value.
typedef int unsigned ptr_t;

module if_utils #(parameter integer ID = 0, BITSIZE_data = 32) ();
    function automatic [BITSIZE_data-1:0] read_a(input ptr_t addr);
        read_a = addr >> 2;
    endfunction

    function automatic integer write_strobe(
        input logic [BITSIZE_data-1:0] data,
        input logic [((BITSIZE_data + 7) / 8)-1:0] strobe,
        input ptr_t addr
    );
        write_strobe = 0;
    endfunction
endmodule
