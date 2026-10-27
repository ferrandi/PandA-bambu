module tb_t4b_floor #(
    parameter integer B_MAX = 16,
    parameter integer MAX_OUTSTANDING = 1,
    parameter integer FIFO_DEPTH = 256,
    parameter integer N = 1024,
    parameter integer AXI_LATENCY = 64,
    parameter integer READ_DELAY = 63,
    parameter integer QUEUE_SIZE = 33
);
    logic clock = 1'b0;
    always #5 clock = ~clock;

    logic reset = 1'b0;
    logic start = 1'b0;
    logic [1:0] opcode = 0;
    logic [31:0] size_bits = 32;
    logic [31:0] payload = 0;
    logic [31:0] byte_address = 0;
    logic [31:0] count = 0;
    wire done;
    wire [31:0] data_out;
    wire fault;
    wire [31:0] araddr;
    wire [7:0] arlen;
    wire [2:0] arsize;
    wire [1:0] arburst;
    wire [0:0] arid;
    wire arvalid;
    wire arready;
    wire [31:0] rdata;
    wire [1:0] rresp;
    wire [0:0] rid;
    wire rlast;
    wire rvalid;
    wire rready;

    localparam integer BASE = 32'h0000_1000;
    wire awready, wready, bvalid;
    wire [0:0] bid;
    wire [1:0] bresp, buser;
    wire [1:0] ruser;

    MinimalAXI4MasterPipelined #(
        .B_MAX(B_MAX), .MAX_OUTSTANDING(MAX_OUTSTANDING), .FIFO_DEPTH(FIFO_DEPTH)
    ) burst_engine (
        .clock(clock), .reset(reset), .start(start), .in1(opcode),
        .in2(size_bits), .in3(payload), .in4(byte_address), .in5(count),
        .done(done), .out1(data_out), .fault(fault),
        .m_axi_araddr(araddr), .m_axi_arlen(arlen), .m_axi_arsize(arsize),
        .m_axi_arburst(arburst), .m_axi_arid(arid), .m_axi_arvalid(arvalid),
        .m_axi_arready(arready), .m_axi_rdata(rdata), .m_axi_rresp(rresp),
        .m_axi_rid(rid), .m_axi_rlast(rlast), .m_axi_rvalid(rvalid),
        .m_axi_rready(rready)
    );

    TestbenchMEMAXI #(.READ_DELAY(READ_DELAY), .QUEUE_SIZE(QUEUE_SIZE)) memory (
        .clock(clock), .reset(reset),
        .arid(arid), .araddr(araddr), .arlen(arlen), .arsize(arsize),
        .arburst(arburst), .arlock(1'b0), .arcache(4'b0), .arprot(3'b0),
        .arqos(4'b0), .arregion(4'b0), .aruser(1'b0),
        .arvalid(arvalid), .arready(arready), .rid(rid), .rdata(rdata),
        .rresp(rresp), .ruser(ruser), .rlast(rlast), .rvalid(rvalid), .rready(rready),
        .awid(1'b0), .awaddr(32'b0), .awlen(8'b0), .awsize(3'b0),
        .awburst(2'b0), .awlock(1'b0), .awcache(4'b0), .awprot(3'b0),
        .awqos(4'b0), .awregion(4'b0), .awuser(1'b0), .awvalid(1'b0),
        .awready(awready), .wdata(32'b0), .wstrb(4'b0), .wlast(1'b0),
        .wvalid(1'b0), .wready(wready), .wuser(1'b0), .bid(bid),
        .bresp(bresp), .buser(buser), .bvalid(bvalid), .bready(1'b0)
    );

    integer cycle_count = 0;
    integer configure_cycle = -1;
    integer final_consume_cycle = -1;
    integer drain_cycle = -1;
    integer first_ar_cycle = -1;
    integer first_rvalid_cycle = -1;
    integer accepted_ar = 0;
    integer accepted_r = 0;
    integer completed_descriptors = 0;
    integer accepted_beats [0:4095];
    integer bus_issued_beats = 0;
    integer bus_returned_beats = 0;
    integer bus_outstanding = 0;
    integer outstanding_peak = 0;
    integer n_desc_min = 0;
    integer descriptor_resp_beats = 0;
    integer reservation_count = 0;
    integer frozen_addr = 0;
    integer frozen_beats = 0;
    integer frozen_valid = 0;
    integer exp_remaining;
    integer exp_addr;
    integer exp_boundary;
    integer exp_credit;
    integer exp_beats;
    integer push;
    integer pop;
    integer i;

    always @(posedge clock)
    begin : independent_scoreboard
        if (!reset)
        begin
            cycle_count = 0;
            configure_cycle = -1;
            final_consume_cycle = -1;
            drain_cycle = -1;
            first_ar_cycle = -1;
            first_rvalid_cycle = -1;
            accepted_ar = 0;
            accepted_r = 0;
            completed_descriptors = 0;
            bus_issued_beats = 0;
            bus_returned_beats = 0;
            bus_outstanding = 0;
            outstanding_peak = 0;
            descriptor_resp_beats = 0;
            reservation_count = 0;
            frozen_valid = 0;
        end
        else
        begin
            cycle_count = cycle_count + 1;

            if (start && opcode == 2 && done && configure_cycle < 0)
                configure_cycle = cycle_count;
            if (burst_engine.fifo_load_done && burst_engine.consumed_reg == N-1 &&
                final_consume_cycle < 0)
                final_consume_cycle = cycle_count;
            if (rvalid && first_rvalid_cycle < 0)
                first_rvalid_cycle = cycle_count;

            // Snapshot the descriptor at its reservation edge, using the
            // pre-edge state used by the unchanged engine implementation.
            if (burst_engine.reservation_eligible)
                begin
                exp_remaining = burst_engine.region_count_reg - burst_engine.issued_reg;
                exp_addr = burst_engine.region_base_reg + (burst_engine.issued_reg << 2);
                exp_boundary = (4096 - (exp_addr & 4095)) / 4;
                exp_credit = FIFO_DEPTH - (burst_engine.issued_reg - burst_engine.consumed_reg);
                exp_beats = B_MAX;
                if (exp_beats > exp_remaining) exp_beats = exp_remaining;
                if (exp_beats > exp_boundary) exp_beats = exp_boundary;
                if (exp_beats > exp_credit) exp_beats = exp_credit;
                if (burst_engine.next_address != exp_addr ||
                    (burst_engine.selected_arlen + 1) != exp_beats || exp_beats <= 0)
                    $fatal(1, "reservation mismatch addr=%h beats=%0d expected addr=%h beats=%0d rem=%0d boundary=%0d credit=%0d",
                           burst_engine.next_address, burst_engine.selected_arlen + 1,
                           exp_addr, exp_beats, exp_remaining, exp_boundary, exp_credit);
                frozen_addr = exp_addr;
                frozen_beats = exp_beats;
                frozen_valid = 1;
                reservation_count = reservation_count + 1;
            end
            if (burst_engine.m_axi_arvalid_reg)
            begin
                if (!frozen_valid || araddr !== frozen_addr ||
                    arlen + 1 !== frozen_beats)
                    $fatal(1, "reserved descriptor changed before AR handshake");
            end

            push = arvalid && arready;
            pop = rvalid && rready && rlast;
            if (push)
            begin
                if (arsize != 2 || arburst != 2'b01 || arid != 0)
                    $fatal(1, "unexpected AXI INCR profile");
                if (araddr !== BASE + 4 * bus_issued_beats)
                    $fatal(1, "AR order/address mismatch: got=%h expected=%h",
                           araddr, BASE + 4 * bus_issued_beats);
                if ((araddr[11:0] + ((arlen + 1) << 2)) > 4096)
                    $fatal(1, "AR burst crossed a 4-KiB boundary");
                if (arlen + 1 > B_MAX || arlen + 1 > N - bus_issued_beats)
                    $fatal(1, "AR length exceeds B_MAX or remaining N");
                if (first_ar_cycle < 0) first_ar_cycle = cycle_count;
                accepted_beats[accepted_ar] = arlen + 1;
                bus_issued_beats = bus_issued_beats + arlen + 1;
                accepted_ar = accepted_ar + 1;
                frozen_valid = 0;
            end
            if (rvalid && rready)
            begin
                if (rid != 0 || rresp != 0)
                    $fatal(1, "bad R ID/response");
                if (bus_returned_beats >= bus_issued_beats)
                    $fatal(1, "R beat without previously accepted AR credit");
                if (rdata !== ((BASE >> 2) + bus_returned_beats))
                    $fatal(1, "R data/order mismatch at beat %0d", bus_returned_beats);
            if (descriptor_resp_beats == 0)
            begin
                if (completed_descriptors >= accepted_ar)
                    $fatal(1, "response descriptor without accepted AR");
                end
                if (rlast !== (descriptor_resp_beats ==
                               accepted_beats[completed_descriptors] - 1))
                    $fatal(1, "RLAST mismatch descriptor=%0d beat=%0d expected_beats=%0d",
                           completed_descriptors, descriptor_resp_beats,
                           accepted_beats[completed_descriptors]);
                descriptor_resp_beats = descriptor_resp_beats + 1;
                if (rlast)
                begin
                    descriptor_resp_beats = 0;
                    completed_descriptors = completed_descriptors + 1;
                end
                bus_returned_beats = bus_returned_beats + 1;
                accepted_r = accepted_r + 1;
            end
            bus_outstanding = bus_outstanding + push - pop;
            if (bus_outstanding < 0 || bus_outstanding > MAX_OUTSTANDING)
                $fatal(1, "observed AXI descriptor occupancy=%0d outside [0,%0d]",
                       bus_outstanding, MAX_OUTSTANDING);
            if (bus_outstanding > outstanding_peak) outstanding_peak = bus_outstanding;
        end
    end

    // Observe post-NBA state but retain the edge number of the transition.
    // This makes drain_cycle the edge that completes the final CPU pop and
    // final accepted RLAST, rather than the following sampling edge.
    always @(posedge clock)
    begin
        #1;
        if (reset && drain_cycle < 0 && burst_engine.drain_complete)
            drain_cycle = cycle_count;
    end

    function automatic integer geometric_desc_min(input integer beats, input integer max_beats);
        integer remaining;
        integer addr;
        integer boundary;
        integer one_burst;
        begin
            remaining = beats;
            addr = BASE;
            geometric_desc_min = 0;
            while (remaining > 0)
            begin
                boundary = (4096 - (addr & 4095)) / 4;
                one_burst = max_beats;
                if (one_burst > remaining) one_burst = remaining;
                if (one_burst > boundary) one_burst = boundary;
                remaining = remaining - one_burst;
                addr = addr + 4 * one_burst;
                geometric_desc_min = geometric_desc_min + 1;
            end
        end
    endfunction

    task automatic issue_load(input integer index);
        begin
            start = 1'b1;
            opcode = 2'd0;
            size_bits = 32'd32;
            byte_address = BASE + 4 * index;
            #1;
            if (done)
            begin
                if (data_out !== ((BASE >> 2) + index))
                    $fatal(1, "immediate load %0d got %h", index, data_out);
                @(posedge clock);
            end
            else
            begin
                @(posedge clock); // Capture one pending load command.
                @(negedge clock);
                start = 1'b0;
                wait (done);
                if (data_out !== ((BASE >> 2) + index))
                    $fatal(1, "pending load %0d got %h", index, data_out);
                @(posedge clock); // Consume; next request is driven next edge.
            end
        end
    endtask

    initial
    begin : run_floor
        integer index;
        integer latency;
        integer configure_to_consume;
        integer configure_to_drain;
        if (N != 1024 && N != 2048)
            $fatal(1, "N1b only accepts N=1024 or 2048");
        if (AXI_LATENCY != 64 || READ_DELAY != 63)
            $fatal(1, "N1b requires declared L=64 with TestbenchMEMAXI READ_DELAY=63");
        if (MAX_OUTSTANDING < 1 || MAX_OUTSTANDING > 16 ||
            FIFO_DEPTH < 1 || FIFO_DEPTH > 4096 ||
            (FIFO_DEPTH & (FIFO_DEPTH-1)) != 0)
            $fatal(1, "invalid engine profile");
        n_desc_min = geometric_desc_min(N, B_MAX);
        repeat (4) @(negedge clock);
        reset = 1'b1;
        @(negedge clock);
        start = 1'b1;
        opcode = 2'd2;
        size_bits = 32'd32;
        byte_address = BASE;
        count = N;
        #1;
        latency = 0;
        while (!done && latency < 16)
        begin
            @(negedge clock); #1;
            latency = latency + 1;
        end
        if (!done) $fatal(1, "configure command was not acknowledged");
        @(posedge clock);
        @(negedge clock);
        for (index = 0; index < N; index = index + 1)
        begin
            issue_load(index);
            if (index + 1 < N) @(negedge clock);
        end
        @(negedge clock);
        start = 1'b0;
        wait (drain_cycle >= 0);
        @(negedge clock);

        if (fault) $fatal(1, "burst engine raised fault");
        if (bus_issued_beats != N || bus_returned_beats != N ||
            burst_engine.consumed_reg != N || burst_engine.fifo_count_reg != 0 ||
            bus_outstanding != 0 || burst_engine.descriptor_count_reg != 0 ||
            accepted_r != N || completed_descriptors != accepted_ar ||
            descriptor_resp_beats != 0)
            $fatal(1, "engine failed final drain invariants issued=%0d returned=%0d consumed=%0d outstanding=%0d fifo=%0d",
                   bus_issued_beats, bus_returned_beats, burst_engine.consumed_reg,
                   bus_outstanding, burst_engine.fifo_count_reg);
        if (configure_cycle < 0 || final_consume_cycle < 0 || drain_cycle < 0)
            $fatal(1, "missing configure/consume/drain endpoint");
        latency = first_rvalid_cycle - first_ar_cycle;
        if (latency < 63 || latency > 65)
            $fatal(1, "N1b AR-to-first-RVALID=%0d outside [63,65]", latency);
        configure_to_consume = final_consume_cycle - configure_cycle;
        configure_to_drain = drain_cycle - configure_cycle;
        if (configure_to_consume < 0 || configure_to_drain < configure_to_consume)
            $fatal(1, "invalid cycle endpoints consume=%0d drain=%0d",
                   configure_to_consume, configure_to_drain);
        $display("T4B_N1B PASS B=%0d O=%0d D=%0d N=%0d L=%0d READ_DELAY=%0d Q=%0d C_consume=%0d C_drain=%0d C_engine_floor=%0d n_desc_min=%0d n_desc=%0d outstanding_peak=%0d L_axi=%0d reservations=%0d",
                 B_MAX, MAX_OUTSTANDING, FIFO_DEPTH, N, AXI_LATENCY, READ_DELAY, QUEUE_SIZE,
                 configure_to_consume, configure_to_drain, configure_to_drain,
                 n_desc_min, accepted_ar, outstanding_peak, latency, reservation_count);
        $finish;
    end

    initial
    begin
        #50000000;
        $fatal(1, "N1b watchdog expired B=%0d O=%0d D=%0d N=%0d", B_MAX,
               MAX_OUTSTANDING, FIFO_DEPTH, N);
    end
endmodule
