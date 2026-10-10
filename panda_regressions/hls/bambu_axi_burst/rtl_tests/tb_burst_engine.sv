module tb_burst_engine #(
    parameter integer B_MAX = 16,
    parameter integer MAX_OUTSTANDING = 1,
    parameter integer FIFO_DEPTH = 256
);
    logic clk = 1'b0;
    always #5 clk <= ~clk;

    logic reset_n = 1'b0;
    logic start = 1'b0;
    logic [1:0] opcode = 2'b0;
    logic [31:0] size_bits = 32'd32;
    logic [31:0] payload = 32'b0;
    logic [31:0] byte_address = 32'b0;
    logic [31:0] count = 32'b0;
    logic done;
    logic [31:0] data_out;
    logic fault;

    logic [31:0] araddr;
    logic [7:0] arlen;
    logic [2:0] arsize;
    logic [1:0] arburst;
    logic [0:0] arid;
    logic arvalid;
    logic arready = 1'b0;
    logic [31:0] rdata;
    logic [1:0] rresp;
    logic [0:0] rid;
    logic rlast;
    logic rvalid = 1'b0;
    logic rready;

    wire slave_active;
    logic stall_ar = 1'b1;
    logic inject_error = 1'b0;
    logic inject_wrong_rid = 1'b0;
    logic inject_early_rlast = 1'b0;
    logic suppress_rlast = 1'b0;
    wire [31:0] slave_addr;
    wire [31:0] slave_left;
    reg [31:0] slave_q_addr [0:15];
    integer slave_q_left [0:15];
    integer slave_q_rd = 0;
    integer slave_q_wr = 0;
    integer slave_q_count = 0;
    integer hold_responses_until = 0;
    integer peak_descriptors = 0;
    integer delay_left = 0;
    integer ar_count = 0;
    integer deferred_cfg_done = 0;
    integer configure_completion_count = 0;
    logic prev_stalled = 1'b0;
    logic [31:0] stalled_addr;
    logic [7:0] stalled_len;
    logic [31:0] seen_addr [0:2047];
    logic [7:0] seen_len [0:2047];
    integer r_beat_count = 0;
    integer before_ar;
    integer fifo_write_wraps = 0;
    integer fifo_read_wraps = 0;
    integer descriptor_write_wraps = 0;
    integer descriptor_read_wraps = 0;
    integer simultaneous_push_pop = 0;
    integer monitor_reserved_beats;
    integer bus_issued_beats = 0;
    integer bus_returned_beats = 0;
    integer bus_desc_count = 0;
    integer bus_desc_rd_ptr = 0;
    integer bus_desc_wr_ptr = 0;
    integer bus_desc_remaining [0:15];
    integer bus_region_count = 0;
    integer bus_desc_sum;
    integer bus_reserved_beats;
    integer bus_r_is_good;
    integer bus_r_finishes_desc;
    wire bus_r_finishes_desc_now = rvalid && rready && bus_desc_count != 0 &&
        rid == 0 && rresp == 0 && rlast &&
        bus_desc_remaining[bus_desc_rd_ptr] == 1;
    integer aqc_hits = 0;
    integer stalled_fault_ar_hits = 0;
    integer multi_reset_hits = 0;

    MinimalAXI4MasterPipelined #(.B_MAX(B_MAX),
        .MAX_OUTSTANDING(MAX_OUTSTANDING), .FIFO_DEPTH(FIFO_DEPTH)) dut (
        .clock(clk), .reset(reset_n), .start(start), .in1(opcode),
        .in2(size_bits), .in3(payload), .in4(byte_address),
        .in5(count), .done(done), .out1(data_out), .fault(fault),
        .m_axi_araddr(araddr), .m_axi_arlen(arlen), .m_axi_arsize(arsize),
        .m_axi_arburst(arburst), .m_axi_arid(arid), .m_axi_arvalid(arvalid),
        .m_axi_arready(arready), .m_axi_rdata(rdata), .m_axi_rresp(rresp),
        .m_axi_rid(rid), .m_axi_rlast(rlast), .m_axi_rvalid(rvalid),
        .m_axi_rready(rready)
    );

    assign slave_active = slave_q_count != 0;
    assign slave_addr = slave_q_count != 0 ? slave_q_addr[slave_q_rd] : 32'b0;
    assign slave_left = slave_q_count != 0 ? slave_q_left[slave_q_rd] : 0;
    assign rdata = slave_addr >> 2;
    assign rid = inject_wrong_rid ? 1'b1 : 1'b0;
    assign rlast = suppress_rlast ? 1'b0 :
                   (inject_early_rlast ? (slave_left == 2) : (slave_left == 1));
    assign rresp = inject_error ? 2'b10 : 2'b00;
    assign arready = !stall_ar && (slave_q_count < 16 ||
                   (rvalid && rready && slave_left == 1));

    always @(posedge clk) begin
        integer descriptor_sum;
        integer accepted_ar_beats;
        if (reset_n && dut.configure_complete)
            configure_completion_count <= configure_completion_count + 1;
        if (reset_n && dut.activate_pending)
            deferred_cfg_done <= deferred_cfg_done + 1;
        if (!reset_n) begin
            rvalid <= 1'b0;
            slave_q_rd <= 0;
            slave_q_wr <= 0;
            slave_q_count <= 0;
            delay_left <= 0;
            ar_count <= 0;
            deferred_cfg_done <= 0;
            configure_completion_count <= 0;
            r_beat_count <= 0;
            fifo_write_wraps <= 0;
            fifo_read_wraps <= 0;
            simultaneous_push_pop <= 0;
            descriptor_write_wraps <= 0;
            descriptor_read_wraps <= 0;
            peak_descriptors <= 0;
            bus_issued_beats = 0;
            bus_returned_beats = 0;
            bus_desc_count = 0;
            bus_desc_rd_ptr = 0;
            bus_desc_wr_ptr = 0;
            bus_region_count = 0;
            aqc_hits = 0;
            stalled_fault_ar_hits = 0;
            multi_reset_hits = 0;
            prev_stalled <= 1'b0;
        end else begin
            if (dut.burst_outstanding !== (dut.descriptor_count != 0))
                $fatal(1, "Phase-A burst_outstanding alias disagrees with descriptor_count: O=%0d count=%0d alias=%b",
                       MAX_OUTSTANDING, dut.descriptor_count, dut.burst_outstanding);
            // An independent bus-handshake model checks accounting; it does
            // not use DUT counters to derive accepted beats or descriptor sums.
            if (dut.configure_direct_capture) begin
                bus_issued_beats = 0;
                bus_returned_beats = 0;
                bus_desc_count = 0;
                bus_desc_rd_ptr = 0;
                bus_desc_wr_ptr = 0;
                bus_region_count = count;
            end else if (dut.activate_pending) begin
                bus_issued_beats = 0;
                bus_returned_beats = 0;
                bus_desc_count = 0;
                bus_desc_rd_ptr = 0;
                bus_desc_wr_ptr = 0;
                bus_region_count = dut.pending_count;
            end

            bus_reserved_beats = arvalid ? (arlen + 1) : 0;
            bus_desc_sum = 0;
            for (integer bi = 0; bi < MAX_OUTSTANDING; bi = bi + 1)
                if (bi < bus_desc_count)
                    bus_desc_sum = bus_desc_sum + bus_desc_remaining[(bus_desc_rd_ptr + bi) % MAX_OUTSTANDING];
            bus_r_is_good = rvalid && rready && bus_desc_count != 0 &&
                rid == 0 && rresp == 0 &&
                ((bus_desc_remaining[bus_desc_rd_ptr] == 1 && rlast) ||
                 (bus_desc_remaining[bus_desc_rd_ptr] > 1 && !rlast));
            bus_r_finishes_desc = bus_r_is_good &&
                bus_desc_remaining[bus_desc_rd_ptr] == 1;

            // Check accounting against pre-edge state. Skip the edge on which
            // reset/reconfiguration atomically replaces the region counters.
            if (dut.region_active && !fault && !dut.configure_direct_capture && !dut.activate_pending) begin
                if (dut.consumed > dut.returned || dut.returned > dut.issued ||
                    dut.issued > dut.region_count)
                    $fatal(1, "burst counter ordering violated: consumed=%0d returned=%0d issued=%0d count=%0d",
                           dut.consumed, dut.returned, dut.issued, dut.region_count);
                if (dut.fifo_count != (dut.returned - dut.consumed) ||
                    dut.fifo_count > FIFO_DEPTH)
                    $fatal(1, "FIFO occupancy/counter mismatch: fifo=%0d returned=%0d consumed=%0d",
                           dut.fifo_count, dut.returned, dut.consumed);
                monitor_reserved_beats = dut.m_axi_arvalid_reg ?
                    ({24'b0, dut.m_axi_arlen_reg} + 1) : 0;
                if ((dut.issued - dut.consumed + monitor_reserved_beats) > FIFO_DEPTH)
                    $fatal(1, "burst credit limit exceeded: issued=%0d consumed=%0d reserved=%0d",
                           dut.issued, dut.consumed, monitor_reserved_beats);
                if (dut.issued + monitor_reserved_beats > dut.region_count)
                    $fatal(1, "DUT issued+reserved exceeds configured region: issued=%0d reserved=%0d count=%0d",
                           dut.issued, monitor_reserved_beats, dut.region_count);
                descriptor_sum = 0;
                for (integer di = 0; di < MAX_OUTSTANDING; di = di + 1)
                    if (di < dut.descriptor_count)
                        descriptor_sum = descriptor_sum + dut.descriptor_remaining[(dut.descriptor_rd_ptr + di) % MAX_OUTSTANDING];
                if (dut.descriptor_count > MAX_OUTSTANDING ||
                    dut.descriptor_count + dut.m_axi_arvalid_reg > MAX_OUTSTANDING ||
                    descriptor_sum != (dut.issued - dut.returned))
                    $fatal(1, "descriptor invariant violated count=%0d sum=%0d outstanding=%0d issued-returned=%0d",
                           dut.descriptor_count, descriptor_sum,
                           dut.outstanding_after_reservation, dut.issued-dut.returned);
                if (bus_issued_beats != dut.issued || bus_returned_beats != dut.returned ||
                    bus_desc_count != dut.descriptor_count ||
                    bus_desc_sum != (bus_issued_beats - bus_returned_beats))
                    $fatal(1, "independent bus scoreboard mismatch: ARbeats=%0d Rbeats=%0d desc=%0d sum=%0d; DUT issued=%0d returned=%0d desc=%0d",
                           bus_issued_beats, bus_returned_beats, bus_desc_count,
                           bus_desc_sum, dut.issued, dut.returned, dut.descriptor_count);
                if (bus_issued_beats + bus_reserved_beats > bus_region_count)
                    $fatal(1, "bus-issued+reserved exceeds configured region: issued=%0d reserved=%0d region=%0d",
                           bus_issued_beats, bus_reserved_beats, bus_region_count);
                if (dut.descriptor_count > peak_descriptors)
                    peak_descriptors <= dut.descriptor_count;
            end
            if (dut.r_good && dut.fifo_wr_ptr == FIFO_DEPTH-1)
                fifo_write_wraps <= fifo_write_wraps + 1;
            if (dut.fifo_load_pop && dut.fifo_rd_ptr == FIFO_DEPTH-1)
                fifo_read_wraps <= fifo_read_wraps + 1;
            if (dut.r_push && dut.fifo_load_pop)
                simultaneous_push_pop <= simultaneous_push_pop + 1;
            if (arvalid && arready && dut.descriptor_pop &&
                bus_r_finishes_desc_now && dut.fifo_load_pop)
                aqc_hits = aqc_hits + 1;
            if (arvalid && arready && dut.descriptor_wr_ptr == MAX_OUTSTANDING-1)
                descriptor_write_wraps <= descriptor_write_wraps + 1;
            if (dut.descriptor_pop && dut.descriptor_rd_ptr == MAX_OUTSTANDING-1)
                descriptor_read_wraps <= descriptor_read_wraps + 1;
            if (prev_stalled && (araddr !== stalled_addr || arlen !== stalled_len || !arvalid))
                $fatal(1, "AR descriptor changed while stalled");
            prev_stalled <= arvalid && !arready;
            if (arvalid && !arready) begin
                stalled_addr <= araddr;
                stalled_len <= arlen;
            end

            if (arvalid && arready) begin
                if (arsize != 2 || arburst != 2'b01 || arid != 0)
                    $fatal(1, "unexpected AXI profile");
                if (ar_count >= 2048)
                    $fatal(1, "descriptor scoreboard overflow");
                seen_addr[ar_count] <= araddr;
                seen_len[ar_count] <= arlen;
                accepted_ar_beats = arlen + 1;
                bus_issued_beats = bus_issued_beats + accepted_ar_beats;
                bus_desc_remaining[bus_desc_wr_ptr] = accepted_ar_beats;
                if (bus_desc_wr_ptr == MAX_OUTSTANDING-1) bus_desc_wr_ptr = 0;
                else bus_desc_wr_ptr = bus_desc_wr_ptr + 1;
                ar_count <= ar_count + 1;
                slave_q_addr[slave_q_wr] <= araddr;
                slave_q_left[slave_q_wr] <= {24'b0, arlen} + 1;
                if (slave_q_wr == 15) slave_q_wr <= 0;
                else slave_q_wr <= slave_q_wr + 1;
                delay_left <= 3;
            end

            if (rvalid && rready) begin
                rvalid <= 1'b0;
                r_beat_count <= r_beat_count + 1;
                if (bus_r_is_good) begin
                    bus_returned_beats = bus_returned_beats + 1;
                    if (bus_r_finishes_desc) begin
                        if (bus_desc_rd_ptr == MAX_OUTSTANDING-1) bus_desc_rd_ptr = 0;
                        else bus_desc_rd_ptr = bus_desc_rd_ptr + 1;
                        bus_desc_count = bus_desc_count - 1;
                    end else begin
                        bus_desc_remaining[bus_desc_rd_ptr] =
                            bus_desc_remaining[bus_desc_rd_ptr] - 1;
                    end
                end
                if (slave_left == 1) begin
                    if (slave_q_rd == 15) slave_q_rd <= 0;
                    else slave_q_rd <= slave_q_rd + 1;
                end else begin
                    slave_q_addr[slave_q_rd] <= slave_addr + 4;
                    slave_q_left[slave_q_rd] <= slave_left - 1;
                    delay_left <= (r_beat_count % 3) + 1;
                end
            end else if (slave_active && !rvalid && ar_count >= hold_responses_until) begin
                if (delay_left == 0)
                    rvalid <= 1'b1;
                else
                    delay_left <= delay_left - 1;
            end
            case ({(arvalid && arready), (rvalid && rready && slave_left == 1)})
                2'b10: slave_q_count <= slave_q_count + 1;
                2'b01: slave_q_count <= slave_q_count - 1;
                default: slave_q_count <= slave_q_count;
            endcase
            if (arvalid && arready) begin
                // The count update follows the R retirement above, making a
                // same-edge descriptor completion/acceptance atomic.
                bus_desc_count = bus_desc_count + 1;
            end
        end
    end

    task automatic configure(input [31:0] base, input [31:0] n);
        begin
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = base;
            count = n;
            #1;
            if (!done)
                $fatal(1, "configure was not acknowledged at capture");
            @(posedge clk);
            @(negedge clk);
            start = 1'b0;
        end
    endtask

    task automatic configure_invalid(input [31:0] base, input [31:0] n,
                                     input [31:0] invalid_size);
        begin
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd2;
            size_bits = invalid_size;
            byte_address = base;
            count = n;
            #1;
            if (done)
                $fatal(1, "invalid configure asserted done before capture");
            @(posedge clk);
            #1;
            if (done)
                $fatal(1, "invalid configure asserted done on capture edge");
            @(negedge clk);
            start = 1'b0;
            wait (fault);
        end
    endtask

    // Keep a wide arithmetic oracle in the testbench while the RTL is free to
    // use a narrower overflow check. This mirrors the original 64-bit
    // last-byte calculation, including the zero-length special case.
    function automatic bit range_valid_64(input [31:0] base,
                                          input [31:0] n,
                                          input [31:0] bytes_per_beat);
        reg [63:0] last_byte;
        begin
            last_byte = {32'b0, base};
            if (n != 0)
                last_byte = {32'b0, base} +
                    ({32'b0, n} * {32'b0, bytes_per_beat}) - 64'd1;
            range_valid_64 = (base % bytes_per_beat == 0) &&
                             (last_byte <= 64'h00000000ffffffff);
        end
    endfunction

    function automatic [31:0] next_range_prng(input [31:0] state);
        reg [31:0] x;
        begin
            x = state;
            x = x ^ (x << 13);
            x = x ^ (x >> 17);
            x = x ^ (x << 5);
            next_range_prng = x;
        end
    endfunction

    task automatic configure_valid_range(input [31:0] base,
                                         input [31:0] n);
        begin
            if (!range_valid_64(base, n, 32'd4))
                $fatal(1, "test vector expected valid under 64-bit reference: base=%h count=%h",
                       base, n);
            configure(base, n);
            #1;
            if (fault || !dut.region_active || dut.region_count != n)
                $fatal(1, "valid configure range was rejected/mis-captured: base=%h count=%h fault=%b active=%b captured=%h",
                       base, n, fault, dut.region_active, dut.region_count);
            if (ar_count != 0)
                $fatal(1, "stalled range unexpectedly accepted AXI traffic: base=%h count=%h",
                       base, n);
        end
    endtask

    task automatic configure_invalid_range(input [31:0] base,
                                           input [31:0] n);
        begin
            if (range_valid_64(base, n, 32'd4))
                $fatal(1, "test vector expected invalid under 64-bit reference: base=%h count=%h",
                       base, n);
            configure_invalid(base, n, 32'd32);
            if (ar_count != 0)
                $fatal(1, "invalid configure range issued AXI traffic: base=%h count=%h",
                       base, n);
        end
    endtask

    task automatic reset_between_range_samples;
        begin
            @(negedge clk);
            start = 1'b0;
            reset_n = 1'b0;
            repeat (3) @(posedge clk);
            @(negedge clk);
            reset_n = 1'b1;
            repeat (2) @(posedge clk);
        end
    endtask

    task automatic capture_busy_config(input [31:0] base, input [31:0] n);
        begin
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = base;
            count = n;
            #1;
            if (done)
                $fatal(1, "busy configure completed before prior region drained");
            @(posedge clk);
            #1;
            if (!dut.cfg_pending || dut.pending_base != base || dut.pending_count != n)
                $fatal(1, "busy configure did not capture its pending descriptor");
            @(negedge clk);
            start = 1'b0;
        end
    endtask

    task automatic consume_pending(input [31:0] expected);
        integer watchdog;
        begin
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd0;
            size_bits = 32'd32;
            byte_address = dut.next_expected_address;
            #1;
            if (done)
                $fatal(1, "pending-load test unexpectedly had buffered data");
            @(posedge clk); // command captured while start is high
            @(negedge clk);
            start = 1'b0;
            // The captured pending command must not depend on live bus values.
            byte_address = 32'hdeadbeec;
            size_bits = 32'd16;
            stall_ar = 1'b0;
            watchdog = 0;
            while (!done && watchdog < 20000) begin
                @(negedge clk);
                #1;
                watchdog = watchdog + 1;
            end
            if (!done)
                $fatal(1, "pending load failed to complete with start low");
            if (data_out !== expected)
                $fatal(1, "pending load value %0d expected %0d", data_out, expected);
            @(posedge clk); // retire exactly one word
        end
    endtask

    task automatic consume_stream(input [31:0] base, input integer n);
        integer idx;
        integer watchdog;
        bit waiting_pending;
        begin
            idx = 0;
            watchdog = 0;
            waiting_pending = 1'b0;
            while (idx < n && watchdog < 100000) begin
                @(negedge clk);
                if (!waiting_pending)
                    start = 1'b1;
                else
                    start = 1'b0;
                opcode = 2'd0;
                size_bits = 32'd32;
                byte_address = base + (idx * 4);
                #1;
                if (done) begin
                    if (data_out !== ((base >> 2) + idx))
                        $fatal(1, "load %0d returned %0d expected %0d", idx,
                               data_out, (base >> 2) + idx);
                    @(posedge clk);
                    idx = idx + 1;
                    waiting_pending = 1'b0;
                end else if (!waiting_pending) begin
                    @(posedge clk); // capture exactly one pending load
                    waiting_pending = 1'b1;
                    @(negedge clk);
                    start = 1'b0;
                    #1;
                    if (done) begin
                        if (data_out !== ((base >> 2) + idx))
                            $fatal(1, "pending stream load %0d returned %0d expected %0d", idx,
                                   data_out, (base >> 2) + idx);
                        @(posedge clk);
                        idx = idx + 1;
                        waiting_pending = 1'b0;
                    end
                end
                watchdog = watchdog + 1;
            end
            @(negedge clk);
            start = 1'b0;
            if (idx != n)
                $fatal(1, "load stream timed out at %0d/%0d issued=%0d returned=%0d consumed=%0d fifo=%0d pending=%b outstanding=%b fault=%b",
                       idx, n, dut.issued, dut.returned, dut.consumed, dut.fifo_count,
                       dut.load_pending, dut.descriptor_count, fault);
        end
    endtask

    task automatic wait_drained;
        integer watchdog;
        begin
            watchdog = 0;
            while (dut.region_active && watchdog < 100000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.region_active)
                $fatal(1, "region did not drain: issued=%0d returned=%0d consumed=%0d count=%0d fifo=%0d arvalid=%b outstanding=%b pending=%b",
                       dut.issued, dut.returned, dut.consumed, dut.region_count,
                       dut.fifo_count, arvalid, dut.descriptor_count, dut.load_pending);
            if (dut.issued != dut.region_count || dut.returned != dut.region_count ||
                dut.consumed != dut.region_count || dut.fifo_count != 0)
                $fatal(1, "region counters failed drain invariant");
        end
    endtask

    task automatic consume_immediate_stream(input [31:0] base, input integer n);
        integer idx;
        begin
            for (idx = 0; idx < n; idx = idx + 1) begin
                @(negedge clk);
                start = 1'b1;
                opcode = 2'd0;
                size_bits = 32'd32;
                byte_address = base + (idx * 4);
                #1;
                if (!done || data_out !== ((base >> 2) + idx))
                    $fatal(1, "consecutive load %0d got done=%b value=%0d expected=%0d",
                           idx, done, data_out, (base >> 2) + idx);
                @(posedge clk);
            end
            @(negedge clk);
            start = 1'b0;
        end
    endtask

    task automatic check_descriptors(input [31:0] base, input integer n, input integer from_ar);
        integer idx;
        integer addr;
        integer rem;
        integer boundary;
        integer beats;
        integer maximum_expected;
        begin
            idx = from_ar;
            addr = base;
            rem = n;
            while (rem > 0) begin
                boundary = (4096 - (addr % 4096)) / 4;
                beats = {24'b0, seen_len[idx]} + 1;
                if (seen_addr[idx] !== addr || beats > B_MAX || beats > rem || beats > boundary)
                    $fatal(1, "descriptor %0d got addr=%h len=%0d remaining=%0d boundary=%0d",
                           idx-from_ar, seen_addr[idx], seen_len[idx], rem, boundary);
                maximum_expected = B_MAX;
                if (maximum_expected > FIFO_DEPTH) maximum_expected = FIFO_DEPTH;
                if (maximum_expected > rem) maximum_expected = rem;
                if (maximum_expected > boundary) maximum_expected = boundary;
                if (n <= 256 && FIFO_DEPTH >= B_MAX * MAX_OUTSTANDING &&
                    beats != maximum_expected)
                    $fatal(1, "descriptor %0d has non-maximal length %0d expected %0d",
                           idx-from_ar, beats, maximum_expected);
                addr = addr + 4 * beats;
                rem = rem - beats;
                idx = idx + 1;
            end
            if (ar_count != idx)
                $fatal(1, "unexpected descriptor count got %0d expected %0d", ar_count-from_ar, idx-from_ar);
        end
    endtask

    task automatic run_case(input [31:0] base, input integer n, input bit do_pending);
        begin
            before_ar = ar_count;
            stall_ar = 1'b1;
            configure(base, n);
            if (n == 0) begin
                repeat (8) @(posedge clk);
                if (ar_count != before_ar)
                    $fatal(1, "zero-count configure issued AR");
            end else begin
                if (do_pending) begin
                    consume_pending(base >> 2);
                    stall_ar = 1'b0;
                    if (n > 1)
                        consume_stream(base+4, n-1);
                end else begin
                    stall_ar = 1'b0;
                    wait (dut.returned == n);
                    consume_stream(base, n);
                end
                wait_drained();
                check_descriptors(base, n, before_ar);
            end
            repeat (3) @(posedge clk);
            if (fault)
                $fatal(1, "unexpected sticky protocol fault");
        end
    endtask

    task automatic test_invalid_load(input bit wrong_address, input bit full_fifo);
        reg [31:0] base;
        begin
            base = full_fifo ? 32'h00009000 : 32'h0000a000;
            stall_ar = !full_fifo;
            configure(base, full_fifo ? 256 : 4);
            if (full_fifo) begin
                wait (dut.returned == 256);
                if (dut.fifo_count != FIFO_DEPTH || dut.consumed != 0)
                    $fatal(1, "invalid-load full-FIFO setup failed");
            end else begin
                wait (arvalid);
                if (dut.fifo_count != 0 || dut.consumed != 0)
                    $fatal(1, "invalid-load empty-FIFO setup failed");
            end

            @(negedge clk);
            start = 1'b1;
            opcode = 2'd0;
            size_bits = wrong_address ? 32'd32 : 32'd16;
            byte_address = wrong_address ? base + 4 : base;
            #1;
            if (done)
                $fatal(1, "invalid scalar load asserted done (wrong_address=%b full=%b)",
                       wrong_address, full_fifo);
            @(posedge clk);
            @(negedge clk);
            start = 1'b0;
            wait (fault);
            if (done || dut.consumed != 0 ||
                dut.fifo_count != (full_fifo ? 256 : 0))
                $fatal(1, "invalid scalar load consumed data (wrong_address=%b full=%b)",
                       wrong_address, full_fifo);
            $display("PASS B=%0d invalid load rejected wrong_address=%b full=%b",
                     B_MAX, wrong_address, full_fifo);
            $finish;
        end
    endtask

    initial begin
        if ($test$plusargs("ERROR")) inject_error = 1'b1;
        repeat (4) @(posedge clk);
        reset_n = 1'b1;
        repeat (2) @(posedge clk);

        if ($test$plusargs("RANGE_ORACLE")) begin : range_oracle_case
            reg [31:0] prng_state;
            reg [31:0] random_base;
            reg [31:0] random_count;
            reg [63:0] max_valid_count;
            integer sample_index;
            integer valid_samples;
            integer invalid_samples;
            prng_state = 32'h6d2b79f5;
            valid_samples = 0;
            invalid_samples = 0;
            for (sample_index = 0; sample_index < 32; sample_index = sample_index + 1) begin
                prng_state = next_range_prng(prng_state);
                random_base = {prng_state[31:2], 2'b00};
                prng_state = next_range_prng(prng_state);
                max_valid_count = (64'h0000000100000000 - {32'b0, random_base}) >> 2;
                case (sample_index % 4)
                    0: random_count = {22'b0, prng_state[9:0]};
                    1: random_count = prng_state % (max_valid_count + 64'd1);
                    2: random_count = max_valid_count + 64'd1;
                    default: random_count = prng_state;
                endcase
                if (range_valid_64(random_base, random_count, 32'd4)) begin
                    valid_samples = valid_samples + 1;
                    configure_valid_range(random_base, random_count);
                end else begin
                    invalid_samples = invalid_samples + 1;
                    configure_invalid_range(random_base, random_count);
                end
                if (sample_index != 31)
                    reset_between_range_samples();
            end
            if (valid_samples < 8 || invalid_samples < 8)
                $fatal(1, "range oracle did not cover both outcomes: valid=%0d invalid=%0d",
                       valid_samples, invalid_samples);
            $display("PASS B=%0d seeded 32-sample range oracle (seed=0x6d2b79f5) valid=%0d invalid=%0d",
                     B_MAX, valid_samples, invalid_samples);
            $finish;
        end
        if ($test$plusargs("RANGE_ZERO")) begin
            configure_valid_range(32'h00000000, 32'h00000000);
            if (ar_count != 0 || dut.region_count != 0)
                $fatal(1, "zero-count configure was not an empty accepted range");
            $display("PASS B=%0d zero-count range accepted", B_MAX);
            $finish;
        end
        if ($test$plusargs("RANGE_LIMIT_BASE0")) begin
            configure_valid_range(32'h00000000, 32'h40000000);
            $display("PASS B=%0d base=0 count=0x40000000 accepted at 32-bit limit", B_MAX);
            $finish;
        end
        if ($test$plusargs("RANGE_LIMIT_BASE4")) begin
            configure_valid_range(32'h00000004, 32'h3fffffff);
            $display("PASS B=%0d base=4 count=0x3fffffff accepted at 32-bit limit", B_MAX);
            $finish;
        end
        if ($test$plusargs("RANGE_OVER_BASE4")) begin
            configure_invalid_range(32'h00000004, 32'h40000000);
            $display("PASS B=%0d base=4 count=0x40000000 overflow rejected", B_MAX);
            $finish;
        end
        if ($test$plusargs("RANGE_OVER_PLUS1")) begin
            configure_invalid_range(32'h00000000, 32'h40000001);
            $display("PASS B=%0d count=0x40000001 overflow rejected", B_MAX);
            $finish;
        end
        if ($test$plusargs("RANGE_OVER_HALF")) begin
            configure_invalid_range(32'h00000000, 32'h80000000);
            $display("PASS B=%0d count=0x80000000 overflow rejected", B_MAX);
            $finish;
        end
        if ($test$plusargs("RANGE_OVER_MAX")) begin
            configure_invalid_range(32'h00000000, 32'hffffffff);
            $display("PASS B=%0d count=0xffffffff overflow rejected", B_MAX);
            $finish;
        end
        if ($test$plusargs("RANGE_TOP_VALID")) begin
            configure_valid_range(32'hfffffffc, 32'h00000001);
            $display("PASS B=%0d last aligned beat at 0xffffffff accepted", B_MAX);
            $finish;
        end
        if ($test$plusargs("RANGE_TOP_OVER")) begin
            configure_invalid_range(32'hfffffffc, 32'h00000002);
            $display("PASS B=%0d range beyond 0xffffffff rejected", B_MAX);
            $finish;
        end
        if ($test$plusargs("BADALIGN")) begin
            configure_invalid(32'h00001002, 1, 32'd32);
            if (ar_count != 0)
                $fatal(1, "misaligned range issued AXI traffic");
            $display("PASS B=%0d misaligned range rejected", B_MAX);
            $finish;
        end
        if ($test$plusargs("OVERFLOW")) begin
            configure_invalid(32'hfffffffc, 2, 32'd32);
            if (ar_count != 0)
                $fatal(1, "overflowing range issued AXI traffic");
            $display("PASS B=%0d overflowing range rejected", B_MAX);
            $finish;
        end
        if ($test$plusargs("BADSIZE_CONFIG")) begin
            configure_invalid(32'h00001000, 1, 32'd16);
            if (ar_count != 0)
                $fatal(1, "malformed-size configure issued AXI traffic");
            $display("PASS B=%0d malformed configure size rejected", B_MAX);
            $finish;
        end
        if ($test$plusargs("STORE")) begin
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd1;
            #1;
            if (done)
                $fatal(1, "unsupported store completed successfully");
            @(posedge clk);
            @(negedge clk);
            start = 1'b0;
            wait (fault);
            if (done || ar_count != 0)
                $fatal(1, "unsupported store reported success or issued traffic");
            $display("PASS B=%0d unsupported store rejected", B_MAX);
            $finish;
        end
        if ($test$plusargs("BADADDR_EMPTY")) begin
            test_invalid_load(1'b1, 1'b0);
        end
        if ($test$plusargs("BADSIZE_EMPTY")) begin
            test_invalid_load(1'b0, 1'b0);
        end
        if ($test$plusargs("BADADDR_FULL")) begin
            test_invalid_load(1'b1, 1'b1);
        end
        if ($test$plusargs("BADSIZE_FULL")) begin
            test_invalid_load(1'b0, 1'b1);
        end

        if ($test$plusargs("BADRID") || $test$plusargs("EARLY_RLAST") ||
            $test$plusargs("MISSING_RLAST")) begin
            integer watchdog;
            integer beats_before;
            inject_wrong_rid = $test$plusargs("BADRID");
            inject_early_rlast = $test$plusargs("EARLY_RLAST");
            suppress_rlast = $test$plusargs("MISSING_RLAST");
            stall_ar = 1'b0;
            beats_before = r_beat_count;
            configure(32'h0000f000, 4);
            watchdog = 0;
            while (!fault && watchdog < 20000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (!fault || r_beat_count <= beats_before || done)
                $fatal(1, "AXI protocol fault not surfaced (RID=%b early_RLAST=%b missing_RLAST=%b beats=%0d done=%b)",
                       inject_wrong_rid, inject_early_rlast, suppress_rlast,
                       r_beat_count - beats_before, done);
            $display("PASS B=%0d AXI response fault RID=%b early_RLAST=%b missing_RLAST=%b beats=%0d",
                     B_MAX, inject_wrong_rid, inject_early_rlast, suppress_rlast,
                     r_beat_count - beats_before);
            $finish;
        end

        if ($test$plusargs("RESET_TRAFFIC")) begin
            integer watchdog;
            stall_ar = 1'b0;
            configure(32'h00013000, 16);
            watchdog = 0;
            while ((!slave_active || dut.descriptor_count == 0 || rvalid || !rready) &&
                   watchdog < 20000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (!slave_active || dut.descriptor_count == 0 || rvalid || !rready)
                $fatal(1, "reset-under-traffic gap setup failed: RVALID=%b active=%b outstanding=%b RREADY=%b",
                       rvalid, slave_active, dut.descriptor_count, rready);
            // Reset during the response gap, not on a cycle that would handshake.
            reset_n = 1'b0;
            @(posedge clk);
            #1;
            if (slave_active || rvalid || slave_left != 0 ||
                dut.region_active || dut.descriptor_count != 0 || dut.m_axi_arvalid_reg ||
                dut.fifo_count != 0 || dut.issued != 0 || dut.returned != 0 || dut.consumed != 0)
                $fatal(1, "coordinated reset failed to discard in-flight AXI state");
            repeat (2) @(negedge clk);
            reset_n = 1'b1;
            repeat (2) @(posedge clk);
            configure(32'h00014000, 5);
            wait (dut.returned == 5);
            consume_stream(32'h00014000, 5);
            wait_drained();
            check_descriptors(32'h00014000, 5, 0);
            $display("PASS B=%0d coordinated reset in outstanding-burst response gap, then clean recovery",
                     B_MAX);
            $finish;
        end

        if ($test$plusargs("RESET_MULTI")) begin
            integer watchdog;
            integer restarted_n;
            integer reset_descriptors;
            restarted_n = 20;
            stall_ar = 1'b0;
            hold_responses_until = 2;
            configure(32'h00022000, 64);
            watchdog = 0;
            while (dut.descriptor_count < 2 && watchdog < 100000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.descriptor_count < 2 || bus_desc_count < 2 || ar_count < 2 || r_beat_count != 0)
                $fatal(1, "multi-reset setup lacks two accepted descriptors: DUT=%0d bus=%0d AR=%0d R=%0d",
                       dut.descriptor_count, bus_desc_count, ar_count, r_beat_count);
            reset_descriptors = bus_desc_count;
            @(negedge clk);
            reset_n = 1'b0;
            @(posedge clk);
            #1;
            if (dut.descriptor_count != 0 || bus_desc_count != 0 || slave_q_count != 0 ||
                dut.m_axi_arvalid_reg || dut.issued != 0 || dut.returned != 0 || dut.consumed != 0)
                $fatal(1, "coordinated reset did not discard multiple accepted descriptors");
            repeat (2) @(negedge clk);
            reset_n = 1'b1;
            multi_reset_hits = 1;
            hold_responses_until = 2;
            configure(32'h00023000, restarted_n);
            watchdog = 0;
            while (dut.descriptor_count < 2 && watchdog < 100000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.descriptor_count < 2)
                $fatal(1, "post-reset region did not restart multiple outstanding descriptors");
            hold_responses_until = 0;
            consume_stream(32'h00023000, restarted_n);
            wait_drained();
            check_descriptors(32'h00023000, restarted_n, 0);
            if (multi_reset_hits == 0)
                $fatal(1, "required multi-descriptor coordinated reset coverage missed");
            $display("PASS coordinated reset discarded %0d accepted descriptors then restarted cleanly",
                     reset_descriptors);
            $finish;
        end

        if ($test$plusargs("FAULT_STALLED_AR")) begin
            integer watchdog;
            integer issued_before_fault;
            integer returned_before_fault;
            integer fifo_before_fault;
            integer accepted_before_fault;
            reg [31:0] held_addr;
            reg [7:0] held_len;
            stall_ar = 1'b0;
            hold_responses_until = 2;
            configure(32'h00024000, 64);
            watchdog = 0;
            while (dut.descriptor_count != 2 && watchdog < 100000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.descriptor_count != 2 || ar_count != 2)
                $fatal(1, "fault/stalled-AR setup did not accept two descriptors");
            stall_ar = 1'b1;
            hold_responses_until = 0;
            watchdog = 0;
            while ((!arvalid || !rvalid || !rready || !prev_stalled) && watchdog < 100000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (!arvalid || !rvalid || !rready || !prev_stalled)
                $fatal(1, "fault/stalled-AR setup lacks stalled AR plus accepted-response opportunity");
            held_addr = araddr;
            held_len = arlen;
            issued_before_fault = dut.issued;
            returned_before_fault = dut.returned;
            fifo_before_fault = dut.fifo_count;
            accepted_before_fault = ar_count;
            inject_wrong_rid = 1'b1;
            #1;
            if (!arvalid || arready || araddr != held_addr || arlen != held_len || !rready)
                $fatal(1, "failed to align malformed R under a stable stalled AR");
            @(posedge clk);
            #1;
            inject_wrong_rid = 1'b0;
            if (!fault || dut.returned != returned_before_fault ||
                dut.fifo_count != fifo_before_fault || dut.consumed != 0 || done)
                $fatal(1, "malformed R was counted or consumed instead of faulting");
            repeat (3) begin
                @(negedge clk);
                #1;
                if (!arvalid || arready || araddr != held_addr || arlen != held_len ||
                    dut.issued != issued_before_fault || ar_count != accepted_before_fault)
                    $fatal(1, "stalled AR changed or a new reservation appeared after malformed R");
            end
            stalled_fault_ar_hits = stalled_fault_ar_hits + 1;
            stall_ar = 1'b0;
            watchdog = 0;
            while (ar_count != accepted_before_fault+1 && watchdog < 100000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (ar_count != accepted_before_fault+1 || arvalid ||
                dut.issued != issued_before_fault+held_len+1 ||
                seen_addr[accepted_before_fault] != held_addr ||
                seen_len[accepted_before_fault] != held_len ||
                dut.returned != returned_before_fault || dut.fifo_count != fifo_before_fault)
                $fatal(1, "faulted held AR did not handshake exactly once without accepting bad data");
            repeat (3) begin
                @(negedge clk);
                if (arvalid || ar_count != accepted_before_fault+1 || !fault ||
                    dut.returned != returned_before_fault ||
                    dut.fifo_count != fifo_before_fault)
                    $fatal(1, "new reservation/data acceptance continued after sticky fault");
            end
            if (stalled_fault_ar_hits == 0)
                $fatal(1, "required malformed-R/stalled-AR hit count is zero");
            $display("PASS malformed R with held AR: stable through handshake, no extra reservation, bad beat unconsumed hits=%0d",
                     stalled_fault_ar_hits);
            $finish;
        end

        if ($test$plusargs("BOUNDARY_PENDING")) begin
            integer watchdog;
            integer first_descriptor;
            integer target_n;
            target_n = 40;
            first_descriptor = ar_count;
            hold_responses_until = MAX_OUTSTANDING;
            stall_ar = 1'b0;
            configure(32'h00000ffc, target_n);
            watchdog = 0;
            while (dut.descriptor_count != 3 && watchdog < 100000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (MAX_OUTSTANDING < 3 || dut.descriptor_count != 3 ||
                ar_count-first_descriptor != 3 || r_beat_count != 0)
                $fatal(1, "multi boundary setup failed to hold three descriptors");
            if (seen_addr[first_descriptor] != 32'h00000ffc || seen_len[first_descriptor] != 0 ||
                seen_addr[first_descriptor+1] != 32'h00001000 || seen_len[first_descriptor+1] != 15 ||
                seen_addr[first_descriptor+2] != 32'h00001040 || seen_len[first_descriptor+2] != 15)
                $fatal(1, "4-KiB split/order incorrect with multiple outstanding descriptors");
            stall_ar = 1'b1;
            capture_busy_config(32'h00030000, 5);
            hold_responses_until = 0;
            stall_ar = 1'b0;
            consume_stream(32'h00000ffc, target_n);
            check_descriptors(32'h00000ffc, target_n, first_descriptor);
            watchdog = 0;
            while (dut.region_base != 32'h00030000 && watchdog < 100000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.region_base != 32'h00030000 || dut.region_count != 5 || dut.cfg_pending)
                $fatal(1, "pending configure did not activate after multi-descriptor drain");
            wait (dut.returned == 5);
            consume_stream(32'h00030000, 5);
            wait_drained();
            $display("PASS O=%0d 4-KiB boundary/tail and pending configure while three descriptors active",
                     MAX_OUTSTANDING);
            $finish;
        end

        if ($test$plusargs("PENDING_SECOND")) begin
            integer watchdog;
            stall_ar = 1'b0;
            hold_responses_until = 2;
            configure(32'h00032000, 64);
            watchdog = 0;
            while (dut.descriptor_count != 2 && watchdog < 100000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.descriptor_count != 2)
                $fatal(1, "second-pending setup lacks multiple outstanding descriptors");
            capture_busy_config(32'h00033000, 5);
            if (fault || !dut.cfg_pending || dut.pending_base != 32'h00033000)
                $fatal(1, "first busy configure was not retained");
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = 32'h00034000;
            count = 7;
            #1;
            if (done)
                $fatal(1, "distinct second pending configure completed");
            @(posedge clk);
            #1;
            if (!fault || !dut.cfg_pending || dut.pending_base != 32'h00033000 ||
                dut.pending_count != 5)
                $fatal(1, "second distinct busy configure did not fault/preserve first pending slot");
            if (ar_count != 2 || dut.descriptor_count != 2)
                $fatal(1, "second pending configure changed active descriptor accounting");
            $display("PASS second distinct configure faults while O=%0d descriptors and pending slot are active",
                     dut.descriptor_count);
            $finish;
        end

        if ($test$plusargs("DEPTH4096")) begin
            integer watchdog;
            integer target_n;
            target_n = 4100;
            stall_ar = 1'b0;
            configure(32'h00040000, target_n);
            watchdog = 0;
            while (dut.fifo_count != 4096 && watchdog < 1000000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.fifo_count != 4096 || dut.issued != 4096 ||
                dut.returned != 4096 || dut.consumed != 0 || rready || arvalid)
                $fatal(1, "FIFO_DEPTH=4096 full-credit stop invariant failed");
            consume_stream(32'h00040000, target_n);
            wait_drained();
            check_descriptors(32'h00040000, target_n, 0);
            if (fifo_write_wraps == 0 || fifo_read_wraps == 0)
                $fatal(1, "FIFO_DEPTH=4096 did not wrap data FIFO pointers");
            $display("PASS FIFO_DEPTH=4096 positive wrap/stop/restart N=%0d writes=%0d reads=%0d",
                     target_n, fifo_write_wraps, fifo_read_wraps);
            $finish;
        end

        if ($test$plusargs("FIFO_BACKPRESSURE")) begin
            integer watchdog;
            integer held_descriptors;
            stall_ar = 1'b0;
            configure(32'h00015000, 300);
            watchdog = 0;
            while (dut.returned != 256 && watchdog < 100000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.returned != FIFO_DEPTH || dut.fifo_count != FIFO_DEPTH ||
                dut.issued != FIFO_DEPTH || rready || arvalid || slave_active)
                $fatal(1, "FIFO credit limit failed: returned=%0d fifo=%0d issued=%0d RREADY=%b ARVALID=%b active=%b",
                       dut.returned, dut.fifo_count, dut.issued, rready, arvalid, slave_active);
            held_descriptors = ar_count;
            repeat (16) begin
                @(negedge clk);
                if (dut.fifo_count != FIFO_DEPTH || dut.issued != FIFO_DEPTH || dut.returned != FIFO_DEPTH ||
                    rready || arvalid || ar_count != held_descriptors)
                    $fatal(1, "full FIFO allowed additional traffic or failed to backpressure");
            end
            consume_stream(32'h00015000, 300);
            wait_drained();
            check_descriptors(32'h00015000, 300, 0);
            if (dut.consumed != 300 || dut.returned != 300 || dut.fifo_count != 0)
                $fatal(1, "FIFO did not resume and drain after consumer resumed");
            $display("PASS B=%0d FIFO full backpressure at 256 credits, resume/drain N=300",
                     B_MAX);
            $finish;
        end

        if ($test$plusargs("FIFO_WRAP")) begin
            integer watchdog;
            configure(32'h00016000, 600);
            fifo_write_wraps = 0;
            fifo_read_wraps = 0;
            simultaneous_push_pop = 0;
            stall_ar = 1'b0;
            watchdog = 0;
            while ((dut.fifo_count != 1 || !rvalid || !rready) && watchdog < 100000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.fifo_count != 1 || !rvalid || !rready)
                $fatal(1, "failed to align FIFO push/pop event: count=%0d RVALID=%b RREADY=%b",
                       dut.fifo_count, rvalid, rready);
            start = 1'b1;
            opcode = 2'd0;
            size_bits = 32'd32;
            byte_address = 32'h00016000;
            #1;
            if (!done || !dut.r_push || !dut.fifo_load_pop ||
                data_out !== (32'h00016000 >> 2))
                $fatal(1, "could not force simultaneous response push and FIFO pop");
            @(posedge clk);
            @(negedge clk);
            start = 1'b0;
            consume_stream(32'h00016004, 599);
            wait_drained();
            check_descriptors(32'h00016000, 600, 0);
            if (fifo_write_wraps < 2 || fifo_read_wraps < 2 ||
                simultaneous_push_pop == 0)
                $fatal(1, "FIFO stress lacked wrap/concurrent operations: write_wraps=%0d read_wraps=%0d simultaneous=%0d",
                       fifo_write_wraps, fifo_read_wraps, simultaneous_push_pop);
            $display("PASS B=%0d FIFO pointer wrap write=%0d read=%0d simultaneous_push_pop=%0d N=600",
                     B_MAX, fifo_write_wraps, fifo_read_wraps, simultaneous_push_pop);
            $finish;
        end

        if ($test$plusargs("AQC")) begin
            integer target_n;
            integer watchdog;
            integer descriptors_before;
            target_n = 64;
            descriptors_before = ar_count;
            hold_responses_until = 2;
            stall_ar = 1'b0;
            configure(32'h0001e000, target_n);
            watchdog = 0;
            while (dut.descriptor_count != MAX_OUTSTANDING && watchdog < 100000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.descriptor_count != 2 || ar_count-descriptors_before != 2)
                $fatal(1, "A+Q+C setup did not accept two descriptors");
            // Free a descriptor slot while holding its replacement AR. Keep
            // responses running until one is ready behind buffered data.
            stall_ar = 1'b1;
            hold_responses_until = 0;
            watchdog = 0;
            while ((!arvalid || !rvalid || !rready || dut.fifo_count == 0 ||
                    !rlast || bus_desc_count == 0 ||
                    bus_desc_remaining[bus_desc_rd_ptr] != 1) &&
                   watchdog < 100000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (!arvalid || !rvalid || !rready || dut.fifo_count == 0 ||
                !rlast || !bus_r_finishes_desc_now)
                $fatal(1, "could not align held AR with final R beat behind buffered data");
            start = 1'b1;
            opcode = 2'd0;
            size_bits = 32'd32;
            byte_address = 32'h0001e000;
            stall_ar = 1'b0;
            #1;
            if (!done || !arvalid || !arready || !rvalid || !rready ||
                !dut.descriptor_pop || !bus_r_finishes_desc_now || !dut.fifo_load_pop)
                $fatal(1, "failed to force simultaneous AR accept, RLAST queue pop, and FIFO load");
            @(posedge clk);
            #1;
            if (aqc_hits == 0)
                $fatal(1, "simultaneous A+Q+C event was not counted");
            @(negedge clk);
            start = 1'b0;
            consume_stream(32'h0001e004, target_n-1);
            wait_drained();
            check_descriptors(32'h0001e000, target_n, descriptors_before);
            if (aqc_hits == 0)
                $fatal(1, "required-hit A+Q+C coverage count is zero");
            $display("PASS required-hit A+Q+C coverage (A=AR handshake, Q=RLAST pop) hits=%0d independent AR/R scoreboard active",
                     aqc_hits);
            $finish;
        end

        if ($test$plusargs("MULTI")) begin
            integer target_n;
            integer watchdog;
            integer descriptors_before;
            target_n = B_MAX * MAX_OUTSTANDING + 2;
            descriptors_before = ar_count;
            hold_responses_until = MAX_OUTSTANDING;
            stall_ar = 1'b0;
            configure(32'h00018000, target_n);
            watchdog = 0;
            while (dut.descriptor_count != MAX_OUTSTANDING && watchdog < 100000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.descriptor_count != MAX_OUTSTANDING ||
                ar_count - descriptors_before != MAX_OUTSTANDING || r_beat_count != 0)
                $fatal(1, "failed to fill outstanding descriptor queue: O=%0d descriptors=%0d Rbeats=%0d",
                       MAX_OUTSTANDING, dut.descriptor_count, r_beat_count);
            if (dut.descriptor_count > peak_descriptors)
                peak_descriptors = dut.descriptor_count;
            if (peak_descriptors < MAX_OUTSTANDING)
                $fatal(1, "observed peak outstanding %0d expected %0d", peak_descriptors, MAX_OUTSTANDING);
            hold_responses_until = 0;
            consume_stream(32'h00018000, target_n);
            wait_drained();
            check_descriptors(32'h00018000, target_n, descriptors_before);
            if (descriptor_write_wraps == 0 || descriptor_read_wraps == 0)
                $fatal(1, "descriptor FIFO did not explicitly wrap O=%0d write=%0d read=%0d",
                       MAX_OUTSTANDING, descriptor_write_wraps, descriptor_read_wraps);
            $display("PASS O=%0d D=%0d reached peak outstanding=%0d before first R beat; descriptor wraps write=%0d read=%0d",
                     MAX_OUTSTANDING, FIFO_DEPTH, peak_descriptors,
                     descriptor_write_wraps, descriptor_read_wraps);
            $finish;
        end

        if ($test$plusargs("CREDIT_LIMIT")) begin
            integer target_n;
            integer expected_issue;
            integer expected_descriptors;
            integer watchdog;
            target_n = (MAX_OUTSTANDING == 1 && FIFO_DEPTH == 1) ? 8 : 40;
            expected_issue = (MAX_OUTSTANDING == 16 && FIFO_DEPTH == 32) ? 32 :
                             ((MAX_OUTSTANDING == 2 && FIFO_DEPTH == 8) ? 8 : 1);
            expected_descriptors = (MAX_OUTSTANDING == 16 && FIFO_DEPTH == 32) ? 2 : 1;
            hold_responses_until = 999999;
            stall_ar = 1'b0;
            configure(32'h0001a000, target_n);
            watchdog = 0;
            while (dut.issued != expected_issue && watchdog < 100000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.issued != expected_issue || dut.descriptor_count != expected_descriptors ||
                dut.issued - dut.consumed > FIFO_DEPTH ||
                (dut.m_axi_arvalid_reg && dut.issued-dut.consumed+
                 ({24'b0,dut.m_axi_arlen_reg}+1) > FIFO_DEPTH))
                $fatal(1, "credit limit failed O=%0d D=%0d issued=%0d desc=%0d ARVALID=%b",
                       MAX_OUTSTANDING, FIFO_DEPTH, dut.issued, dut.descriptor_count,
                       dut.m_axi_arvalid_reg);
            hold_responses_until = 0;
            consume_stream(32'h0001a000, target_n);
            wait_drained();
            check_descriptors(32'h0001a000, target_n, 0);
            $display("PASS credit-limited O=%0d D=%0d reserved=%0d issued first; stop/restart drained N=%0d",
                     MAX_OUTSTANDING, FIFO_DEPTH, expected_issue, target_n);
            $finish;
        end

        if ($test$plusargs("CREDIT_POP")) begin
            integer target_n;
            integer watchdog;
            target_n = 40;
            hold_responses_until = 2;
            stall_ar = 1'b0;
            configure(32'h0001c000, target_n);
            watchdog = 0;
            while (dut.issued != 32 && watchdog < 100000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.issued != 32 || dut.descriptor_count != 2)
                $fatal(1, "D-1 push/pop setup did not reserve two bursts");
            hold_responses_until = 0;
            watchdog = 0;
            while ((dut.fifo_count != FIFO_DEPTH-1 || !rvalid || !rready) &&
                   watchdog < 100000) begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.fifo_count != FIFO_DEPTH-1 || !rvalid || !rready)
                $fatal(1, "failed to align R plus FIFO pop at D-1 occupancy=%0d", dut.fifo_count);
            start = 1'b1;
            opcode = 2'd0;
            size_bits = 32'd32;
            byte_address = 32'h0001c000;
            #1;
            if (!done || !dut.r_push || !dut.fifo_load_pop ||
                data_out !== (32'h0001c000 >> 2))
                $fatal(1, "R+FIFO pop did not occur at D-1 occupancy");
            @(posedge clk);
            @(negedge clk);
            start = 1'b0;
            consume_stream(32'h0001c004, target_n-1);
            wait_drained();
            check_descriptors(32'h0001c000, target_n, 0);
            $display("PASS simultaneous R+FIFO pop at D-1=%0d occupancy", FIFO_DEPTH-1);
            $finish;
        end

        if ($test$plusargs("ERROR")) begin
            stall_ar = 1'b0;
            configure(32'h00007000, 1);
            wait (fault);
            if (done)
                $fatal(1, "done asserted after response fault");
            $display("PASS B=%0d error fault surfaced", B_MAX);
            $finish;
        end
        if ($test$plusargs("DRAIN_CONFIG_RACE")) begin
            integer completions_before;
            stall_ar = 1'b0;

            // Arrive after the last pop, while drain_complete is true but
            // before the old region's active bit is cleared.
            configure(32'h0000b000, 1);
            completions_before = configure_completion_count;
            wait (dut.returned == 1);
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd0;
            size_bits = 32'd32;
            byte_address = 32'h0000b000;
            #1;
            if (!done)
                $fatal(1, "final old-region load was not immediately available");
            @(posedge clk); // final load pop makes drain_complete true
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = 32'h0000c000;
            count = 2;
            #1;
            if (!dut.drain_complete || !done || !dut.configure_direct_capture)
                $fatal(1, "configure was not accepted on the already-drained edge");
            @(posedge clk);
            #1;
            if (!dut.region_active || dut.region_base != 32'h0000c000 ||
                dut.cfg_pending || configure_completion_count != completions_before + 1)
                $fatal(1, "direct drain-edge configure failed or completed more than once");
            @(negedge clk);
            start = 1'b0;
            wait (dut.returned == 2);
            consume_stream(32'h0000c000, 2);
            wait_drained();

            // Also collide configure capture with the final pending-load pop.
            stall_ar = 1'b1;
            configure(32'h0000d000, 1);
            completions_before = configure_completion_count;
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd0;
            size_bits = 32'd32;
            byte_address = 32'h0000d000;
            #1;
            if (done)
                $fatal(1, "same-cycle final-load setup unexpectedly buffered data");
            @(posedge clk);
            @(negedge clk);
            start = 1'b0;
            stall_ar = 1'b0;
            while (!done) begin
                @(negedge clk);
                #1;
            end
            start = 1'b1;
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = 32'h0000e000;
            count = 2;
            #1;
            if (dut.configure_complete)
                $fatal(1, "reconfigure completed on the old pending-load result");
            @(posedge clk); // captures configure and consumes old pending load
            #1;
            if (!dut.cfg_pending || dut.consumed != 1 || !dut.configure_complete)
                $fatal(1, "same-cycle configure/load-pop state was not retained");
            @(negedge clk);
            start = 1'b0;
            #1;
            if (!dut.configure_complete || !done)
                $fatal(1, "deferred configure did not complete after the final pop");
            @(posedge clk);
            #1;
            if (!dut.region_active || dut.region_base != 32'h0000e000 ||
                dut.cfg_pending || configure_completion_count != completions_before + 1)
                $fatal(1, "same-cycle reconfigure failed or completion count was not exact");
            wait (dut.returned == 2);
            consume_stream(32'h0000e000, 2);
            wait_drained();
            $display("PASS B=%0d drain-edge and same-cycle-pop configure", B_MAX);
            $finish;
        end
        if ($test$plusargs("ADJACENT_CFG")) begin
            integer completions_before;
            stall_ar = 1'b0;
            completions_before = configure_completion_count;

            // Two configure commands back to back with start never lowered and the
            // opcode never changed. The first must complete; the second, carrying
            // different operands, must also be captured. A repeat with identical
            // operands must stay one request.
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = 32'h00021000;
            // An empty region is drained as soon as it is active, so the replacement
            // can be captured directly instead of waiting for a region that nothing
            // consumes. The point under test is the guard, not the drain path.
            count = 0;
            #1;
            if (!done)
                $fatal(1, "adjacent-cfg first configure did not complete at capture");
            @(posedge clk);

            // Change only the operands: start and opcode stay exactly as they are.
            // The engine releases its guard on the edge that sees the new operands and
            // captures on the following one, so wait on clock edges rather than assuming
            // a single cycle.
            @(negedge clk);
            byte_address = 32'h00022000;
            count = 0;
            begin
                integer guard = 0;
                while (configure_completion_count < completions_before + 2 && guard < 12) begin
                    @(posedge clk);
                    guard = guard + 1;
                end
                if (configure_completion_count < completions_before + 2)
                    $fatal(1, "adjacent-cfg second configure never completed");
            end
            if (configure_completion_count != completions_before + 2)
                $fatal(1, "adjacent-cfg completions = %0d, expected %0d",
                       configure_completion_count, completions_before + 2);

            // Same operands again with start still high: still one request, no new capture.
            @(posedge clk);
            repeat (2) begin
                @(negedge clk);
                #1;
                if (done)
                    $fatal(1, "adjacent-cfg repeated identical configure completed again");
            end
            if (configure_completion_count != completions_before + 2)
                $fatal(1, "adjacent-cfg identical repeat produced an extra completion");
            @(negedge clk);
            start = 1'b0;
            $display("ADJACENT_CFG: back-to-back configures with held start accepted");
        end

        if ($test$plusargs("ADJACENT_CFG_TRUNC")) begin
            // The guard re-arm must compare every operand at full width. Changing only
            // the count from 0 to a value whose top bit is set is an invalid request: if
            // the comparison truncates the count, the request looks identical to the one
            // already guarded, is never captured, and never reaches validation - so it
            // produces neither done nor fault.
            integer guard;
            stall_ar = 1'b0;
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = 32'h00023000;
            count = 0;
            #1;
            if (!done)
                $fatal(1, "adjacent-cfg-trunc baseline configure did not complete");
            @(posedge clk);
            @(negedge clk);
            count = 32'h80000000;
            guard = 0;
            while (!fault && guard < 12) begin
                @(posedge clk);
                guard = guard + 1;
            end
            #1;
            if (!fault)
                $fatal(1, "adjacent-cfg-trunc: oversized count was ignored instead of rejected");
            $display("ADJACENT_CFG_TRUNC: oversized count reached validation and faulted");
            $finish;
        end

        if ($test$plusargs("ADJACENT_CFG_SIZE")) begin
            // Same hole through the other operand: changing only the element size from 32
            // to 16 must be re-armed and rejected. An operand set that ignores size_bits
            // leaves the request unvalidated, with neither done nor fault.
            integer guard;
            stall_ar = 1'b0;
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = 32'h00024000;
            count = 0;
            #1;
            if (!done)
                $fatal(1, "adjacent-cfg-size baseline configure did not complete");
            @(posedge clk);
            @(negedge clk);
            size_bits = 32'd16;
            guard = 0;
            while (!fault && guard < 12) begin
                @(posedge clk);
                guard = guard + 1;
            end
            #1;
            if (!fault)
                $fatal(1, "adjacent-cfg-size: changed element size was ignored instead of rejected");
            $display("ADJACENT_CFG_SIZE: changed element size reached validation and faulted");
            $finish;
        end

        if ($test$plusargs("CONTINUOUS_CFG")) begin
            integer completions_before;
            integer descriptors_before;
            stall_ar = 1'b0;
            completions_before = configure_completion_count;
            descriptors_before = ar_count;

            // Keep start high while holding configure; it must capture once.
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = 32'h00011000;
            count = 1;
            #1;
            if (!done)
                $fatal(1, "first continuous configure did not complete at capture");
            @(posedge clk);
            repeat (2) begin
                @(negedge clk);
                if (done || configure_completion_count != completions_before + 1)
                    $fatal(1, "held configure completed more than once");
                @(posedge clk);
            end

            // Transition to a scalar read without lowering start.
            @(negedge clk);
            opcode = 2'd0;
            size_bits = 32'd32;
            byte_address = 32'h00011000;
            #1;
            if (!done) begin
                @(posedge clk); // accept the pending scalar read
            end
            while (!done) begin
                @(negedge clk);
                #1;
            end
            if (data_out !== (32'h00011000 >> 2))
                $fatal(1, "continuous-sequence first load returned wrong data");
            @(posedge clk); // last pop

            // The opcode left configure, so the next held-start configure is new.
            @(negedge clk);
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = 32'h00012000;
            count = 2;
            #1;
            if (!dut.drain_complete || !dut.configure_direct_capture || !done)
                $fatal(1, "configure after continuous-start loads was not accepted");
            @(posedge clk);
            #1;
            if (!dut.region_active || dut.region_base != 32'h00012000 ||
                dut.cfg_pending || configure_completion_count != completions_before + 2 ||
                ar_count != descriptors_before + 1)
                $fatal(1, "continuous configure sequence duplicated or lost a capture");
            repeat (2) begin
                @(negedge clk);
                if (done || configure_completion_count != completions_before + 2)
                    $fatal(1, "second held configure completed more than once");
                @(posedge clk);
            end
            consume_stream(32'h00012000, 2);
            wait_drained();
            $display("PASS B=%0d continuous-start configure/load/configure", B_MAX);
            $finish;
        end

        run_case(32'h00001000, 0, 1'b0);
        run_case(32'h00002000, 1, 1'b1);
        run_case(32'h00002400, B_MAX-1, 1'b1);
        run_case(32'h00002800, B_MAX, 1'b0);
        run_case(32'h00003000, B_MAX+1, 1'b1);
        run_case(32'h00004000, 2*B_MAX+6, 1'b1);
        run_case(32'h00005000, 38, 1'b0);
        run_case(32'h00000ffc, 5, 1'b0); // 4 KiB split after one beat

        // A long fully-buffered region exercises immediate back-to-back starts.
        before_ar = ar_count;
        stall_ar = 1'b0;
        configure(32'h00006000, 38);
        wait (dut.returned == 38);
        consume_immediate_stream(32'h00006000, 38);
        wait_drained();
        check_descriptors(32'h00006000, 38, before_ar);

        // A second configure is retained until the current region drains.
        before_ar = ar_count;
        configure(32'h00007000, 4);
        wait (dut.returned == 4);
        @(negedge clk);
        start = 1'b1;
        opcode = 2'd2;
        byte_address = 32'h00008000;
        count = 2;
        #1;
        if (done)
            $fatal(1, "busy configure completed before the old region drained");
        @(posedge clk);
        @(negedge clk);
        start = 1'b0;
        consume_stream(32'h00007000, 4);
        wait (deferred_cfg_done != 0);
        if (deferred_cfg_done == 0 || !dut.region_active || dut.region_base != 32'h00008000)
            $fatal(1, "pending configure was not captured after drain: done_count=%0d active=%b base=%h pending=%b count=%0d consumed=%0d returned=%0d",
                   deferred_cfg_done, dut.region_active, dut.region_base, dut.cfg_pending,
                   dut.region_count, dut.consumed, dut.returned);
        check_descriptors(32'h00007000, 4, before_ar);
        before_ar = ar_count;
        wait (dut.returned == 2);
        consume_stream(32'h00008000, 2);
        wait_drained();
        check_descriptors(32'h00008000, 2, before_ar);

        $display("PASS B=%0d AR=%0d R=%0d", B_MAX, ar_count, r_beat_count);
        $finish;
    end

    initial begin
        #2000000;
        $fatal(1, "global test watchdog expired O=%0d D=%0d issued=%0d returned=%0d consumed=%0d desc=%0d fifo=%0d AR=%0d R=%0d fault=%b",
               MAX_OUTSTANDING, FIFO_DEPTH, dut.issued, dut.returned, dut.consumed,
               dut.descriptor_count, dut.fifo_count, ar_count, r_beat_count, fault);
    end
endmodule
