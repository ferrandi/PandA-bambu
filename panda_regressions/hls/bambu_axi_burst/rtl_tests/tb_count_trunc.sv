// Witness for the element-count limit of the read burst engine.
//
// The region counters are COUNT_W = BITSIZE_in5 - 1 bits wide, one narrower than
// the count operand, because validation proves a region holds at most 2^30
// elements when the address bus is 32 bits. With a 64-bit address the address
// check alone allows counts far above that, so a count of 0x80000000 was accepted
// and then truncated to zero: the engine configured an empty region and reported
// success. The engine must reject a count that does not fit its own counters.
//
// Build the DUT with 64-bit address widths, matching the signals below:
//   -GBITSIZE_in4=64 -GBITSIZE_m_axi_araddr=64
module tb_count_trunc #(
    parameter integer B_MAX = 16,
    parameter integer MAX_OUTSTANDING = 1,
    parameter integer FIFO_DEPTH = 256,
    parameter integer ADDR_BITS = 64,
    parameter integer COUNT_BITS = 32,
    parameter integer DATA_BITS = 32
);
    logic clk = 1'b0;
    always #5 clk <= ~clk;

    logic reset_n = 1'b0;
    logic start = 1'b0;
    logic [1:0] opcode = 2'b0;
    logic [31:0] size_bits = 32'd32;
    logic [31:0] payload = 32'b0;
    logic [63:0] byte_address = 64'b0;
    logic [31:0] count = 32'b0;
    logic done;
    logic [31:0] data_out;
    logic fault;

    logic [63:0] araddr;
    logic [7:0] arlen;
    logic [2:0] arsize;
    logic [1:0] arburst;
    logic [0:0] arid;
    logic arvalid;
    logic arready = 1'b0;
    logic [31:0] rdata = 32'b0;
    logic [1:0] rresp = 2'b0;
    logic [0:0] rid = 1'b0;
    logic rlast = 1'b0;
    logic rvalid = 1'b0;
    logic rready;

    MinimalAXI4MasterPipelined #(
        .B_MAX(B_MAX), .MAX_OUTSTANDING(MAX_OUTSTANDING), .FIFO_DEPTH(FIFO_DEPTH),
        .BITSIZE_in4(ADDR_BITS), .BITSIZE_m_axi_araddr(ADDR_BITS),
        .BITSIZE_in5(COUNT_BITS), .BITSIZE_in2(DATA_BITS),
        .BITSIZE_m_axi_rdata(DATA_BITS), .BITSIZE_out1(DATA_BITS)
    ) dut (
        .clock(clk), .reset(reset_n), .start(start), .in1(opcode),
        .in2(size_bits), .in3(payload), .in4(byte_address),
        .in5(count), .done(done), .out1(data_out), .fault(fault),
        .m_axi_araddr(araddr), .m_axi_arlen(arlen), .m_axi_arsize(arsize),
        .m_axi_arburst(arburst), .m_axi_arid(arid), .m_axi_arvalid(arvalid),
        .m_axi_arready(arready), .m_axi_rdata(rdata), .m_axi_rresp(rresp),
        .m_axi_rid(rid), .m_axi_rlast(rlast), .m_axi_rvalid(rvalid),
        .m_axi_rready(rready)
    );

    task automatic apply_configure(input [63:0] base, input [31:0] n);
        begin
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = base;
            count = n;
            @(negedge clk);
            start = 1'b0;
            opcode = 2'd0;
            count = 32'b0;
            byte_address = 64'b0;
        end
    endtask

    initial begin
        repeat (4) @(posedge clk);
        reset_n = 1'b1;
        repeat (2) @(posedge clk);

        if ($test$plusargs("COUNT_TRUNC_64")) begin
            // 2^31 elements at a 32-bit element size is 8 GiB, well inside a
            // 64-bit address space, but one bit wider than the counters hold.
            apply_configure(64'h0000000000000000, 32'h80000000);
            #1;
            if (!fault)
                $fatal(1, "count 0x80000000 was accepted although it does not fit COUNT_W");
            if (dut.region_count !== 32'b0)
                $fatal(1, "region_count is %0d after rejecting the oversized count",
                       dut.region_count);
            $display("COUNT_TRUNC_64: oversized count rejected with fault");
            $finish;
        end

        else if ($test$plusargs("COUNT_MAX_64")) begin
            // Positive control: the largest count the counters can hold must
            // still be accepted, so the new check is about the counter width and
            // not about the address space.
            apply_configure(64'h0000000000000000, 32'h7fffffff);
            #1;
            if (fault)
                $fatal(1, "count 0x7fffffff was rejected, but it fits COUNT_W");
            if (dut.region_count !== 32'h7fffffff)
                $fatal(1, "region_count is %0d, expected 0x7fffffff", dut.region_count);
            $display("COUNT_MAX_64: largest representable count accepted");
            $finish;
        end else begin
            $fatal(1, "no scenario selected: pass +COUNT_TRUNC_64 or +COUNT_MAX_64");
        end
    end

    initial begin
        #200000;
        $fatal(1, "timeout waiting for the configure response");
    end
endmodule
