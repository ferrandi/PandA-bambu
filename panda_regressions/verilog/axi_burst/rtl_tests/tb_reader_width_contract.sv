module tb_reader_width_contract #(
    parameter integer DATA_BITS = 32,
    parameter integer EXPECT_ARSIZE = (DATA_BITS <= 8) ? 0 : $clog2(DATA_BITS / 8)
);
    logic clock = 1'b0;
    always #5 clock = ~clock;

    logic reset = 1'b0;
    logic start = 1'b0;
    logic [1:0] in1 = 2'b0;
    logic [31:0] in2 = DATA_BITS;
    logic [31:0] in3 = 32'b0;
    logic [31:0] in4 = 32'b0;
    logic [31:0] in5 = 32'b0;
    wire done;
    wire [DATA_BITS-1:0] out1;
    wire fault;

    wire [31:0] araddr;
    wire [7:0] arlen;
    wire [2:0] arsize;
    wire [1:0] arburst;
    wire arid;
    wire arvalid;
    logic arready = 1'b1;
    wire [DATA_BITS-1:0] rdata = {{(DATA_BITS-1){1'b0}}, 1'b1};
    wire [1:0] rresp = 2'b0;
    wire rid = 1'b0;
    wire rlast = 1'b1;
    logic rvalid = 1'b0;
    wire rready;
    integer ar_count = 0;

    MinimalAXI4MasterPipelined #(
        .BITSIZE_out1(DATA_BITS),
        .BITSIZE_m_axi_rdata(DATA_BITS)
    ) dut (
        .clock(clock), .reset(reset), .start(start), .peer_fault(1'b0),
        .in1(in1), .in2(in2), .in3(in3), .in4(in4), .in5(in5),
        .done(done), .out1(out1), .fault(fault),
        .m_axi_araddr(araddr), .m_axi_arlen(arlen), .m_axi_arsize(arsize),
        .m_axi_arburst(arburst), .m_axi_arid(arid), .m_axi_arvalid(arvalid),
        .m_axi_arready(arready), .m_axi_rdata(rdata), .m_axi_rresp(rresp),
        .m_axi_rid(rid), .m_axi_rlast(rlast), .m_axi_rvalid(rvalid), .m_axi_rready(rready)
    );

    always @(posedge clock) begin
        if (!reset) begin
            rvalid <= 1'b0;
            ar_count <= 0;
        end else if (arvalid && arready) begin
            if (arsize !== EXPECT_ARSIZE || arlen !== 0 || araddr !== 0 || arburst !== 2'b01 || arid !== 0)
                $fatal(1, "AXI_READER_WIDTH_BAD_AR DATA_BITS=%0d ARSIZE=%0d ARLEN=%0d ARADDR=%0d",
                       DATA_BITS, arsize, arlen, araddr);
            ar_count <= ar_count + 1;
            rvalid <= 1'b1;
        end else if (rvalid && rready) begin
            rvalid <= 1'b0;
        end
    end

    task automatic issue(input logic [1:0] opcode, input logic [31:0] count,
                         output logic [DATA_BITS-1:0] result);
        integer cycle;
        bit completed;
        begin
            @(negedge clock);
            in1 = opcode;
            in2 = DATA_BITS;
            in4 = 0;
            in5 = count;
            start = 1'b1;
            completed = 1'b0;
            result = '0;
            #1;
            if (done) begin
                completed = 1'b1;
                result = out1;
                start = 1'b0;
            end
            for (cycle = 0; !completed && cycle < 100; cycle = cycle + 1) begin
                @(negedge clock);
                #1;
                if (done) begin
                    completed = 1'b1;
                    result = out1;
                    start = 1'b0;
                    cycle = 100;
                end
            end
            if (!completed)
                $fatal(1, "AXI_READER_WIDTH_TIMEOUT DATA_BITS=%0d opcode=%0d", DATA_BITS, opcode);
            start = 1'b0;
            repeat (2) @(negedge clock);
        end
    endtask

    initial begin
        logic [DATA_BITS-1:0] result;
        repeat (2) @(negedge clock);
        reset = 1'b1;
        repeat (2) @(negedge clock);
        issue(2'd2, 1, result);
        issue(2'd0, 1, result);
        if (fault || ar_count != 1 || result !== {{(DATA_BITS-1){1'b0}}, 1'b1})
            $fatal(1, "AXI_READER_WIDTH_BAD_RESULT DATA_BITS=%0d fault=%0d AR_COUNT=%0d OUT=%0d",
                   DATA_BITS, fault, ar_count, result);
        $display("AXI_READER_WIDTH_PASS DATA_BITS=%0d ARSIZE=%0d", DATA_BITS, EXPECT_ARSIZE);
        $finish;
    end
endmodule
