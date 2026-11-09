module tb_writer_width_contract #(
    parameter integer DATA_BITS = 32
);
    MinimalAXI4MasterPipelinedWrite #(
        .BITSIZE_in3(DATA_BITS),
        .BITSIZE_m_axi_wdata(DATA_BITS),
        .BITSIZE_m_axi_wstrb(DATA_BITS / 8)
    ) dut ();

    initial begin
        #1;
        $display("AXI_WRITER_WIDTH_PASS DATA_BITS=%0d", DATA_BITS);
        $finish;
    end
endmodule
