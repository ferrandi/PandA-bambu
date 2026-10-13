module tb_t4b_slave #(
    parameter integer READ_DELAY = 63,
    parameter integer QUEUE_SIZE = 33,
    parameter integer N_REQUESTS = 16
);
    logic clock = 1'b0;
    always #5 clock = ~clock;

    logic reset = 1'b0;
    logic [0:0] arid = 1'b0;
    logic [31:0] araddr = 0;
    logic [7:0] arlen = 0;
    logic [2:0] arsize = 3'd2;
    logic [1:0] arburst = 2'b01;
    logic [0:0] arlock = 0;
    logic [3:0] arcache = 0;
    logic [2:0] arprot = 0;
    logic [3:0] arqos = 0;
    logic [3:0] arregion = 0;
    logic [0:0] aruser = 0;
    logic arvalid = 0;
    wire arready;
    wire [0:0] rid;
    wire [31:0] rdata;
    wire [1:0] rresp;
    wire [1:0] ruser;
    wire rlast;
    wire rvalid;
    logic rready = 0;

    wire awready, wready, bvalid;
    wire [0:0] bid;
    wire [1:0] bresp, buser;
    logic [0:0] awid = 0;
    logic [31:0] awaddr = 0;
    logic [7:0] awlen = 0;
    logic [2:0] awsize = 0;
    logic [1:0] awburst = 0;
    logic [0:0] awlock = 0;
    logic [3:0] awcache = 0;
    logic [2:0] awprot = 0;
    logic [3:0] awqos = 0;
    logic [3:0] awregion = 0;
    logic [0:0] awuser = 0;
    logic awvalid = 0;
    logic [31:0] wdata = 0;
    logic [3:0] wstrb = 0;
    logic wlast = 0, wvalid = 0;
    logic [0:0] wuser = 0;
    logic bready = 0;

    localparam integer BASE = 32'h0000_1000;
    integer planned_addr [0:N_REQUESTS-1];
    integer planned_beats [0:N_REQUESTS-1];
    integer cycle_count = 0;
    integer accepted_ar = 0;
    integer accepted_r = 0;
    integer descriptor_index = 0;
    integer beat_in_descriptor = 0;
    integer total_beats = 0;
    integer outstanding = 0;
    integer outstanding_peak = 0;
    integer first_ar_cycle = -1;
    integer first_rvalid_cycle = -1;
    integer previous_r_cycle = -1;

    TestbenchMEMAXI #(.READ_DELAY(READ_DELAY), .QUEUE_SIZE(QUEUE_SIZE)) memory (
        .clock(clock), .reset(reset),
        .arid(arid), .araddr(araddr), .arlen(arlen), .arsize(arsize),
        .arburst(arburst), .arlock(arlock), .arcache(arcache), .arprot(arprot),
        .arqos(arqos), .arregion(arregion), .aruser(aruser),
        .arvalid(arvalid), .arready(arready), .rid(rid), .rdata(rdata),
        .rresp(rresp), .ruser(ruser), .rlast(rlast), .rvalid(rvalid), .rready(rready),
        .awid(awid), .awaddr(awaddr), .awlen(awlen), .awsize(awsize),
        .awburst(awburst), .awlock(awlock), .awcache(awcache), .awprot(awprot),
        .awqos(awqos), .awregion(awregion), .awuser(awuser),
        .awvalid(awvalid), .awready(awready), .wdata(wdata), .wstrb(wstrb),
        .wlast(wlast), .wvalid(wvalid), .wready(wready), .wuser(wuser),
        .bid(bid), .bresp(bresp), .buser(buser), .bvalid(bvalid), .bready(bready)
    );

    always @(posedge clock) begin : scoreboard
        integer push;
        integer pop;
        integer expected_addr;
        if (!reset) begin
            cycle_count = 0;
            accepted_ar = 0;
            accepted_r = 0;
            descriptor_index = 0;
            beat_in_descriptor = 0;
            outstanding = 0;
            outstanding_peak = 0;
            first_ar_cycle = -1;
            first_rvalid_cycle = -1;
            previous_r_cycle = -1;
        end else begin
            cycle_count = cycle_count + 1;
            if (rvalid && first_rvalid_cycle < 0)
                first_rvalid_cycle = cycle_count;
            push = arvalid && arready;
            pop = rvalid && rready && rlast;
            if (push) begin
                if (accepted_ar >= N_REQUESTS)
                    $fatal(1, "unexpected extra AR request");
                if (araddr !== planned_addr[accepted_ar] ||
                    arlen + 1 !== planned_beats[accepted_ar])
                    $fatal(1, "AR[%0d] got addr=%h beats=%0d expected addr=%h beats=%0d",
                           accepted_ar, araddr, arlen + 1,
                           planned_addr[accepted_ar], planned_beats[accepted_ar]);
                if (arsize != 2 || arburst != 2'b01 || arid != 0)
                    $fatal(1, "unexpected AXI profile on AR[%0d]", accepted_ar);
                if ((araddr[11:0] + ((arlen + 1) << 2)) > 4096)
                    $fatal(1, "AR[%0d] crosses the 4-KiB boundary", accepted_ar);
                if (first_ar_cycle < 0)
                    first_ar_cycle = cycle_count;
                accepted_ar = accepted_ar + 1;
                total_beats = total_beats + arlen + 1;
            end
            if (rvalid && rready) begin
                if (rid != 0 || rresp != 0)
                    $fatal(1, "bad R ID/response");
                if (descriptor_index >= accepted_ar)
                    $fatal(1, "response arrived without an accepted descriptor");
                expected_addr = planned_addr[descriptor_index] + 4 * beat_in_descriptor;
                if (rdata !== (expected_addr >> 2))
                    $fatal(1, "R[%0d] got data=%h expected=%h address=%h",
                           accepted_r, rdata, expected_addr >> 2, expected_addr);
                if (rlast !== (beat_in_descriptor == planned_beats[descriptor_index] - 1))
                    $fatal(1, "RLAST mismatch at descriptor=%0d beat=%0d",
                           descriptor_index, beat_in_descriptor);
                if (previous_r_cycle >= 0 && cycle_count - previous_r_cycle != 1)
                    $fatal(1, "R beat spacing=%0d, expected 1", cycle_count - previous_r_cycle);
                previous_r_cycle = cycle_count;
                accepted_r = accepted_r + 1;
                if (rlast) begin
                    descriptor_index = descriptor_index + 1;
                    beat_in_descriptor = 0;
                end else begin
                    beat_in_descriptor = beat_in_descriptor + 1;
                end
            end
            outstanding = outstanding + push - pop;
            if (outstanding < 0)
                $fatal(1, "response completed without an outstanding request");
            if (outstanding > outstanding_peak)
                outstanding_peak = outstanding;
        end
    end

    initial begin : run_test
        integer i;
        integer addr_cursor;
        integer latency;
        if (N_REQUESTS < 16 || QUEUE_SIZE <= 16 || READ_DELAY < 1)
            $fatal(1, "invalid N1 parameters");
        addr_cursor = BASE;
        total_beats = 0;
        for (i = 0; i < N_REQUESTS; i = i + 1) begin
            planned_addr[i] = addr_cursor;
            planned_beats[i] = (i % 5) + 1;
            addr_cursor = addr_cursor + 4 * planned_beats[i];
        end

        repeat (4) @(negedge clock);
        reset = 1'b1;
        for (i = 0; i < N_REQUESTS; i = i + 1) begin
            @(negedge clock);
            araddr = planned_addr[i];
            arlen = planned_beats[i] - 1;
            arvalid = 1'b1;
            do @(posedge clock); while (!arready);
            @(negedge clock);
            arvalid = 1'b0;
        end
        if (accepted_ar != N_REQUESTS || accepted_r != 0)
            $fatal(1, "expected %0d accepted AR and no accepted R before release; got AR=%0d R=%0d",
                   N_REQUESTS, accepted_ar, accepted_r);
        if (outstanding_peak < 16)
            $fatal(1, "only %0d requests were simultaneously outstanding", outstanding_peak);
        wait (first_rvalid_cycle >= 0);
        latency = first_rvalid_cycle - first_ar_cycle;
        if (latency < 63 || latency > 65)
            $fatal(1, "AR-accept to first RVALID latency=%0d outside [63,65]", latency);
        // Do not change a handshake input in the posedge active region that
        // wakes the wait above: the scoreboard and memory model also sample
        // rready on that edge.  Release the response channel at a negedge.
        @(negedge clock);
        rready = 1'b1;
        wait (accepted_r == total_beats);
        @(negedge clock);
        rready = 1'b0;
        if (descriptor_index != N_REQUESTS || outstanding != 0)
            $fatal(1, "N1 drain failed descriptors=%0d outstanding=%0d", descriptor_index, outstanding);
        $display("T4B_N1 PASS READ_DELAY=%0d QUEUE_SIZE=%0d accepted_ar=%0d outstanding_peak=%0d L_axi=%0d beats=%0d",
                 READ_DELAY, QUEUE_SIZE, accepted_ar, outstanding_peak, latency, accepted_r);
        $finish;
    end

    initial begin
        #10000000;
        $fatal(1, "N1 watchdog expired");
    end
endmodule
