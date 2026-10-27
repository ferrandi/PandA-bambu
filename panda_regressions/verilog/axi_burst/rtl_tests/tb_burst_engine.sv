module tb_burst_engine #(
    parameter integer B_MAX = 16,
    parameter integer MAX_OUTSTANDING = 1,
    parameter integer FIFO_DEPTH = 256,
    parameter integer ASYNC_RESET = 0,
    parameter integer RESET_ACTIVE_LOW = 1
);
    logic clk = 1'b0;
    always #5 clk <= ~clk;

    logic reset_n = 1'b0;
    // reset_n is the logical "out of reset" level of the bench; the value driven into the
    // component depends on the polarity the extractor substituted for 1RESET_VALUE.
    localparam logic RESET_ASSERTED = RESET_ACTIVE_LOW[0] ? 1'b0 : 1'b1;
    wire dut_reset = reset_n ? ~RESET_ASSERTED : RESET_ASSERTED;
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
    wire arready;
    logic [31:0] rdata;
    logic [1:0] rresp;
    logic [0:0] rid;
    logic rlast;
    logic rvalid_reg = 1'b0;
    logic force_rvalid = 1'b0;
    wire rvalid = rvalid_reg || force_rvalid;
    logic rready;

    wire slave_active;
    logic stall_ar = 1'b1;
    logic inject_error = 1'b0;
    integer inject_error_at = -1;
    logic inject_wrong_rid = 1'b0;
    logic inject_early_rlast = 1'b0;
    logic suppress_rlast = 1'b0;
    logic force_rlast = 1'b0;
    logic forced_rlast = 1'b0;
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
    string selected_case;
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
    integer bus_transport_beats = 0;
    integer bus_returned_beats = 0;
    integer bus_fault_seen = 0;
    integer bus_desc_count = 0;
    integer bus_desc_rd_ptr = 0;
    integer bus_desc_wr_ptr = 0;
    integer bus_desc_remaining [0:15];
    integer bus_region_count = 0;
    integer bus_desc_sum;
    integer bus_reserved_beats;
    integer bus_r_is_good;
    integer bus_r_structural;
    integer bus_r_finishes_desc;
    wire bus_r_finishes_desc_now = rvalid && rready && bus_desc_count != 0 &&
        rid == 0 && rlast &&
        bus_desc_remaining[bus_desc_rd_ptr] == 1;
    integer aqc_hits = 0;
    integer stalled_fault_ar_hits = 0;
    integer multi_reset_hits = 0;
    integer clock_cycles = 0;

    MinimalAXI4MasterPipelined #(.B_MAX(B_MAX),
        .MAX_OUTSTANDING(MAX_OUTSTANDING), .FIFO_DEPTH(FIFO_DEPTH)) dut (
        .clock(clk), .reset(dut_reset), .start(start), .in1(opcode),
        .in2(size_bits), .in3(payload), .in4(byte_address),
        .in5(count), .done(done), .out1(data_out), .fault(fault), .peer_fault(1'b0),
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
    assign rlast = force_rlast ? forced_rlast :
                   (suppress_rlast ? 1'b0 :
                    (inject_early_rlast ? (slave_left == 2) : (slave_left == 1)));
    assign rresp = (inject_error || r_beat_count == inject_error_at) ? 2'b10 : 2'b00;
    assign arready = !stall_ar && (slave_q_count < 16 ||
                   (rvalid && rready && slave_left == 1));

    always @(posedge clk)
    begin
        integer descriptor_sum;
        integer accepted_ar_beats;
        if (reset_n && dut.configure_complete)
            configure_completion_count <= configure_completion_count + 1;
        if (reset_n && dut.activate_pending)
            deferred_cfg_done <= deferred_cfg_done + 1;
        if (!reset_n)
        begin
            rvalid_reg <= 1'b0;
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
            bus_transport_beats = 0;
            bus_returned_beats = 0;
            bus_fault_seen = 0;
            bus_desc_count = 0;
            bus_desc_rd_ptr = 0;
            bus_desc_wr_ptr = 0;
            bus_region_count = 0;
            aqc_hits = 0;
            stalled_fault_ar_hits = 0;
            multi_reset_hits = 0;
            prev_stalled <= 1'b0;
        end
        else
        begin
            if (dut.burst_outstanding !== (dut.descriptor_count_reg != 0))
                $fatal(1, "Phase-A burst_outstanding alias disagrees with descriptor_count: O=%0d count=%0d alias=%b",
                       MAX_OUTSTANDING, dut.descriptor_count_reg, dut.burst_outstanding);
            // An independent bus-handshake model checks accounting; it does
            // not use DUT counters to derive accepted beats or descriptor sums.
            if (dut.configure_ack_reg)
            begin
                bus_issued_beats = 0;
                bus_transport_beats = 0;
                bus_returned_beats = 0;
                bus_fault_seen = 0;
                bus_desc_count = 0;
                bus_desc_rd_ptr = 0;
                bus_desc_wr_ptr = 0;
                bus_region_count = dut.region_count_reg;
            end

            bus_reserved_beats = arvalid ? (arlen + 1) : 0;
            bus_desc_sum = 0;
            for (integer bi = 0; bi < MAX_OUTSTANDING; bi = bi + 1)
                if (bi < bus_desc_count)
                    bus_desc_sum = bus_desc_sum + bus_desc_remaining[(bus_desc_rd_ptr + bi) % MAX_OUTSTANDING];
            bus_r_structural = rvalid && rready && bus_desc_count != 0 &&
                rid == 0 &&
                ((bus_desc_remaining[bus_desc_rd_ptr] == 1 && rlast) ||
                 (bus_desc_remaining[bus_desc_rd_ptr] > 1 && !rlast));
            bus_r_is_good = bus_r_structural && rresp == 0 && !bus_fault_seen;
            bus_r_finishes_desc = bus_r_structural &&
                bus_desc_remaining[bus_desc_rd_ptr] == 1;

            // Check accounting against pre-edge state. Skip the edge on which
            // reset/reconfiguration atomically replaces the region counters.
            if (dut.region_active_reg && !fault && !dut.peer_fault_active &&
                !dut.configure_ack_reg)
                begin
                if (dut.consumed_reg > dut.returned_reg || dut.returned_reg > dut.issued_reg ||
                    dut.issued_reg > dut.region_count_reg)
                    $fatal(1, "burst counter ordering violated: consumed=%0d returned=%0d issued=%0d count=%0d",
                           dut.consumed_reg, dut.returned_reg, dut.issued_reg, dut.region_count_reg);
                if (dut.fifo_count_reg != (dut.returned_reg - dut.consumed_reg) ||
                    dut.fifo_count_reg > FIFO_DEPTH)
                    $fatal(1, "FIFO occupancy/counter mismatch: fifo=%0d returned=%0d consumed=%0d",
                           dut.fifo_count_reg, dut.returned_reg, dut.consumed_reg);
                monitor_reserved_beats = dut.m_axi_arvalid_reg ?
                    ({24'b0, dut.m_axi_arlen_reg} + 1) : 0;
                if ((dut.issued_reg - dut.consumed_reg + monitor_reserved_beats) > FIFO_DEPTH)
                    $fatal(1, "burst credit limit exceeded: issued=%0d consumed=%0d reserved=%0d",
                           dut.issued_reg, dut.consumed_reg, monitor_reserved_beats);
                if (dut.issued_reg + monitor_reserved_beats > dut.region_count_reg)
                    $fatal(1, "DUT issued+reserved exceeds configured region: issued=%0d reserved=%0d count=%0d",
                           dut.issued_reg, monitor_reserved_beats, dut.region_count_reg);
                descriptor_sum = 0;
                for (integer di = 0; di < MAX_OUTSTANDING; di = di + 1)
                    if (di < dut.descriptor_count_reg)
                        descriptor_sum = descriptor_sum + dut.descriptor_remaining_reg[(dut.descriptor_rd_ptr_reg + di) % MAX_OUTSTANDING];
                if (dut.fifo_count_reg + descriptor_sum + dut.fifo_credit != FIFO_DEPTH)
                    $fatal(1, "READY_CREDIT_PROOF credit conservation failed: fifo=%0d outstanding=%0d credit=%0d depth=%0d",
                           dut.fifo_count_reg, descriptor_sum, dut.fifo_credit, FIFO_DEPTH);
                if (dut.m_axi_arvalid_reg &&
                    ({24'b0, dut.m_axi_arlen_reg} + 1) > dut.fifo_credit)
                    $fatal(1, "READY_CREDIT_PROOF exposed AR exceeds unspent credit: beats=%0d credit=%0d",
                           {24'b0, dut.m_axi_arlen_reg} + 1, dut.fifo_credit);
                if ((dut.descriptor_count_reg != 0 && dut.fifo_count_reg == FIFO_DEPTH) ||
                    (dut.fifo_count_reg == FIFO_DEPTH &&
                     (dut.m_axi_arvalid_reg || rready)))
                    $fatal(1, "READY_CREDIT_PROOF full FIFO has live transport: descriptors=%0d ARVALID=%b RREADY=%b",
                           dut.descriptor_count_reg, dut.m_axi_arvalid_reg, rready);
                if (dut.descriptor_count_reg > MAX_OUTSTANDING ||
                    dut.descriptor_count_reg + dut.m_axi_arvalid_reg > MAX_OUTSTANDING ||
                    descriptor_sum != (dut.issued_reg - dut.returned_reg))
                    $fatal(1, "descriptor invariant violated count=%0d sum=%0d outstanding=%0d issued-returned=%0d",
                           dut.descriptor_count_reg, descriptor_sum,
                           (dut.descriptor_count_reg + dut.m_axi_arvalid_reg), dut.issued_reg-dut.returned_reg);
                if (bus_issued_beats != dut.issued_reg || bus_returned_beats != dut.returned_reg ||
                    bus_desc_count != dut.descriptor_count_reg ||
                    bus_desc_sum != (bus_issued_beats - bus_transport_beats))
                    $fatal(1, "independent bus scoreboard mismatch: ARbeats=%0d Rbeats=%0d desc=%0d sum=%0d; DUT issued=%0d returned=%0d desc=%0d",
                           bus_issued_beats, bus_returned_beats, bus_desc_count,
                           bus_desc_sum, dut.issued_reg, dut.returned_reg, dut.descriptor_count_reg);
                if (bus_issued_beats + bus_reserved_beats > bus_region_count)
                    $fatal(1, "bus-issued+reserved exceeds configured region: issued=%0d reserved=%0d region=%0d",
                           bus_issued_beats, bus_reserved_beats, bus_region_count);
                if (dut.descriptor_count_reg > peak_descriptors)
                    peak_descriptors <= dut.descriptor_count_reg;
            end
            if (dut.r_good && dut.fifo_wr_ptr_reg == FIFO_DEPTH-1)
                fifo_write_wraps <= fifo_write_wraps + 1;
            if (dut.fifo_load_pop && dut.fifo_rd_ptr_reg == FIFO_DEPTH-1)
                fifo_read_wraps <= fifo_read_wraps + 1;
            if (dut.r_push && dut.fifo_load_pop)
                simultaneous_push_pop <= simultaneous_push_pop + 1;
            if (arvalid && arready && dut.descriptor_pop &&
                bus_r_finishes_desc_now && dut.fifo_load_pop)
                aqc_hits = aqc_hits + 1;
            if (arvalid && arready && dut.descriptor_wr_ptr_reg == MAX_OUTSTANDING-1)
                descriptor_write_wraps <= descriptor_write_wraps + 1;
            if (dut.descriptor_pop && dut.descriptor_rd_ptr_reg == MAX_OUTSTANDING-1)
                descriptor_read_wraps <= descriptor_read_wraps + 1;
            if (prev_stalled && (araddr !== stalled_addr || arlen !== stalled_len || !arvalid))
                $fatal(1, "AR descriptor changed while stalled");
            prev_stalled <= arvalid && !arready;
            if (arvalid && !arready)
            begin
                stalled_addr <= araddr;
                stalled_len <= arlen;
            end

            if (arvalid && arready)
            begin
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

            if (rvalid && rready)
            begin
                rvalid_reg <= 1'b0;
                r_beat_count <= r_beat_count + 1;
                if (bus_r_is_good)
                begin
                    bus_returned_beats = bus_returned_beats + 1;
                end
                if (bus_r_structural)
                begin
                    bus_transport_beats = bus_transport_beats + 1;
                    if (bus_r_finishes_desc)
                    begin
                        if (bus_desc_rd_ptr == MAX_OUTSTANDING-1) bus_desc_rd_ptr = 0;
                        else bus_desc_rd_ptr = bus_desc_rd_ptr + 1;
                        bus_desc_count = bus_desc_count - 1;
                    end
                    else
                    begin
                        bus_desc_remaining[bus_desc_rd_ptr] =
                            bus_desc_remaining[bus_desc_rd_ptr] - 1;
                    end
                end
                if (!bus_r_structural || rresp != 0)
                    bus_fault_seen = 1;
                if (slave_left == 1)
                begin
                    if (slave_q_rd == 15) slave_q_rd <= 0;
                    else slave_q_rd <= slave_q_rd + 1;
                end
                else
                begin
                    slave_q_addr[slave_q_rd] <= slave_addr + 4;
                    slave_q_left[slave_q_rd] <= slave_left - 1;
                    delay_left <= (r_beat_count % 3) + 1;
                end
            end
            else if (slave_active && !rvalid && ar_count >= hold_responses_until)
            begin
                if (delay_left == 0)
                    rvalid_reg <= 1'b1;
                else
                    delay_left <= delay_left - 1;
            end
            case ({(arvalid && arready), (rvalid && rready && slave_left == 1)})
                2'b10: slave_q_count <= slave_q_count + 1;
                2'b01: slave_q_count <= slave_q_count - 1;
                default: slave_q_count <= slave_q_count;
            endcase
            if (arvalid && arready)
            begin
                // The count update follows the R retirement above, making a
                // same-edge descriptor completion/acceptance atomic.
                bus_desc_count = bus_desc_count + 1;
            end
        end
    end

    always @(posedge clk)
        clock_cycles <= clock_cycles + 1;

    task automatic configure(input [31:0] base, input [31:0] n);
        integer cycles;
        begin
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = base;
            count = n;
            @(posedge clk); // E0: capture the request and its operands.
            @(negedge clk);
            start = 1'b0;
            opcode = 2'b0;
            size_bits = 32'b0;
            payload = 32'b0;
            byte_address = 32'b0;
            count = 32'b0;
            cycles = 0;
            while (!done && !fault && cycles < 16)
            begin
                @(posedge clk);
                #1;
                cycles = cycles + 1;
            end
            if (fault)
                $fatal(1, "valid configure faulted after E0 capture base=%h count=%h", base, n);
            if (!done)
                $fatal(1, "configure acknowledgement timeout after E0 capture base=%h count=%h", base, n);
            @(posedge clk); // HLS samples the registered acknowledgement at E2.
            #1;
        end
    endtask

    task automatic configure_invalid(input [31:0] base, input [31:0] n,
                                     input [31:0] invalid_size);
        integer cycles;
        begin
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd2;
            size_bits = invalid_size;
            byte_address = base;
            count = n;
            @(posedge clk); // E0: capture both payload and validation result.
            @(negedge clk);
            start = 1'b0;
            opcode = 2'b0;
            size_bits = 32'b0;
            payload = 32'b0;
            byte_address = 32'b0;
            count = 32'b0;
            cycles = 0;
            while (!fault && !done && cycles < 16)
            begin
                @(posedge clk);
                #1;
                cycles = cycles + 1;
            end
            if (done)
                $fatal(1, "invalid configure acknowledged after E0 capture base=%h count=%h", base, n);
            if (!fault)
                $fatal(1, "invalid configure fault timeout after E0 capture base=%h count=%h", base, n);
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
            if (fault || (n != 0 && !dut.region_active_reg) ||
                (n == 0 && dut.region_active_reg) || dut.region_count_reg != n)
                $fatal(1, "valid configure range was rejected/mis-captured: base=%h count=%h fault=%b active=%b captured=%h",
                       base, n, fault, dut.region_active_reg, dut.region_count_reg);
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
        reg [31:0] active_base_before;
        reg [30:0] active_count_before;
        integer watchdog;
        begin
            active_base_before = dut.region_base_reg;
            active_count_before = dut.region_count_reg;
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = base;
            count = n;
            @(posedge clk); // E0: latch the queued request before changing live inputs.
            #1;
            if (done || fault || dut.cfg_pending_reg)
                $fatal(1, "busy configure acknowledged or queued at E0");
            start = 1'b0;
            opcode = 2'b0;
            size_bits = 32'b0;
            payload = 32'b0;
            byte_address = 32'b0;
            count = 32'b0;
            watchdog = 0;
            while (!dut.cfg_pending_reg && watchdog < 16)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (!dut.cfg_pending_reg || dut.pending_base_reg != base || dut.pending_count_reg != n ||
                !dut.region_active_reg || dut.region_base_reg != active_base_before ||
                dut.region_count_reg != active_count_before)
                $fatal(1, "busy configure did not queue its E0-captured descriptor");
            #1;
            if (done || !dut.region_active_reg || dut.region_base_reg != active_base_before ||
                dut.region_count_reg != active_count_before)
                $fatal(1, "busy configure acknowledged early or overwrote active region");
        end
    endtask

    task automatic consume_pending(input [31:0] expected);
        integer watchdog;
        begin
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd0;
            size_bits = 32'd32;
            byte_address = dut.next_expected_address_reg;
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
            while (!done && watchdog < 20000)
            begin
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
            while (idx < n && watchdog < 100000)
            begin
                @(negedge clk);
                if (!waiting_pending)
                    start = 1'b1;
                else
                    start = 1'b0;
                opcode = 2'd0;
                size_bits = 32'd32;
                byte_address = base + (idx * 4);
                #1;
                if (done)
                begin
                    if (data_out !== ((base >> 2) + idx))
                        $fatal(1, "load %0d returned %0d expected %0d", idx,
                               data_out, (base >> 2) + idx);
                    @(posedge clk);
                    idx = idx + 1;
                    waiting_pending = 1'b0;
                end
                else if (!waiting_pending)
                begin
                    @(posedge clk); // capture exactly one pending load
                    waiting_pending = 1'b1;
                    @(negedge clk);
                    start = 1'b0;
                    #1;
                    if (done)
                    begin
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
                       idx, n, dut.issued_reg, dut.returned_reg, dut.consumed_reg, dut.fifo_count_reg,
                       dut.load_pending_reg, dut.descriptor_count_reg, fault);
        end
    endtask

    task automatic wait_drained;
        integer watchdog;
        begin
            watchdog = 0;
            while (dut.region_active_reg && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.region_active_reg)
                $fatal(1, "region did not drain: issued=%0d returned=%0d consumed=%0d count=%0d fifo=%0d arvalid=%b outstanding=%b pending=%b",
                       dut.issued_reg, dut.returned_reg, dut.consumed_reg, dut.region_count_reg,
                       dut.fifo_count_reg, arvalid, dut.descriptor_count_reg, dut.load_pending_reg);
            if (dut.issued_reg != dut.region_count_reg || dut.returned_reg != dut.region_count_reg ||
                dut.consumed_reg != dut.region_count_reg || dut.fifo_count_reg != 0)
                $fatal(1, "region counters failed drain invariant");
        end
    endtask

    task automatic consume_immediate_stream(input [31:0] base, input integer n);
        integer idx;
        begin
            for (idx = 0; idx < n; idx = idx + 1)
            begin
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
            while (rem > 0)
            begin
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
            if (n == 0)
            begin
                repeat (8) @(posedge clk);
                if (ar_count != before_ar)
                    $fatal(1, "zero-count configure issued AR");
            end
            else
            begin
                if (do_pending)
                begin
                    consume_pending(base >> 2);
                    stall_ar = 1'b0;
                    if (n > 1)
                        consume_stream(base+4, n-1);
                end
                else
                begin
                    stall_ar = 1'b0;
                    wait (dut.returned_reg == n);
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
            if (full_fifo)
            begin
                wait (dut.returned_reg == 256);
                if (dut.fifo_count_reg != FIFO_DEPTH || dut.consumed_reg != 0)
                    $fatal(1, "invalid-load full-FIFO setup failed");
            end
            else
            begin
                wait (arvalid);
                if (dut.fifo_count_reg != 0 || dut.consumed_reg != 0)
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
            if (done || dut.consumed_reg != 0 ||
                dut.fifo_count_reg != (full_fifo ? 256 : 0))
                $fatal(1, "invalid scalar load consumed data (wrong_address=%b full=%b)",
                       wrong_address, full_fifo);
            $display("PASS B=%0d invalid load rejected wrong_address=%b full=%b",
                     B_MAX, wrong_address, full_fifo);
            $finish;
        end
    endtask

    initial
    begin
        if (!$value$plusargs("CASE=%s", selected_case))
            $fatal(1, "missing exact CASE selector");
        case (selected_case)
            "ERROR", "BADALIGN", "OVERFLOW", "BADSIZE_CONFIG", "STORE",
            "RANGE_ZERO", "RANGE_LIMIT_BASE0", "RANGE_LIMIT_BASE4",
            "RANGE_OVER_BASE4", "RANGE_OVER_PLUS1", "RANGE_OVER_HALF",
            "RANGE_OVER_MAX", "RANGE_TOP_VALID", "RANGE_TOP_OVER", "RANGE_ORACLE",
            "BADADDR_EMPTY", "BADSIZE_EMPTY", "BADADDR_FULL", "BADSIZE_FULL",
            "DRAIN_CONFIG_RACE", "CONTINUOUS_CFG", "ADJACENT_CFG",
            "ADJACENT_CFG_TRUNC", "ADJACENT_CFG_SIZE", "BADRID", "EARLY_RLAST",
            "CONFIG_PIPELINE_VALID", "CONFIG_PIPELINE_INVALID", "CONFIG_PIPELINE_B1",
            "RRESP_FIRST", "RRESP_INTERMEDIATE", "RRESP_FINAL", "MISSING_RLAST",
            "RESET_TRAFFIC", "FIFO_WRAP", "FIFO_BACKPRESSURE", "RESET_MULTI",
            "FAULT_STALLED_AR", "AQC", "PENDING_SECOND", "BOUNDARY_PENDING",
            "RRESP_STALLED_AR", "RRESP_RESERVATION_RACE", "RREADY_SIDEBAND",
            "DEPTH4096", "MULTI", "ASYNC_RESET",
            "CREDIT_LIMIT", "CREDIT_POP", "READY_CREDIT_PROOF", "SMOKE":
            begin
            end
            default: $fatal(1, "unknown exact CASE selector: %s", selected_case);
        endcase
        if (!$test$plusargs(selected_case))
            $fatal(1, "selected CASE has no matching scenario plusarg: %s", selected_case);
        if ($test$plusargs("ERROR")) inject_error = 1'b1;
        repeat (4) @(posedge clk);
        reset_n = 1'b1;
        repeat (2) @(posedge clk);

        if ($test$plusargs("CONFIG_PIPELINE_VALID"))
        begin
            integer completion_cycle;
            stall_ar = 1'b0;
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = 32'h00051000;
            count = 3;
            @(posedge clk); // E0 captures this descriptor.
            #1;
            if (done || fault || arvalid || ar_count != 0)
                $fatal(1, "CONFIG_PIPELINE_E0_VISIBLE done=%b fault=%b arvalid=%b AR=%0d",
                       done, fault, arvalid, ar_count);
            @(negedge clk);
            start = 1'b0;
            opcode = 2'd3;
            size_bits = 32'd16;
            payload = 32'hdead_beef;
            byte_address = 32'h00051001;
            count = 32'h8000_0000;
            @(posedge clk); // E1 consumes the saved valid descriptor.
            #1;
            if (!done || fault || !dut.region_active_reg ||
                dut.region_base_reg != 32'h00051000 || dut.region_count_reg != 3 || ar_count != 0)
                $fatal(1, "CONFIG_PIPELINE_E1_CAPTURE_MISMATCH done=%b fault=%b active=%b base=%h count=%0d AR=%0d",
                       done, fault, dut.region_active_reg, dut.region_base_reg, dut.region_count_reg, ar_count);
            completion_cycle = clock_cycles;
            @(posedge clk); // Retire configure ACK before presenting the first LOAD.
            #1;
            wait (dut.returned_reg == 3);
            consume_stream(32'h00051000, 3);
            wait_drained();
            if (fault || ar_count != 1 || r_beat_count != 3 || dut.consumed_reg != 3)
                $fatal(1, "CONFIG_PIPELINE_VALID_DATA_OR_TRAFFIC_MISMATCH fault=%b AR=%0d R=%0d consumed=%0d",
                       fault, ar_count, r_beat_count, dut.consumed_reg);
            $display("CONFIG_PIPELINE_VALID_PASS E1_cycle=%0d AR=%0d R=%0d",
                     completion_cycle, ar_count, r_beat_count);
            $finish;
        end

        if ($test$plusargs("CONFIG_PIPELINE_INVALID"))
        begin
            stall_ar = 1'b0;
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = 32'hffff_fffc;
            count = 2;
            @(posedge clk); // E0 captures both the invalid payload and validity.
            #1;
            if (done || fault || arvalid || ar_count != 0)
                $fatal(1, "CONFIG_PIPELINE_INVALID_E0_VISIBLE done=%b fault=%b arvalid=%b AR=%0d",
                       done, fault, arvalid, ar_count);
            @(negedge clk);
            start = 1'b0;
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = 32'h00052000;
            count = 1;
            @(posedge clk); // E1 must reject the E0-captured invalid request.
            #1;
            if (!fault || done || dut.region_active_reg || ar_count != 0)
                $fatal(1, "CONFIG_PIPELINE_INVALID_E1_NOT_REJECTED fault=%b done=%b active=%b AR=%0d",
                       fault, done, dut.region_active_reg, ar_count);
            $display("CONFIG_PIPELINE_INVALID_PASS E1 fault observed without AXI traffic");
            $finish;
        end

        if ($test$plusargs("CONFIG_PIPELINE_B1"))
        begin
            integer first_cycle;
            integer last_cycle;
            if (B_MAX != 1 || FIFO_DEPTH != 1)
                $fatal(1, "CONFIG_PIPELINE_B1_REQUIRES_B1_FIFO1");
            stall_ar = 1'b0;
            configure(32'h00053000, 8);
            first_cycle = clock_cycles;
            consume_stream(32'h00053000, 8);
            last_cycle = clock_cycles;
            wait_drained();
            if (last_cycle-first_cycle > 64)
                $fatal(1, "CONFIG_PIPELINE_B1_THROUGHPUT_REGRESSION cycles=%0d baseline=64 beats=8",
                       last_cycle-first_cycle);
            if (fault || ar_count != 8 || r_beat_count != 8 || dut.consumed_reg != 8)
                $fatal(1, "CONFIG_PIPELINE_B1_READ_TRAFFIC_MISMATCH fault=%b AR=%0d R=%0d consumed=%0d",
                       fault, ar_count, r_beat_count, dut.consumed_reg);
            $display("CONFIG_PIPELINE_B1_PASS read_cycles=%0d baseline_cycles=64 beats=8 AR=%0d R=%0d",
                     last_cycle-first_cycle, ar_count, r_beat_count);
            $finish;
        end

        if ($test$plusargs("RANGE_ORACLE"))
        begin : range_oracle_case
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
            for (sample_index = 0; sample_index < 32; sample_index = sample_index + 1)
            begin
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
                if (range_valid_64(random_base, random_count, 32'd4))
                begin
                    valid_samples = valid_samples + 1;
                    configure_valid_range(random_base, random_count);
                end
                else
                begin
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
        if ($test$plusargs("RANGE_ZERO"))
        begin
            configure_valid_range(32'h00000000, 32'h00000000);
            if (ar_count != 0 || dut.region_count_reg != 0)
                $fatal(1, "zero-count configure was not an empty accepted range");
            $display("PASS B=%0d zero-count range accepted", B_MAX);
            $finish;
        end
        if ($test$plusargs("RANGE_LIMIT_BASE0"))
        begin
            configure_valid_range(32'h00000000, 32'h40000000);
            $display("PASS B=%0d base=0 count=0x40000000 accepted at 32-bit limit", B_MAX);
            $finish;
        end
        if ($test$plusargs("RANGE_LIMIT_BASE4"))
        begin
            configure_valid_range(32'h00000004, 32'h3fffffff);
            $display("PASS B=%0d base=4 count=0x3fffffff accepted at 32-bit limit", B_MAX);
            $finish;
        end
        if ($test$plusargs("RANGE_OVER_BASE4"))
        begin
            configure_invalid_range(32'h00000004, 32'h40000000);
            $display("PASS B=%0d base=4 count=0x40000000 overflow rejected", B_MAX);
            $finish;
        end
        if ($test$plusargs("RANGE_OVER_PLUS1"))
        begin
            configure_invalid_range(32'h00000000, 32'h40000001);
            $display("PASS B=%0d count=0x40000001 overflow rejected", B_MAX);
            $finish;
        end
        if ($test$plusargs("RANGE_OVER_HALF"))
        begin
            configure_invalid_range(32'h00000000, 32'h80000000);
            $display("PASS B=%0d count=0x80000000 overflow rejected", B_MAX);
            $finish;
        end
        if ($test$plusargs("RANGE_OVER_MAX"))
        begin
            configure_invalid_range(32'h00000000, 32'hffffffff);
            $display("PASS B=%0d count=0xffffffff overflow rejected", B_MAX);
            $finish;
        end
        if ($test$plusargs("RANGE_TOP_VALID"))
        begin
            configure_valid_range(32'hfffffffc, 32'h00000001);
            $display("PASS B=%0d last aligned beat at 0xffffffff accepted", B_MAX);
            $finish;
        end
        if ($test$plusargs("RANGE_TOP_OVER"))
        begin
            configure_invalid_range(32'hfffffffc, 32'h00000002);
            $display("PASS B=%0d range beyond 0xffffffff rejected", B_MAX);
            $finish;
        end
        if ($test$plusargs("BADALIGN"))
        begin
            configure_invalid(32'h00001002, 1, 32'd32);
            if (ar_count != 0)
                $fatal(1, "misaligned range issued AXI traffic");
            $display("PASS B=%0d misaligned range rejected", B_MAX);
            $finish;
        end
        if ($test$plusargs("OVERFLOW"))
        begin
            configure_invalid(32'hfffffffc, 2, 32'd32);
            if (ar_count != 0)
                $fatal(1, "overflowing range issued AXI traffic");
            $display("PASS B=%0d overflowing range rejected", B_MAX);
            $finish;
        end
        if ($test$plusargs("BADSIZE_CONFIG"))
        begin
            configure_invalid(32'h00001000, 1, 32'd16);
            if (ar_count != 0)
                $fatal(1, "malformed-size configure issued AXI traffic");
            $display("PASS B=%0d malformed configure size rejected", B_MAX);
            $finish;
        end
        if ($test$plusargs("STORE"))
        begin
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
        if ($test$plusargs("BADADDR_EMPTY"))
        begin
            test_invalid_load(1'b1, 1'b0);
        end
        if ($test$plusargs("BADSIZE_EMPTY"))
        begin
            test_invalid_load(1'b0, 1'b0);
        end
        if ($test$plusargs("BADADDR_FULL"))
        begin
            test_invalid_load(1'b1, 1'b1);
        end
        if ($test$plusargs("BADSIZE_FULL"))
        begin
            test_invalid_load(1'b0, 1'b1);
        end

        if ($test$plusargs("BADRID") || $test$plusargs("EARLY_RLAST") ||
            $test$plusargs("MISSING_RLAST"))
            begin
            integer watchdog;
            integer beats_before;
            inject_wrong_rid = $test$plusargs("BADRID");
            inject_early_rlast = $test$plusargs("EARLY_RLAST");
            suppress_rlast = $test$plusargs("MISSING_RLAST");
            stall_ar = 1'b0;
            beats_before = r_beat_count;
            configure(32'h0000f000, 4);
            watchdog = 0;
            while (!fault && watchdog < 20000)
            begin
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

        if ($test$plusargs("RESET_TRAFFIC"))
        begin
            integer watchdog;
            stall_ar = 1'b0;
            configure(32'h00013000, 16);
            watchdog = 0;
            while ((!slave_active || dut.descriptor_count_reg == 0 || rvalid || !rready) &&
                   watchdog < 20000)
                   begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (!slave_active || dut.descriptor_count_reg == 0 || rvalid || !rready)
                $fatal(1, "reset-under-traffic gap setup failed: RVALID=%b active=%b outstanding=%b RREADY=%b",
                       rvalid, slave_active, dut.descriptor_count_reg, rready);
            // Reset during the response gap, not on a cycle that would handshake.
            reset_n = 1'b0;
            @(posedge clk);
            #1;
            if (slave_active || rvalid || slave_left != 0 ||
                dut.region_active_reg || dut.descriptor_count_reg != 0 || dut.m_axi_arvalid_reg ||
                dut.fifo_count_reg != 0 || dut.issued_reg != 0 || dut.returned_reg != 0 || dut.consumed_reg != 0)
                $fatal(1, "coordinated reset failed to discard in-flight AXI state");
            repeat (2) @(negedge clk);
            reset_n = 1'b1;
            repeat (2) @(posedge clk);
            configure(32'h00014000, 5);
            wait (dut.returned_reg == 5);
            consume_stream(32'h00014000, 5);
            wait_drained();
            check_descriptors(32'h00014000, 5, 0);
            $display("PASS B=%0d coordinated reset in outstanding-burst response gap, then clean recovery",
                     B_MAX);
            $finish;
        end

        if ($test$plusargs("ASYNC_RESET"))
        begin
            integer watchdog;
            if (!ASYNC_RESET)
                $fatal(1, "ASYNC_RESET_CASE_REQUIRES_ASYNC_VARIANT");
            stall_ar = 1'b0;
            configure(32'h00013000, 16);
            watchdog = 0;
            while ((!slave_active || dut.descriptor_count_reg == 0 || rvalid || !rready) &&
                   watchdog < 20000)
                   begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (!slave_active || dut.descriptor_count_reg == 0 || rvalid || !rready)
                $fatal(1, "asynchronous reset gap setup failed: RVALID=%b active=%b outstanding=%b RREADY=%b",
                       rvalid, slave_active, dut.descriptor_count_reg, rready);
            // The reset is asserted in the middle of a cycle and inspected 1 ns later: no clock
            // edge separates the assertion from the check, so only a block that is genuinely
            // sensitive to the asynchronous reset edge can discard the in-flight state here.
            @(negedge clk);
            #1 reset_n = 1'b0;
            #1;
            if (dut.region_active_reg || dut.descriptor_count_reg != 0 || dut.m_axi_arvalid_reg ||
                dut.fifo_count_reg != 0 || dut.issued_reg != 0 || dut.returned_reg != 0 || dut.consumed_reg != 0 ||
                dut.fifo_credit_committed_reg != FIFO_DEPTH ||
                dut.fifo_credit_minus_one_reg != FIFO_DEPTH - 1 || dut.fifo_credit_token_reg != 1'b0)
                $fatal(1, "ASYNC_RESET_NOT_IMMEDIATE active=%b descriptors=%0d arvalid=%b fifo=%0d issued=%0d returned=%0d consumed=%0d credit=%0d",
                       dut.region_active_reg, dut.descriptor_count_reg, dut.m_axi_arvalid_reg, dut.fifo_count_reg,
                       dut.issued_reg, dut.returned_reg, dut.consumed_reg, dut.fifo_credit_committed_reg);
            @(posedge clk);
            @(negedge clk);
            reset_n = 1'b1;
            repeat (2) @(posedge clk);
            configure(32'h00014000, 5);
            wait (dut.returned_reg == 5);
            consume_stream(32'h00014000, 5);
            wait_drained();
            check_descriptors(32'h00014000, 5, 0);
            $display("PASS B=%0d asynchronous reset between clock edges discards in-flight AXI state, then clean recovery",
                     B_MAX);
            $finish;
        end

        if ($test$plusargs("RESET_MULTI"))
        begin
            integer watchdog;
            integer restarted_n;
            integer reset_descriptors;
            restarted_n = 20;
            stall_ar = 1'b0;
            hold_responses_until = 2;
            configure(32'h00022000, 64);
            watchdog = 0;
            while (dut.descriptor_count_reg < 2 && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.descriptor_count_reg < 2 || bus_desc_count < 2 || ar_count < 2 || r_beat_count != 0)
                $fatal(1, "multi-reset setup lacks two accepted descriptors: DUT=%0d bus=%0d AR=%0d R=%0d",
                       dut.descriptor_count_reg, bus_desc_count, ar_count, r_beat_count);
            reset_descriptors = bus_desc_count;
            @(negedge clk);
            reset_n = 1'b0;
            @(posedge clk);
            #1;
            if (dut.descriptor_count_reg != 0 || bus_desc_count != 0 || slave_q_count != 0 ||
                dut.m_axi_arvalid_reg || dut.issued_reg != 0 || dut.returned_reg != 0 || dut.consumed_reg != 0)
                $fatal(1, "coordinated reset did not discard multiple accepted descriptors");
            repeat (2) @(negedge clk);
            reset_n = 1'b1;
            multi_reset_hits = 1;
            hold_responses_until = 2;
            configure(32'h00023000, restarted_n);
            watchdog = 0;
            while (dut.descriptor_count_reg < 2 && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.descriptor_count_reg < 2)
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

        if ($test$plusargs("FAULT_STALLED_AR"))
        begin
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
            while (dut.descriptor_count_reg != 2 && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.descriptor_count_reg != 2 || ar_count != 2)
                $fatal(1, "fault/stalled-AR setup did not accept two descriptors");
            stall_ar = 1'b1;
            hold_responses_until = 0;
            watchdog = 0;
            while ((!arvalid || !rvalid || !rready || !prev_stalled) && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (!arvalid || !rvalid || !rready || !prev_stalled)
                $fatal(1, "fault/stalled-AR setup lacks stalled AR plus accepted-response opportunity");
            held_addr = araddr;
            held_len = arlen;
            issued_before_fault = dut.issued_reg;
            returned_before_fault = dut.returned_reg;
            fifo_before_fault = dut.fifo_count_reg;
            accepted_before_fault = ar_count;
            inject_wrong_rid = 1'b1;
            #1;
            if (!arvalid || arready || araddr != held_addr || arlen != held_len || !rready)
                $fatal(1, "failed to align malformed R under a stable stalled AR");
            @(posedge clk);
            #1;
            inject_wrong_rid = 1'b0;
            if (!fault || dut.returned_reg != returned_before_fault ||
                dut.fifo_count_reg != fifo_before_fault || dut.consumed_reg != 0 || done)
                $fatal(1, "malformed R was counted or consumed instead of faulting");
            repeat (3)
            begin
                @(negedge clk);
                #1;
                if (!arvalid || arready || araddr != held_addr || arlen != held_len ||
                    dut.issued_reg != issued_before_fault || ar_count != accepted_before_fault)
                    $fatal(1, "stalled AR changed or a new reservation appeared after malformed R");
            end
            stalled_fault_ar_hits = stalled_fault_ar_hits + 1;
            stall_ar = 1'b0;
            watchdog = 0;
            while (ar_count != accepted_before_fault+1 && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (ar_count != accepted_before_fault+1 || arvalid ||
                dut.issued_reg != issued_before_fault+held_len+1 ||
                seen_addr[accepted_before_fault] != held_addr ||
                seen_len[accepted_before_fault] != held_len ||
                dut.returned_reg != returned_before_fault || dut.fifo_count_reg != fifo_before_fault)
                $fatal(1, "faulted held AR did not handshake exactly once without accepting bad data");
            repeat (3)
            begin
                @(negedge clk);
                if (arvalid || ar_count != accepted_before_fault+1 || !fault ||
                    dut.returned_reg != returned_before_fault ||
                    dut.fifo_count_reg != fifo_before_fault)
                    $fatal(1, "new reservation/data acceptance continued after sticky fault");
            end
            if (stalled_fault_ar_hits == 0)
                $fatal(1, "required malformed-R/stalled-AR hit count is zero");
            $display("PASS malformed R with held AR: stable through handshake, no extra reservation, bad beat unconsumed hits=%0d",
                     stalled_fault_ar_hits);
            $finish;
        end

        if ($test$plusargs("BOUNDARY_PENDING"))
        begin
            integer watchdog;
            integer first_descriptor;
            integer target_n;
            target_n = 40;
            first_descriptor = ar_count;
            hold_responses_until = MAX_OUTSTANDING;
            stall_ar = 1'b0;
            configure(32'h00000ffc, target_n);
            watchdog = 0;
            while (dut.descriptor_count_reg != 3 && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (MAX_OUTSTANDING < 3 || dut.descriptor_count_reg != 3 ||
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
            while (dut.region_base_reg != 32'h00030000 && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.region_base_reg != 32'h00030000 || dut.region_count_reg != 5 || dut.cfg_pending_reg)
                $fatal(1, "pending configure did not activate after multi-descriptor drain");
            wait (dut.returned_reg == 5);
            consume_stream(32'h00030000, 5);
            wait_drained();
            $display("PASS O=%0d 4-KiB boundary/tail and pending configure while three descriptors active",
                     MAX_OUTSTANDING);
            $finish;
        end

        if ($test$plusargs("PENDING_SECOND"))
        begin
            integer watchdog;
            stall_ar = 1'b0;
            hold_responses_until = 2;
            configure(32'h00032000, 64);
            watchdog = 0;
            while (dut.descriptor_count_reg != 2 && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.descriptor_count_reg != 2)
                $fatal(1, "second-pending setup lacks multiple outstanding descriptors");
            capture_busy_config(32'h00033000, 5);
            if (fault || !dut.cfg_pending_reg || dut.pending_base_reg != 32'h00033000)
                $fatal(1, "first busy configure was not retained");
            @(negedge clk);
            start = 1'b1;
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = 32'h00034000;
            count = 7;
            @(posedge clk); // E0 captures the conflicting second request.
            #1;
            if (fault || done || !dut.cfg_pending_reg || dut.pending_base_reg != 32'h00033000 ||
                dut.pending_count_reg != 5)
                $fatal(1, "second pending request changed the original slot at E0");
            @(negedge clk);
            start = 1'b0;
            opcode = 2'b0;
            size_bits = 32'b0;
            payload = 32'b0;
            byte_address = 32'b0;
            count = 32'b0;
            @(posedge clk); // E1 rejects without overwriting the queued descriptor.
            #1;
            if (!fault || !dut.cfg_pending_reg || dut.pending_base_reg != 32'h00033000 ||
                dut.pending_count_reg != 5)
                $fatal(1, "second distinct busy configure did not fault/preserve first pending slot");
            if (ar_count != 2 || dut.descriptor_count_reg != 2)
                $fatal(1, "second pending configure changed active descriptor accounting");
            $display("PASS second distinct configure faults while O=%0d descriptors and pending slot are active",
                     dut.descriptor_count_reg);
            $finish;
        end

        if ($test$plusargs("DEPTH4096"))
        begin
            integer watchdog;
            integer target_n;
            target_n = 4100;
            stall_ar = 1'b0;
            configure(32'h00040000, target_n);
            watchdog = 0;
            while (dut.fifo_count_reg != 4096 && watchdog < 1000000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.fifo_count_reg != 4096 || dut.issued_reg != 4096 ||
                dut.returned_reg != 4096 || dut.consumed_reg != 0 || rready || arvalid)
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

        if ($test$plusargs("FIFO_BACKPRESSURE"))
        begin
            integer watchdog;
            integer held_descriptors;
            stall_ar = 1'b0;
            configure(32'h00015000, 300);
            watchdog = 0;
            while (dut.returned_reg != 256 && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.returned_reg != FIFO_DEPTH || dut.fifo_count_reg != FIFO_DEPTH ||
                dut.issued_reg != FIFO_DEPTH || rready || arvalid || slave_active)
                $fatal(1, "FIFO credit limit failed: returned=%0d fifo=%0d issued=%0d RREADY=%b ARVALID=%b active=%b",
                       dut.returned_reg, dut.fifo_count_reg, dut.issued_reg, rready, arvalid, slave_active);
            held_descriptors = ar_count;
            repeat (16)
            begin
                @(negedge clk);
                if (dut.fifo_count_reg != FIFO_DEPTH || dut.issued_reg != FIFO_DEPTH || dut.returned_reg != FIFO_DEPTH ||
                    rready || arvalid || ar_count != held_descriptors)
                    $fatal(1, "full FIFO allowed additional traffic or failed to backpressure");
            end
            consume_stream(32'h00015000, 300);
            wait_drained();
            check_descriptors(32'h00015000, 300, 0);
            if (dut.consumed_reg != 300 || dut.returned_reg != 300 || dut.fifo_count_reg != 0)
                $fatal(1, "FIFO did not resume and drain after consumer resumed");
            $display("PASS B=%0d FIFO full backpressure at 256 credits, resume/drain N=300",
                     B_MAX);
            $finish;
        end

        if ($test$plusargs("FIFO_WRAP"))
        begin
            integer watchdog;
            configure(32'h00016000, 600);
            fifo_write_wraps = 0;
            fifo_read_wraps = 0;
            simultaneous_push_pop = 0;
            stall_ar = 1'b0;
            watchdog = 0;
            while ((dut.fifo_count_reg != 1 || !rvalid || !rready) && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.fifo_count_reg != 1 || !rvalid || !rready)
                $fatal(1, "failed to align FIFO push/pop event: count=%0d RVALID=%b RREADY=%b",
                       dut.fifo_count_reg, rvalid, rready);
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

        if ($test$plusargs("AQC"))
        begin
            integer target_n;
            integer watchdog;
            integer descriptors_before;
            target_n = 64;
            descriptors_before = ar_count;
            hold_responses_until = 2;
            stall_ar = 1'b0;
            configure(32'h0001e000, target_n);
            watchdog = 0;
            while (dut.descriptor_count_reg != MAX_OUTSTANDING && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.descriptor_count_reg != 2 || ar_count-descriptors_before != 2)
                $fatal(1, "A+Q+C setup did not accept two descriptors");
            // Free a descriptor slot while holding its replacement AR. Keep
            // responses running until one is ready behind buffered data.
            stall_ar = 1'b1;
            hold_responses_until = 0;
            watchdog = 0;
            while ((!arvalid || !rvalid || !rready || dut.fifo_count_reg == 0 ||
                    !rlast || bus_desc_count == 0 ||
                    bus_desc_remaining[bus_desc_rd_ptr] != 1) &&
                   watchdog < 100000)
                   begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (!arvalid || !rvalid || !rready || dut.fifo_count_reg == 0 ||
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

        if ($test$plusargs("MULTI"))
        begin
            integer target_n;
            integer watchdog;
            integer descriptors_before;
            target_n = B_MAX * MAX_OUTSTANDING + 2;
            descriptors_before = ar_count;
            hold_responses_until = MAX_OUTSTANDING;
            stall_ar = 1'b0;
            configure(32'h00018000, target_n);
            watchdog = 0;
            while (dut.descriptor_count_reg != MAX_OUTSTANDING && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.descriptor_count_reg != MAX_OUTSTANDING ||
                ar_count - descriptors_before != MAX_OUTSTANDING || r_beat_count != 0)
                $fatal(1, "failed to fill outstanding descriptor queue: O=%0d descriptors=%0d Rbeats=%0d",
                       MAX_OUTSTANDING, dut.descriptor_count_reg, r_beat_count);
            if (dut.descriptor_count_reg > peak_descriptors)
                peak_descriptors = dut.descriptor_count_reg;
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

        if ($test$plusargs("CREDIT_LIMIT"))
        begin
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
            while (dut.issued_reg != expected_issue && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.issued_reg != expected_issue || dut.descriptor_count_reg != expected_descriptors ||
                dut.issued_reg - dut.consumed_reg > FIFO_DEPTH ||
                (dut.m_axi_arvalid_reg && dut.issued_reg-dut.consumed_reg+
                 ({24'b0,dut.m_axi_arlen_reg}+1) > FIFO_DEPTH))
                $fatal(1, "credit limit failed O=%0d D=%0d issued=%0d desc=%0d ARVALID=%b",
                       MAX_OUTSTANDING, FIFO_DEPTH, dut.issued_reg, dut.descriptor_count_reg,
                       dut.m_axi_arvalid_reg);
            hold_responses_until = 0;
            consume_stream(32'h0001a000, target_n);
            wait_drained();
            check_descriptors(32'h0001a000, target_n, 0);
            $display("PASS credit-limited O=%0d D=%0d reserved=%0d issued first; stop/restart drained N=%0d",
                     MAX_OUTSTANDING, FIFO_DEPTH, expected_issue, target_n);
            $finish;
        end

        if ($test$plusargs("CREDIT_POP"))
        begin
            integer target_n;
            integer watchdog;
            target_n = 40;
            hold_responses_until = 2;
            stall_ar = 1'b0;
            configure(32'h0001c000, target_n);
            watchdog = 0;
            while (dut.issued_reg != 32 && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.issued_reg != 32 || dut.descriptor_count_reg != 2)
                $fatal(1, "D-1 push/pop setup did not reserve two bursts");
            hold_responses_until = 0;
            watchdog = 0;
            while ((dut.fifo_count_reg != FIFO_DEPTH-1 || !rvalid || !rready) &&
                   watchdog < 100000)
                   begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.fifo_count_reg != FIFO_DEPTH-1 || !rvalid || !rready)
                $fatal(1, "failed to align R plus FIFO pop at D-1 occupancy=%0d", dut.fifo_count_reg);
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

        if ($test$plusargs("READY_CREDIT_PROOF"))
        begin
            integer target_n;
            integer watchdog;
            integer held_ar_count;
            reg [31:0] test_base;
            target_n = FIFO_DEPTH + 4;
            test_base = 32'h0001d000;
            stall_ar = 1'b0;
            configure(test_base, target_n);
            watchdog = 0;
            while (dut.fifo_count_reg != FIFO_DEPTH && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.fifo_count_reg != FIFO_DEPTH || dut.descriptor_count_reg != 0 ||
                dut.m_axi_arvalid_reg || arvalid || rready || slave_active ||
                dut.fifo_credit != 0 || dut.returned_reg != FIFO_DEPTH ||
                dut.consumed_reg != 0)
                $fatal(1, "READY_CREDIT_PROOF failed to reach isolated full FIFO: fifo=%0d desc=%0d ARVALID=%b RREADY=%b active=%b credit=%0d returned=%0d consumed=%0d",
                       dut.fifo_count_reg, dut.descriptor_count_reg, arvalid, rready,
                       slave_active, dut.fifo_credit, dut.returned_reg, dut.consumed_reg);
            held_ar_count = ar_count;
            $display("PASS READY_CREDIT_PROOF FULL O=%0d D=%0d AR=%0d",
                     MAX_OUTSTANDING, FIFO_DEPTH, held_ar_count);

            @(negedge clk);
            start = 1'b1;
            opcode = 2'd0;
            size_bits = 32'd32;
            byte_address = test_base;
            #1;
            if (!done || !dut.fifo_load_done || rready ||
                data_out !== (test_base >> 2))
                $fatal(1, "READY_CREDIT_PROOF full-FIFO LOAD did not complete with RREADY low");
            @(posedge clk);
            @(negedge clk);
            start = 1'b0;
            if (dut.fifo_count_reg != FIFO_DEPTH-1 || dut.fifo_credit != 1 ||
                dut.consumed_reg != 1 || rready)
                $fatal(1, "READY_CREDIT_PROOF LOAD failed to release exactly one credit: fifo=%0d credit=%0d consumed=%0d RREADY=%b",
                       dut.fifo_count_reg, dut.fifo_credit, dut.consumed_reg, rready);

            watchdog = 0;
            while (dut.issued_reg != FIFO_DEPTH+1 && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.issued_reg != FIFO_DEPTH+1 || ar_count <= held_ar_count)
                $fatal(1, "READY_CREDIT_PROOF released credit did not permit a new AR: issued=%0d AR=%0d prior=%0d",
                       dut.issued_reg, ar_count, held_ar_count);
            watchdog = 0;
            while (dut.returned_reg != FIFO_DEPTH+1 && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.returned_reg != FIFO_DEPTH+1 || dut.fifo_count_reg != FIFO_DEPTH)
                $fatal(1, "READY_CREDIT_PROOF released credit did not produce the next response: returned=%0d fifo=%0d",
                       dut.returned_reg, dut.fifo_count_reg);

            consume_stream(test_base + 4, target_n-1);
            wait_drained();
            check_descriptors(test_base, target_n, 0);
            if (dut.issued_reg != target_n || dut.returned_reg != target_n ||
                dut.consumed_reg != target_n || bus_returned_beats != target_n ||
                dut.fifo_count_reg != 0)
                $fatal(1, "READY_CREDIT_PROOF region did not drain exactly once: issued=%0d returned=%0d consumed=%0d bus=%0d fifo=%0d",
                       dut.issued_reg, dut.returned_reg, dut.consumed_reg, bus_returned_beats,
                       dut.fifo_count_reg);
            $display("PASS READY_CREDIT_PROOF O=%0d D=%0d full-LOAD-credit-AR-response-drain N=%0d",
                     MAX_OUTSTANDING, FIFO_DEPTH, target_n);
            $finish;
        end

        if ($test$plusargs("ERROR"))
        begin
            stall_ar = 1'b0;
            configure(32'h00007000, 1);
            wait (fault);
            if (done)
                $fatal(1, "done asserted after response fault");
            $display("PASS B=%0d error fault surfaced", B_MAX);
            $finish;
        end
        if ($test$plusargs("RRESP_FIRST") || $test$plusargs("RRESP_INTERMEDIATE") ||
            $test$plusargs("RRESP_FINAL"))
            begin
            integer watchdog;
            integer error_index;
            integer base_transport;
            integer base_payload;
            reg [31:0] test_base;
            test_base = 32'h00007100;
            if ($test$plusargs("RRESP_FIRST"))
                error_index = 0;
            else if ($test$plusargs("RRESP_FINAL"))
                error_index = 3;
            else
                error_index = 1;
            stall_ar = 1'b0;
            configure(test_base, 4);
            inject_error_at = r_beat_count + error_index;
            watchdog = 0;
            while (!fault && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (!fault || done || bus_transport_beats != error_index + 1 ||
                bus_returned_beats != error_index || integer'(dut.returned_reg) != error_index ||
                integer'(dut.fifo_count_reg) != error_index)
                $fatal(1, "RRESP witness did not fault/discard at beat %0d: transport=%0d payload=%0d returned=%0d fifo=%0d done=%b",
                       error_index, bus_transport_beats, bus_returned_beats,
                       dut.returned_reg, dut.fifo_count_reg, done);
            base_transport = bus_transport_beats;
            base_payload = bus_returned_beats;
            start = 1'b1;
            opcode = 2'd0;
            size_bits = 32'd32;
            byte_address = test_base;
            #1;
            if (done)
                $fatal(1, "LOAD completed successfully after RRESP fault");
            @(posedge clk);
            #1;
            if (done || integer'(dut.fifo_count_reg) != base_payload ||
                bus_returned_beats != base_payload)
                $fatal(1, "post-fault LOAD consumed or appended payload");
            @(negedge clk);
            start = 1'b0;
            watchdog = 0;
            while ((bus_desc_count != 0 || slave_q_count != 0 || arvalid) &&
                   watchdog < 100000)
                   begin
                @(negedge clk);
                #1;
                watchdog = watchdog + 1;
                if (!fault || done || integer'(dut.fifo_count_reg) != base_payload ||
                    bus_returned_beats != base_payload || ar_count != 1)
                    $fatal(1, "RRESP drain wrote payload, completed LOAD, or issued another AR");
            end
            if (bus_desc_count != 0 || slave_q_count != 0 ||
                bus_transport_beats != 4 || dut.descriptor_count_reg != 0 ||
                bus_issued_beats != bus_transport_beats ||
                bus_transport_beats - base_transport != 4 - (error_index + 1))
                $fatal(1, "RRESP transport did not drain exactly once: issued=%0d transported=%0d bus_desc=%0d dut_desc=%0d",
                       bus_issued_beats, bus_transport_beats, bus_desc_count,
                       dut.descriptor_count_reg);

            @(negedge clk);
            start = 1'b0;
            reset_n = 1'b0;
            repeat (3) @(posedge clk);
            @(negedge clk);
            reset_n = 1'b1;
            repeat (2) @(posedge clk);
            inject_error_at = -1;
            configure(32'h00007200, 3);
            wait (dut.returned_reg == 3);
            consume_stream(32'h00007200, 3);
            wait_drained();
            check_descriptors(32'h00007200, 3, 0);
            if ($test$plusargs("RRESP_FIRST"))
                $display("PASS RRESP first-beat error drained transport and reset recovered");
            else if ($test$plusargs("RRESP_FINAL"))
                $display("PASS RRESP final-beat error drained transport and reset recovered");
            else
                $display("PASS RRESP intermediate-beat error drained transport and reset recovered");
            $finish;
        end
        if ($test$plusargs("RRESP_STALLED_AR"))
        begin
            integer watchdog;
            integer accepted_before_fault;
            integer fifo_before_fault;
            reg [31:0] held_addr;
            reg [7:0] held_len;
            stall_ar = 1'b0;
            hold_responses_until = 2;
            configure(32'h00007300, 64);
            watchdog = 0;
            while (dut.descriptor_count_reg != 2 && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (dut.descriptor_count_reg != 2 || ar_count != 2)
                $fatal(1, "RRESP/stalled-AR setup did not accept two descriptors");
            stall_ar = 1'b1;
            hold_responses_until = 0;
            watchdog = 0;
            while ((!arvalid || !rvalid || !rready || !prev_stalled) &&
                   watchdog < 100000)
                   begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (!arvalid || !rvalid || !rready || !prev_stalled || ar_count != 2)
                $fatal(1, "RRESP/stalled-AR setup lacks two accepted descriptors and held third AR");
            held_addr = araddr;
            held_len = arlen;
            accepted_before_fault = ar_count;
            inject_error = 1'b1;
            #1;
            if (!arvalid || arready || !rready || araddr != held_addr || arlen != held_len)
                $fatal(1, "could not align RRESP with stable stalled AR");
            @(posedge clk);
            #1;
            inject_error = 1'b0;
            if (!fault || !arvalid || arready || araddr != held_addr ||
                arlen != held_len || done)
                $fatal(1, "RRESP did not fault while preserving the exposed stalled AR");
            fifo_before_fault = integer'(dut.fifo_count_reg);
            start = 1'b1;
            opcode = 2'd0;
            size_bits = 32'd32;
            byte_address = 32'h00007300;
            #1;
            if (done)
                $fatal(1, "LOAD completed successfully after RRESP fault with stalled AR");
            @(posedge clk);
            #1;
            if (done || integer'(dut.fifo_count_reg) != fifo_before_fault ||
                bus_returned_beats != fifo_before_fault)
                $fatal(1, "post-fault LOAD consumed or appended payload with stalled AR");
            @(negedge clk);
            start = 1'b0;
            stall_ar = 1'b0;
            watchdog = 0;
            while ((bus_desc_count != 0 || slave_q_count != 0 || arvalid) &&
                   watchdog < 100000)
                   begin
                @(negedge clk);
                #1;
                watchdog = watchdog + 1;
                if (!fault || done || integer'(dut.fifo_count_reg) != fifo_before_fault ||
                    bus_returned_beats != fifo_before_fault || ar_count > accepted_before_fault+1)
                    $fatal(1, "RRESP drain wrote payload, completed LOAD, or issued extra AR");
            end
            if (ar_count != accepted_before_fault+1 || bus_desc_count != 0 ||
                slave_q_count != 0 || dut.descriptor_count_reg != 0 ||
                bus_transport_beats != bus_issued_beats ||
                dut.issued_reg != bus_issued_beats || !fault || done)
                $fatal(1, "RRESP drain failed to retire all committed descriptors exactly once");
            $display("PASS RRESP drain preserved and accepted one exposed stalled AR; transport=%0d payload=%0d",
                     bus_transport_beats, bus_returned_beats);
            $finish;
        end
        if ($test$plusargs("RRESP_RESERVATION_RACE"))
        begin
            integer watchdog;
            stall_ar = 1'b0;
            hold_responses_until = 99;
            configure(32'h00007400, 64);
            watchdog = 0;
            while ((ar_count != 2 || dut.descriptor_count_reg != 2 || arvalid ||
                    !dut.reservation_eligible) && watchdog < 100000)
                    begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (ar_count != 2 || dut.descriptor_count_reg != 2 || arvalid ||
                !dut.reservation_eligible || fault)
                $fatal(1, "RRESP reservation-race setup lacks two committed descriptors and a free eligible slot");
            force_rvalid = 1'b1;
            inject_error = 1'b1;
            #1;
            if (!rvalid || !rready || arvalid || dut.fault_reg ||
                !dut.region_active_reg || dut.issued_reg >= dut.region_count_reg ||
                dut.m_axi_arvalid_reg ||
                (dut.descriptor_count_reg + dut.m_axi_arvalid_reg) >= MAX_OUTSTANDING ||
                dut.fifo_credit == 0 || dut.reservation_eligible)
                $fatal(1, "RRESP reservation-race stimulus is not aligned before the edge");
            @(posedge clk);
            #1;
            force_rvalid = 1'b0;
            inject_error = 1'b0;
            if (!fault || arvalid || ar_count != 2 || done)
                $fatal(1, "same-edge RRESP fault exposed or accepted a new AR");
            hold_responses_until = 0;
            watchdog = 0;
            while ((bus_desc_count != 0 || slave_q_count != 0 || arvalid) &&
                   watchdog < 100000)
                   begin
                @(negedge clk);
                #1;
                watchdog = watchdog + 1;
                if (!fault || done || arvalid || ar_count != 2 ||
                    bus_returned_beats != 0 || dut.fifo_count_reg != 0)
                    $fatal(1, "RRESP reservation-race drain exposed an AR, wrote payload, or completed");
            end
            if (bus_desc_count != 0 || slave_q_count != 0 || dut.descriptor_count_reg != 0 ||
                bus_transport_beats != bus_issued_beats || bus_issued_beats != 32 ||
                bus_returned_beats != 0 || dut.fifo_count_reg != 0 || ar_count != 2 || !fault || done)
                $fatal(1, "RRESP reservation-race did not drain exactly the two committed descriptors");
            $display("PASS RRESP same-edge fault suppressed unexposed reservation and drained committed transport");
            $finish;
        end
        if ($test$plusargs("RREADY_SIDEBAND"))
        begin
            integer watchdog;
            integer beats_at_toggle;
            logic ready_before_toggle;
            stall_ar = 1'b0;
            configure(32'h00007500, 8);
            watchdog = 0;
            while ((!rvalid || !rready || dut.descriptor_count_reg == 0) && watchdog < 100000)
            begin
                @(negedge clk);
                watchdog = watchdog + 1;
            end
            if (!rvalid || !rready || dut.descriptor_count_reg == 0)
                $fatal(1, "RREADY sideband witness did not obtain a committed response");
            inject_error = 1'b1;
            @(posedge clk);
            #1;
            inject_error = 1'b0;
            if (!fault || dut.descriptor_count_reg == 0 || done ||
                dut.returned_reg != 0 || dut.fifo_count_reg != 0 || bus_returned_beats != 0)
                $fatal(1, "RREADY sideband witness failed to establish a sticky RRESP fault with outstanding transport");
            force_rvalid = 1'b1;
            #1;
            ready_before_toggle = rready;
            beats_at_toggle = r_beat_count;
            if (!ready_before_toggle || !fault || dut.descriptor_count_reg == 0)
                $fatal(1, "RREADY sideband witness did not enter discard-ready state");
            if (rlast !== 1'b0)
                $fatal(1, "RREADY sideband witness expected RLAST low after the first beat");
            inject_wrong_rid = 1'b1;
            #1;
            if (rready !== ready_before_toggle || !rvalid || r_beat_count != beats_at_toggle)
                $fatal(1, "RREADY changed with RID while sticky fault was set and no clock edge occurred");
            inject_wrong_rid = 1'b0;
            force_rlast = 1'b1;
            forced_rlast = 1'b1;
            #1;
            if (rlast !== 1'b1 || rready !== ready_before_toggle ||
                !rvalid || r_beat_count != beats_at_toggle)
                $fatal(1, "RREADY changed or RLAST did not rise while sticky fault was set without a clock edge");
            forced_rlast = 1'b0;
            #1;
            if (rlast !== 1'b0 || rready !== ready_before_toggle ||
                !rvalid || r_beat_count != beats_at_toggle)
                $fatal(1, "RREADY changed or RLAST did not fall while sticky fault was set without a clock edge");
            force_rlast = 1'b0;
            inject_error = 1'b1;
            #1;
            if (rready !== ready_before_toggle || !rvalid || r_beat_count != beats_at_toggle)
                $fatal(1, "RREADY changed with RRESP while sticky fault was set and no clock edge occurred");
            inject_error = 1'b0;
            #1;
            if (rready !== ready_before_toggle || !rvalid || r_beat_count != beats_at_toggle)
                $fatal(1, "RREADY did not remain stable after response sideband restoration");
            force_rvalid = 1'b0;
            start = 1'b1;
            opcode = 2'd0;
            size_bits = 32'd32;
            byte_address = 32'h00007500;
            #1;
            if (done)
                $fatal(1, "LOAD completed successfully after sticky RRESP fault");
            @(posedge clk);
            #1;
            if (done || dut.returned_reg != 0 || dut.fifo_count_reg != 0 ||
                bus_returned_beats != 0)
                $fatal(1, "post-fault LOAD consumed or appended payload in RREADY sideband witness");
            @(negedge clk);
            start = 1'b0;
            watchdog = 0;
            while ((bus_desc_count != 0 || slave_q_count != 0 || arvalid) &&
                   watchdog < 100000)
                   begin
                @(negedge clk);
                #1;
                watchdog = watchdog + 1;
                if (!fault || done || dut.returned_reg != 0 || dut.fifo_count_reg != 0 ||
                    bus_returned_beats != 0 || ar_count != 1)
                    $fatal(1, "RREADY sideband discard drain accepted payload, completed LOAD, or issued another AR");
            end
            if (bus_desc_count != 0 || slave_q_count != 0 ||
                dut.descriptor_count_reg != 0 || bus_transport_beats != bus_issued_beats ||
                dut.issued_reg != bus_issued_beats || bus_issued_beats != 8 ||
                bus_returned_beats != 0 || dut.fifo_count_reg != 0 || ar_count != 1 ||
                !fault || done)
                $fatal(1, "RREADY sideband witness failed to drain committed transport without payload");
            $display("PASS RREADY stayed stable across RID/RLAST/RRESP toggles under sticky fault; drained=%0d payload=0",
                     bus_transport_beats);
            $finish;
        end
        if ($test$plusargs("DRAIN_CONFIG_RACE"))
        begin
            integer completions_before;
            integer watchdog;
            stall_ar = 1'b0;
            configure(32'h0000b000, 1);
            completions_before = configure_completion_count;
            wait (dut.returned_reg == 1);
            capture_busy_config(32'h0000c000, 2);
            if (configure_completion_count != completions_before)
                $fatal(1, "busy configure acknowledged before the old region drained");
            consume_stream(32'h0000b000, 1);
            watchdog = 0;
            while ((dut.region_base_reg != 32'h0000c000 ||
                    configure_completion_count != completions_before + 1) && watchdog < 32)
                    begin
                @(posedge clk);
                #1;
                watchdog = watchdog + 1;
            end
            if (watchdog == 32 || !dut.region_active_reg || dut.region_base_reg != 32'h0000c000 ||
                dut.region_count_reg != 2 || configure_completion_count != completions_before + 1)
                $fatal(1, "pending configure did not activate and acknowledge after old region drain");
            wait (dut.returned_reg == 2);
            consume_stream(32'h0000c000, 2);
            wait_drained();

            // Capture a second configure while the last old-region load is pending.
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
            opcode = 2'b0;
            size_bits = 32'b0;
            byte_address = 32'b0;
            stall_ar = 1'b0;
            watchdog = 0;
            while (!done && watchdog < 1000)
            begin
                @(negedge clk);
                #1;
                watchdog = watchdog + 1;
            end
            if (watchdog == 1000)
                $fatal(1, "pending load failed to return before queued configure");
            start = 1'b1;
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = 32'h0000e000;
            count = 2;
            @(posedge clk); // E0 captures configure while the final old load retires.
            #1;
            if (dut.cfg_pending_reg || done || dut.consumed_reg != 1 ||
                configure_completion_count != completions_before)
                $fatal(1, "E0 configure/load-pop exposed activation before E1");
            @(negedge clk);
            start = 1'b0;
            opcode = 2'b0;
            size_bits = 32'b0;
            byte_address = 32'b0;
            count = 32'b0;
            watchdog = 0;
            while (configure_completion_count != completions_before + 1 && watchdog < 32)
            begin
                @(posedge clk);
                #1;
                watchdog = watchdog + 1;
            end
            if (watchdog == 32 || !dut.region_active_reg || dut.region_base_reg != 32'h0000e000 ||
                dut.cfg_pending_reg || configure_completion_count != completions_before + 1)
                $fatal(1, "E1 reconfigure failed after the final pending-load pop");
            wait (dut.returned_reg == 2);
            consume_stream(32'h0000e000, 2);
            wait_drained();
            $display("PASS B=%0d drain-edge and same-cycle-pop configure", B_MAX);
            $finish;
        end
        if ((selected_case == "ADJACENT_CFG") && $test$plusargs("ADJACENT_CFG"))
        begin
            integer completions_before;
            integer guard;
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
            @(posedge clk); // E0 captures the first empty descriptor.
            @(negedge clk);
            byte_address = 32'h00022000;
            count = 0;
            guard = 0;
            while (configure_completion_count < completions_before + 2 && guard < 24)
            begin
                @(posedge clk);
                #1;
                guard = guard + 1;
            end
            if (configure_completion_count < completions_before + 2)
                $fatal(1, "adjacent-cfg second configure never completed after E0/E1");
            if (configure_completion_count != completions_before + 2)
                $fatal(1, "adjacent-cfg completions = %0d, expected %0d",
                       configure_completion_count, completions_before + 2);

            // Same operands again with start still high: still one request, no new capture.
            @(posedge clk);
            repeat (2)
            begin
                @(negedge clk);
                if (fault)
                    $fatal(1, "adjacent-cfg identical held request faulted");
                @(posedge clk);
            end
            if (configure_completion_count != completions_before + 2)
                $fatal(1, "adjacent-cfg identical repeat produced an extra completion");
            @(negedge clk);
            start = 1'b0;
            $display("ADJACENT_CFG: back-to-back configures with held start accepted");
        end

        if ((selected_case == "ADJACENT_CFG_TRUNC") && $test$plusargs("ADJACENT_CFG_TRUNC"))
        begin
            // The nonzero high count bit must reach E1 validation after the
            // one-cycle E0 capture even though all live operands are then cleared.
            stall_ar = 1'b0;
            configure(32'h00023000, 0);
            configure_invalid(32'h00023000, 32'h80000000, 32'd32);
            if (ar_count != 0)
                $fatal(1, "adjacent-cfg-trunc: oversized count issued AXI traffic");
            $display("ADJACENT_CFG_TRUNC: oversized count reached validation and faulted");
            $finish;
        end

        if ((selected_case == "ADJACENT_CFG_SIZE") && $test$plusargs("ADJACENT_CFG_SIZE"))
        begin
            // A non-full-word size must be captured at E0 and rejected at E1
            // after the live input bus changes.
            stall_ar = 1'b0;
            configure(32'h00024000, 0);
            configure_invalid(32'h00024000, 1, 32'd16);
            if (ar_count != 0)
                $fatal(1, "adjacent-cfg-size: invalid size issued AXI traffic");
            $display("ADJACENT_CFG_SIZE: changed element size reached validation and faulted");
            $finish;
        end

        if ($test$plusargs("CONTINUOUS_CFG"))
        begin
            integer completions_before;
            integer descriptors_before;
            integer guard;
            stall_ar = 1'b0;
            completions_before = configure_completion_count;
            descriptors_before = ar_count;

            @(negedge clk);
            start = 1'b1;
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = 32'h00011000;
            count = 1;
            @(posedge clk); // E0 captures the request while start remains asserted.
            guard = 0;
            while (configure_completion_count < completions_before + 1 && guard < 16)
            begin
                @(posedge clk);
                #1;
                guard = guard + 1;
            end
            if (guard == 16 || fault || configure_completion_count != completions_before + 1)
                $fatal(1, "held configure did not receive one E1 acknowledgement");
            repeat (2)
            begin
                @(posedge clk);
                #1;
                if (fault || configure_completion_count != completions_before + 1)
                    $fatal(1, "held configure completed more than once");
            end

            // Transition to a scalar read without lowering start.
            @(negedge clk);
            opcode = 2'd0;
            size_bits = 32'd32;
            byte_address = 32'h00011000;
            #1;
            if (!done)
            begin
                @(posedge clk); // accept the pending scalar read
            end
            while (!done)
            begin
                @(negedge clk);
                #1;
            end
            if (data_out !== (32'h00011000 >> 2))
                $fatal(1, "continuous-sequence first load returned wrong data");
            @(posedge clk); // last pop retires the first region.

            // The opcode left configure, so the next held-start configure is new.
            @(negedge clk);
            opcode = 2'd2;
            size_bits = 32'd32;
            byte_address = 32'h00012000;
            count = 2;
            guard = 0;
            while (configure_completion_count < completions_before + 2 && guard < 24)
            begin
                @(posedge clk);
                #1;
                guard = guard + 1;
            end
            if (!dut.region_active_reg || dut.region_base_reg != 32'h00012000 ||
                dut.cfg_pending_reg || configure_completion_count != completions_before + 2 ||
                ar_count != descriptors_before + 1 || guard == 24)
                $fatal(1, "continuous configure sequence duplicated or lost a capture");
            repeat (2)
            begin
                @(posedge clk);
                #1;
                if (fault || configure_completion_count != completions_before + 2)
                    $fatal(1, "second held configure completed more than once");
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
        wait (dut.returned_reg == 38);
        consume_immediate_stream(32'h00006000, 38);
        wait_drained();
        check_descriptors(32'h00006000, 38, before_ar);

        // A second configure is retained until the current region drains.
        before_ar = ar_count;
        configure(32'h00007000, 4);
        wait (dut.returned_reg == 4);
        capture_busy_config(32'h00008000, 2);
        consume_stream(32'h00007000, 4);
        wait (deferred_cfg_done != 0);
        if (deferred_cfg_done == 0 || !dut.region_active_reg || dut.region_base_reg != 32'h00008000)
            $fatal(1, "pending configure was not captured after drain: done_count=%0d active=%b base=%h pending=%b count=%0d consumed=%0d returned=%0d",
                   deferred_cfg_done, dut.region_active_reg, dut.region_base_reg, dut.cfg_pending_reg,
                   dut.region_count_reg, dut.consumed_reg, dut.returned_reg);
        check_descriptors(32'h00007000, 4, before_ar);
        before_ar = ar_count;
        wait (dut.returned_reg == 2);
        consume_stream(32'h00008000, 2);
        wait_drained();
        check_descriptors(32'h00008000, 2, before_ar);

        $display("PASS B=%0d AR=%0d R=%0d", B_MAX, ar_count, r_beat_count);
        $finish;
    end

    initial
    begin
        #2000000;
        $fatal(1, "global test watchdog expired O=%0d D=%0d issued=%0d returned=%0d consumed=%0d desc=%0d fifo=%0d AR=%0d R=%0d fault=%b",
               MAX_OUTSTANDING, FIFO_DEPTH, dut.issued_reg, dut.returned_reg, dut.consumed_reg,
               dut.descriptor_count_reg, dut.fifo_count_reg, ar_count, r_beat_count, fault);
    end

    final
    begin
        $display("DUPLEX_READ PASS CASE=%s", selected_case);
    end
endmodule
