module tb_write_burst_engine #(
    parameter integer B_MAX = 16,
    parameter integer MAX_OUTSTANDING = 1,
    parameter integer FIFO_DEPTH = 32,
    parameter integer ADDR_BITS = 32,
    parameter integer ASYNC_RESET = 0,
    parameter integer RESET_ACTIVE_LOW = 1
);
    logic clock = 1'b0;
    always #5 clock = ~clock;

    localparam logic RESET_ASSERTED = RESET_ACTIVE_LOW ? 1'b0 : 1'b1;
    localparam logic RESET_DEASSERTED = ~RESET_ASSERTED;
    logic reset = RESET_ASSERTED;
    logic start = 1'b0;
    logic [1:0] in1 = 2'b0;
    logic [31:0] in2 = 32'd32;
    logic [31:0] in3 = 32'b0;
    logic [ADDR_BITS-1:0] in4 = '0;
    logic [31:0] in5 = 32'b0;
    wire done;
    wire [31:0] out1;
    wire fault;

    wire [ADDR_BITS-1:0] awaddr;
    wire [7:0] awlen;
    wire [2:0] awsize;
    wire [1:0] awburst;
    wire awid;
    wire awvalid;
    logic awready;
    wire [31:0] wdata;
    wire [3:0] wstrb;
    wire wlast;
    wire wvalid;
    logic wready = 1'b0;
    logic [1:0] bresp = 2'b0;
    logic bid = 1'b0;
    logic bvalid = 1'b0;
    logic hold_b_responses = 1'b0;
    wire bready;

    localparam logic [1:0] OP_STORE = 2'd1;
    localparam logic [1:0] OP_CONFIGURE = 2'd2;
    localparam logic [1:0] OP_DRAIN = 2'd3;
    localparam integer MEM_WORDS = 8192;
    localparam integer MAX_Q = 8192;

    MinimalAXI4MasterPipelinedWrite #(
        .B_MAX(B_MAX), .MAX_OUTSTANDING(MAX_OUTSTANDING), .FIFO_DEPTH(FIFO_DEPTH),
        .BITSIZE_in4(ADDR_BITS), .BITSIZE_m_axi_awaddr(ADDR_BITS)
    ) dut (
        .clock(clock), .reset(reset), .start(start), .in1(in1), .in2(in2), .in3(in3),
        .in4(in4), .in5(in5), .done(done), .out1(out1), .fault(fault),
        .m_axi_awaddr(awaddr), .m_axi_awlen(awlen), .m_axi_awsize(awsize),
        .m_axi_awburst(awburst), .m_axi_awid(awid), .m_axi_awvalid(awvalid),
        .m_axi_awready(awready), .m_axi_wdata(wdata), .m_axi_wstrb(wstrb),
        .m_axi_wlast(wlast), .m_axi_wvalid(wvalid), .m_axi_wready(wready),
        .m_axi_bresp(bresp), .m_axi_bid(bid), .m_axi_bvalid(bvalid),
        .m_axi_bready(bready)
    );

    logic [ADDR_BITS-1:0] aw_queue_addr [0:MAX_Q-1];
    integer aw_queue_len [0:MAX_Q-1];
    logic [31:0] w_queue_data [0:MAX_Q-1];
    logic w_queue_last [0:MAX_Q-1];
    logic [31:0] memory [0:MEM_WORDS-1];
    logic [31:0] expected [0:MEM_WORDS-1];
    logic expected_valid [0:MEM_WORDS-1];
    integer aw_wr = 0, aw_rd = 0, w_wr = 0, w_rd = 0;
    integer accepted_aw = 0, accepted_w = 0, committed_beats = 0, response_count = 0;
    integer reservations = 0, simultaneous_reserve_b = 0, simultaneous_enqueue_w = 0;
    integer before_aw_count;
    integer before_w_count;
    logic [ADDR_BITS+7:0] held_aw;
    logic [36:0] held_stalled_w;
    integer write_index;
    integer commit_count;
    integer response_delay = 0;
    integer response_delay_set = 0;
    logic wait_aw_for_w = 1'b0;
    logic prev_aw_stall = 1'b0;
    logic prev_w_stall = 1'b0;
    logic [ADDR_BITS+9:0] saved_aw;
    logic [36:0] saved_w;
    logic done_seen;
    logic fault_seen;
    integer timeout_count;
    integer test_case;
    integer requested_count;
    integer region_word_base;
    integer expected_burst_index = 0;
    logic check_38_split = 1'b0;
    integer i;
    string selected_case;

    logic awready_driver = 1'b0;
    wire effective_awready = awready_driver && (!wait_aw_for_w || wvalid);
    assign awready = effective_awready;

    always @(posedge clock) begin
        integer j;
        integer count_for_aw;
        if (reset == RESET_ASSERTED) begin
            aw_wr = 0;
            aw_rd = 0;
            w_wr = 0;
            w_rd = 0;
            accepted_aw = 0;
            accepted_w = 0;
            committed_beats = 0;
            response_count = 0;
            reservations = 0;
            simultaneous_reserve_b = 0;
            simultaneous_enqueue_w = 0;
            bvalid <= 1'b0;
            response_delay <= 0;
            prev_aw_stall <= 1'b0;
            prev_w_stall <= 1'b0;
        end else begin
            if (int'(dut.reserved_data_count) > int'(dut.fifo_count) ||
                int'(dut.fifo_count) > FIFO_DEPTH || int'(dut.descriptor_count) > MAX_OUTSTANDING)
                $fatal(1, "F2_CAPACITY_INVARIANT reserved=%0d fifo=%0d descriptors=%0d",
                       dut.reserved_data_count, dut.fifo_count, dut.descriptor_count);
            commit_count = 0;
            if (dut.reserve_beats != 0) reservations = reservations + 1;
            if ((dut.reserve_beats != 0) && bvalid && bready)
                simultaneous_reserve_b = simultaneous_reserve_b + 1;
            if (dut.store_enqueue && wvalid && wready)
                simultaneous_enqueue_w = simultaneous_enqueue_w + 1;
            if (out1 !== 32'b0)
                $fatal(1, "WRITE_OUT1_NOT_ZERO");
            if (awvalid && (awsize !== 3'd2 || awburst !== 2'b01 || awid !== 1'b0))
                $fatal(1, "AW_SIDEBAND_INVALID size=%0d burst=%b id=%b", awsize, awburst, awid);
            if (awvalid && effective_awready) begin
                if (awaddr[1:0] != 0 || (awaddr[11:0] + ((awlen + 1) << 2)) > 4096)
                    $fatal(1, "AW_CROSSES_4K_OR_UNALIGNED addr=%h len=%0d", awaddr, awlen);
                if ((awlen + 1) > B_MAX)
                    $fatal(1, "AW_EXCEEDS_B_MAX len=%0d B_MAX=%0d", awlen + 1, B_MAX);
                if (check_38_split) begin
                    case (expected_burst_index)
                        0: if (awlen + 1 != 16) $fatal(1, "COUNT38_SPLIT_0 len=%0d", awlen + 1);
                        1: if (awlen + 1 != 16) $fatal(1, "COUNT38_SPLIT_1 len=%0d", awlen + 1);
                        2: if (awlen + 1 != 6) $fatal(1, "COUNT38_SPLIT_2 len=%0d", awlen + 1);
                        default: $fatal(1, "COUNT38_EXTRA_BURST");
                    endcase
                    expected_burst_index = expected_burst_index + 1;
                end
            end
            if (wvalid && wstrb !== 4'hf)
                $fatal(1, "WSTRB_NOT_FULL");
            if (prev_aw_stall && (!awvalid || {awaddr, awlen} !== saved_aw))
                $fatal(1, "AW_PAYLOAD_CHANGED_DURING_STALL");
            if (prev_w_stall && (!wvalid || {wdata, wstrb, wlast} !== saved_w))
                $fatal(1, "W_PAYLOAD_CHANGED_DURING_STALL");
            prev_aw_stall <= awvalid && !effective_awready;
            prev_w_stall <= wvalid && !wready;
            if (awvalid && !effective_awready)
                saved_aw <= {awaddr, awlen};
            if (wvalid && !wready)
                saved_w <= {wdata, wstrb, wlast};

            if (awvalid && effective_awready) begin
                if (aw_wr >= MAX_Q)
                    $fatal(1, "AW_SCOREBOARD_OVERFLOW");
                aw_queue_addr[aw_wr] = awaddr;
                aw_queue_len[aw_wr] = awlen + 1;
                aw_wr = aw_wr + 1;
                accepted_aw = accepted_aw + 1;
            end
            if (wvalid && wready) begin
                if (w_wr >= MAX_Q)
                    $fatal(1, "W_SCOREBOARD_OVERFLOW");
                w_queue_data[w_wr] = wdata;
                w_queue_last[w_wr] = wlast;
                w_wr = w_wr + 1;
                accepted_w = accepted_w + 1;
            end

            if (!bvalid && aw_rd < aw_wr) begin
                count_for_aw = aw_queue_len[aw_rd];
                if (w_wr - w_rd >= count_for_aw) begin
                    for (j = 0; j < count_for_aw; j = j + 1) begin
                        if (w_queue_last[w_rd+j] !== (j == count_for_aw-1))
                            $fatal(1, "WLAST_MISMATCH burst=%0d beat=%0d", aw_rd, j);
                        write_index = (aw_queue_addr[aw_rd] >> 2) + j;
                        if (write_index < 0 || write_index >= MEM_WORDS)
                            $fatal(1, "SCOREBOARD_ADDRESS_OUT_OF_RANGE addr=%h", aw_queue_addr[aw_rd]);
                        if (!expected_valid[write_index] || expected[write_index] !== w_queue_data[w_rd+j])
                            $fatal(1, "WRITE_DATA_MISMATCH addr=%h got=%h expected=%h",
                                   aw_queue_addr[aw_rd] + j*4, w_queue_data[w_rd+j], expected[write_index]);
                        memory[write_index] = w_queue_data[w_rd+j];
                    end
                    w_rd = w_rd + count_for_aw;
                    aw_rd = aw_rd + 1;
                    committed_beats = committed_beats + count_for_aw;
                    response_delay <= response_delay_set;
                    commit_count = 1;
                end
            end
            if (!bvalid && !commit_count && response_delay > 0)
                response_delay <= response_delay - 1;
            else if (!hold_b_responses && !bvalid && !commit_count && response_delay == 0 && aw_rd > 0 &&
                     (committed_beats > 0) && response_count < aw_rd)
                bvalid <= 1'b1;
            if (bvalid && bready) begin
                bvalid <= response_count + 1 < aw_rd;
                response_count = response_count + 1;
            end
        end
    end

    task automatic tick(input integer cycles);
        repeat (cycles) @(negedge clock);
    endtask

    task automatic wait_for_done_or_fault(input integer max_cycles, input string label,
                                          output logic seen_done, output logic seen_fault);
        integer cycles;
        begin
            cycles = 0;
            seen_done = 1'b0;
            seen_fault = 1'b0;
            while (!seen_done && !seen_fault && cycles < max_cycles) begin
                #1;
                if (fault)
                    seen_fault = 1'b1;
                @(posedge clock);
                if (done)
                    seen_done = 1'b1;
                #1;
                if (fault)
                    seen_fault = 1'b1;
                @(negedge clock);
                cycles = cycles + 1;
            end
            if (cycles == max_cycles)
                $fatal(1, "DONE_OR_FAULT_TIMEOUT %s", label);
        end
    endtask

    task automatic command(input logic [1:0] opcode, input logic [31:0] size,
                           input logic [31:0] data, input logic [ADDR_BITS-1:0] address,
                           input logic [31:0] count, input logic expect_fault);
        integer cycles;
        logic seen_done;
        logic seen_fault;
        begin
            @(negedge clock);
            in1 = opcode; in2 = size; in3 = data; in4 = address; in5 = count; start = 1'b1;
            cycles = 0;
            seen_done = 1'b0;
            seen_fault = 1'b0;
            while (!seen_done && !seen_fault && cycles < 1000) begin
                #1;
                if (fault)
                    seen_fault = 1'b1;
                @(posedge clock);
                if (done)
                    seen_done = 1'b1;
                #1;
                if (fault)
                    seen_fault = 1'b1;
                @(negedge clock);
                cycles = cycles + 1;
            end
            if (cycles == 1000)
                $fatal(1, "COMMAND_TIMEOUT opcode=%0d", opcode);
            if (expect_fault && !seen_fault)
                $fatal(1, "EXPECTED_FAULT opcode=%0d", opcode);
            if (!expect_fault && seen_fault)
                $fatal(1, "UNEXPECTED_FAULT opcode=%0d", opcode);
            if (seen_done && !expect_fault) begin
                start = 1'b0;
                tick(1);
            end else begin
                start = 1'b0;
            end
        end
    endtask

    task automatic store_region(input logic [ADDR_BITS-1:0] base, input integer count,
                                input integer tag);
        integer n;
        integer addr_word;
        logic [31:0] value;
        integer aw_start;
        integer w_start;
        integer commit_start;
        integer response_start;
        integer expected_aw_delta;
        integer address_offset;
        integer page_beats;
        integer burst_beats;
        begin
            check_38_split = (B_MAX == 16 && base == 32'h0000_0600 && count == 38);
            expected_burst_index = 0;
            aw_start = accepted_aw;
            w_start = accepted_w;
            commit_start = committed_beats;
            response_start = response_count;
            expected_aw_delta = 0;
            address_offset = 0;
            while (address_offset < count) begin
                page_beats = 1024 - (((base + address_offset*4) & 32'h0000_0fff) >> 2);
                burst_beats = count - address_offset;
                if (burst_beats > B_MAX) burst_beats = B_MAX;
                if (burst_beats > page_beats) burst_beats = page_beats;
                address_offset = address_offset + burst_beats;
                expected_aw_delta = expected_aw_delta + 1;
            end
            command(OP_CONFIGURE, 32'd32, 32'b0, base, count, 1'b0);
            region_word_base = base >> 2;
            if (region_word_base > 0)
                memory[region_word_base-1] = 32'h5a5a_a5a5;
            if (region_word_base+count < MEM_WORDS)
                memory[region_word_base+count] = 32'h5a5a_a5a5;
            for (n = 0; n < count; n = n + 1) begin
                addr_word = region_word_base + n;
                value = 32'hc001_0000 ^ (tag << 8) ^ n;
                expected[addr_word] = value;
                expected_valid[addr_word] = 1'b1;
                command(OP_STORE, 32'd32, value, base + n*4, 32'hdead_beef, 1'b0);
            end
            command(OP_DRAIN, 32'hbad0_bad0, 32'hbad0_bad0, '1, 32'hbad0_bad0, 1'b0);
            timeout_count = 0;
            while ((response_count < accepted_aw || aw_rd < aw_wr || w_rd < w_wr) && timeout_count < 10000) begin
                @(negedge clock);
                timeout_count = timeout_count + 1;
            end
            if (timeout_count == 10000)
                $fatal(1, "TRANSPORT_DRAIN_TIMEOUT count=%0d", count);
            if (accepted_w-w_start != count || committed_beats-commit_start != count ||
                accepted_aw-aw_start != expected_aw_delta ||
                response_count-response_start != expected_aw_delta)
                $fatal(1, "REGION_EVENT_DELTA_MISMATCH count=%0d expected_aw=%0d aw=%0d w=%0d committed=%0d responses=%0d",
                       count, expected_aw_delta, accepted_aw-aw_start, accepted_w-w_start,
                       committed_beats-commit_start, response_count-response_start);
            check_38_split = 1'b0;
            if ((base >> 2) > 0 && memory[(base >> 2)-1] !== 32'h5a5a_a5a5)
                $fatal(1, "CANARY_BEFORE_REGION_CORRUPTED base=%h", base);
            if ((base >> 2)+count < MEM_WORDS && memory[(base >> 2)+count] !== 32'h5a5a_a5a5)
                $fatal(1, "CANARY_AFTER_REGION_CORRUPTED base=%h count=%0d", base, count);
            command(OP_DRAIN, 0, 0, '0, 0, 1'b0);
            command(OP_DRAIN, 0, 0, '0, 0, 1'b0);
            for (n = 0; n < count; n = n + 1) begin
                addr_word = region_word_base + n;
                if (memory[addr_word] !== expected[addr_word])
                    $fatal(1, "MEMORY_SCOREBOARD_MISMATCH word=%0d", addr_word);
            end
        end
    endtask

    task automatic multi_outstanding_witness;
        integer total_beats;
        integer address;
        integer start_aw;
        integer start_w;
        integer start_commit;
        integer start_b;
        integer start_reservations;
        integer start_sim_reserve_b;
        integer start_sim_enqueue_w;
        begin
            total_beats = (MAX_OUTSTANDING + 1) * B_MAX;
            start_aw = accepted_aw;
            start_w = accepted_w;
            start_commit = committed_beats;
            start_b = response_count;
            start_reservations = reservations;
            start_sim_reserve_b = simultaneous_reserve_b;
            start_sim_enqueue_w = simultaneous_enqueue_w;
            hold_b_responses = 1'b1;
            awready_driver = 1'b1;
            wready = 1'b1;
            command(OP_CONFIGURE, 32, 0, 32'h3000, total_beats, 1'b0);
            for (integer beat = 0; beat < total_beats; beat = beat + 1) begin
                address = 32'h3000 + beat * 4;
                expected[address/4] = 32'hf200_0000 ^ (MAX_OUTSTANDING << 12) ^ beat;
                expected_valid[address/4] = 1'b1;
                command(OP_STORE, 32, expected[address/4], address, 0, 1'b0);
            end
            tick(4);
            if (accepted_aw-start_aw < 2 || response_count != start_b)
                $fatal(1, "F2_TWO_AW_BEFORE_FIRST_B_REQUIRED O=%0d aw=%0d b=%0d",
                       MAX_OUTSTANDING, accepted_aw-start_aw, response_count-start_b);
            if (dut.descriptor_count != MAX_OUTSTANDING ||
                dut.scheduled != MAX_OUTSTANDING*B_MAX ||
                dut.fifo_count != B_MAX || dut.reserved_data_count != 0 ||
                accepted_aw-start_aw != MAX_OUTSTANDING ||
                accepted_w-start_w != MAX_OUTSTANDING*B_MAX ||
                committed_beats-start_commit != MAX_OUTSTANDING*B_MAX ||
                response_count != start_b || bvalid || done || fault)
                $fatal(1, "F2_FULL_QUEUE_SETUP O=%0d desc=%0d scheduled=%0d fifo=%0d reserved=%0d aw=%0d w=%0d commit=%0d b=%0d",
                       MAX_OUTSTANDING, dut.descriptor_count, dut.scheduled, dut.fifo_count,
                       dut.reserved_data_count, accepted_aw-start_aw, accepted_w-start_w,
                       committed_beats-start_commit, response_count-start_b);
            @(negedge clock); in1 = OP_DRAIN; in2 = 0; in3 = 0; in4 = 0; in5 = 0; start = 1'b1;
            tick(3);
            if (done || fault || !dut.drain_pending || response_count != start_b)
                $fatal(1, "F2_DRAIN_COMPLETED_BEFORE_B O=%0d", MAX_OUTSTANDING);
            start = 1'b0;
            hold_b_responses = 1'b0;
            timeout_count = 0;
            while (response_count-start_b != MAX_OUTSTANDING+1 && timeout_count < 4000) begin
                @(negedge clock);
                timeout_count = timeout_count + 1;
            end
            if (timeout_count == 4000)
                $fatal(1, "F2_FREED_CREDIT_NO_PROGRESS O=%0d desc=%0d fifo=%0d reserved=%0d",
                       MAX_OUTSTANDING, dut.descriptor_count, dut.fifo_count,
                       dut.reserved_data_count);
            wait_for_done_or_fault(1000, "F2_MULTI_DRAIN", done_seen, fault_seen);
            if (!done_seen || fault_seen || dut.region_active || fault ||
                accepted_aw-start_aw != MAX_OUTSTANDING+1 ||
                accepted_w-start_w != total_beats ||
                committed_beats-start_commit != total_beats || response_count-start_b != MAX_OUTSTANDING+1 ||
                reservations-start_reservations != MAX_OUTSTANDING+1 ||
                simultaneous_reserve_b == start_sim_reserve_b ||
                simultaneous_enqueue_w == start_sim_enqueue_w ||
                dut.descriptor_count != 0 || dut.fifo_count != 0 || dut.reserved_data_count != 0)
                $fatal(1, "F2_EVENT_DELTA_MISMATCH O=%0d reserve=%0d aw=%0d w=%0d commit=%0d b=%0d simultaneous_reserve_b=%0d simultaneous_enqueue_w=%0d desc=%0d fifo=%0d reserved=%0d",
                       MAX_OUTSTANDING, reservations-start_reservations, accepted_aw-start_aw,
                       accepted_w-start_w, committed_beats-start_commit, response_count-start_b,
                       simultaneous_reserve_b-start_sim_reserve_b,
                       simultaneous_enqueue_w-start_sim_enqueue_w,
                       dut.descriptor_count, dut.fifo_count, dut.reserved_data_count);
            start = 1'b0;
            for (integer burst = 0; burst <= MAX_OUTSTANDING; burst = burst + 1)
                if (aw_queue_addr[start_aw+burst] !== 32'h3000+burst*B_MAX*4 ||
                    aw_queue_len[start_aw+burst] != B_MAX)
                    $fatal(1, "F2_AW_DESCRIPTOR_MISMATCH O=%0d burst=%0d addr=%h len=%0d",
                           MAX_OUTSTANDING, burst, aw_queue_addr[start_aw+burst],
                           aw_queue_len[start_aw+burst]);
            for (integer check_beat = 0; check_beat < total_beats; check_beat = check_beat + 1)
                if (memory[32'h3000/4+check_beat] !== expected[32'h3000/4+check_beat])
                    $fatal(1, "F2_MEMORY_MISMATCH O=%0d beat=%0d", MAX_OUTSTANDING, check_beat);
        end
    endtask

    task automatic two_aw_before_b_witness;
        begin
            if (MAX_OUTSTANDING != 2)
                $fatal(1, "F2_TWO_AW_WITNESS_REQUIRES_O2");
            hold_b_responses = 1'b1;
            awready_driver = 1'b1;
            wready = 1'b1;
            command(OP_CONFIGURE, 32, 0, 32'h3c00, 2*B_MAX, 1'b0);
            for (integer beat = 0; beat < 2*B_MAX; beat = beat + 1) begin
                expected[32'h3c00/4+beat] = 32'hf230_0000 ^ beat;
                expected_valid[32'h3c00/4+beat] = 1'b1;
                command(OP_STORE, 32, expected[32'h3c00/4+beat], 32'h3c00+beat*4, 0, 1'b0);
            end
            tick(4);
            if (accepted_aw != 2 || response_count != 0 || dut.descriptor_count != 2)
                $fatal(1, "F2_TWO_AW_BEFORE_FIRST_B_REQUIRED aw=%0d b=%0d descriptors=%0d",
                       accepted_aw, response_count, dut.descriptor_count);
        end
    endtask

    task automatic independent_cursor_witness;
        logic [ADDR_BITS+7:0] saved_address;
        logic [36:0] saved_payload;
        begin
            if (MAX_OUTSTANDING != 2 || FIFO_DEPTH < 2*B_MAX)
                $fatal(1, "F2_CURSOR_REQUIRES_O2_FIFO2B");
            hold_b_responses = 1'b1;
            awready_driver = 1'b1;
            wready = 1'b0;
            command(OP_CONFIGURE, 32, 0, 32'h3400, 2*B_MAX, 1'b0);
            for (integer beat = 0; beat < 2*B_MAX; beat = beat + 1) begin
                expected[32'h3400/4+beat] = 32'hf210_0000 ^ beat;
                expected_valid[32'h3400/4+beat] = 1'b1;
                command(OP_STORE, 32, expected[32'h3400/4+beat], 32'h3400+beat*4, 0, 1'b0);
            end
            tick(3);
            if (accepted_aw != 2 || accepted_w != 0 ||
                dut.descriptor_count != 2 || !wvalid)
                $fatal(1, "F2_AW_CURSOR_DID_NOT_RUN_AHEAD aw=%0d w=%0d desc=%0d",
                       accepted_aw, accepted_w, dut.descriptor_count);
            saved_payload = {wdata, wstrb, wlast};
            repeat (3) begin
                @(negedge clock);
                if (!wvalid || {wdata, wstrb, wlast} !== saved_payload)
                    $fatal(1, "F2_W_PAYLOAD_NOT_STABLE_WHILE_STALLED");
            end
            reset_fixture();

            hold_b_responses = 1'b1;
            awready_driver = 1'b0;
            wready = 1'b1;
            command(OP_CONFIGURE, 32, 0, 32'h3600, B_MAX, 1'b0);
            for (integer beat = 0; beat < B_MAX; beat = beat + 1) begin
                expected[32'h3600/4+beat] = 32'hf215_0000 ^ beat;
                expected_valid[32'h3600/4+beat] = 1'b1;
                command(OP_STORE, 32, expected[32'h3600/4+beat], 32'h3600+beat*4, 0, 1'b0);
            end
            timeout_count = 0;
            while (accepted_w != B_MAX && timeout_count < 1000) begin
                @(negedge clock);
                timeout_count = timeout_count + 1;
            end
            if (timeout_count == 1000 || accepted_aw != 0 || !awvalid ||
                {awaddr, awlen} !== {32'h3600, 8'(B_MAX-1)} ||
                dut.descriptor_count != 1 || dut.fifo_count != 0 ||
                dut.reserved_data_count != 0)
                $fatal(1, "F2_W_BEFORE_AW_NOT_PROVEN w=%0d aw=%0d awvalid=%b desc=%0d fifo=%0d reserved=%0d",
                       accepted_w, accepted_aw, awvalid, dut.descriptor_count,
                       dut.fifo_count, dut.reserved_data_count);
            saved_address = {awaddr, awlen};
            repeat (3) begin
                @(negedge clock);
                if (!awvalid || {awaddr, awlen} !== saved_address || accepted_aw != 0)
                    $fatal(1, "F2_AW_PAYLOAD_NOT_STABLE_WHILE_W_RUNS");
            end
            awready_driver = 1'b1;
            timeout_count = 0;
            while (accepted_aw != 1 && timeout_count < 100) begin
                @(negedge clock);
                timeout_count = timeout_count + 1;
            end
            if (timeout_count == 100)
                $fatal(1, "F2_AW_DID_NOT_RESUME_AFTER_W");
            @(negedge clock); in1 = OP_DRAIN; in2 = 0; in3 = 0; in4 = 0; in5 = 0; start = 1'b1;
            tick(2);
            if (done || !dut.drain_pending) $fatal(1, "F2_CURSOR_DRAIN_EARLY");
            start = 1'b0;
            hold_b_responses = 1'b0;
            wait_for_done_or_fault(2000, "F2_CURSOR_DRAIN", done_seen, fault_seen);
            if (!done_seen || fault_seen || accepted_aw != 1 ||
                accepted_w != B_MAX || committed_beats != B_MAX ||
                response_count != 1 || dut.descriptor_count != 0 ||
                memory[32'h3600/4] !== expected[32'h3600/4] ||
                memory[32'h3600/4+B_MAX-1] !== expected[32'h3600/4+B_MAX-1])
                $fatal(1, "F2_CURSOR_FINAL_MISMATCH");
        end
    endtask

    task automatic bresp_multi_witness;
        integer before_accepted;
        integer before_scheduled;
        begin
            if (MAX_OUTSTANDING != 2 || FIFO_DEPTH < 2*B_MAX)
                $fatal(1, "F2_BRESP_REQUIRES_O2_FIFO2B");
            hold_b_responses = 1'b1;
            awready_driver = 1'b1;
            wready = 1'b1;
            command(OP_CONFIGURE, 32, 0, 32'h3800, 4*B_MAX, 1'b0);
            for (integer beat = 0; beat < B_MAX; beat = beat + 1) begin
                expected[32'h3800/4+beat] = 32'hf220_0000 ^ beat;
                expected_valid[32'h3800/4+beat] = 1'b1;
                command(OP_STORE, 32, expected[32'h3800/4+beat], 32'h3800+beat*4, 0, 1'b0);
            end
            timeout_count = 0;
            while ((accepted_aw != 1 || accepted_w != B_MAX || committed_beats != B_MAX) &&
                   timeout_count < 1000) begin
                @(negedge clock);
                timeout_count = timeout_count + 1;
            end
            if (timeout_count == 1000 || response_count != 0 || bvalid ||
                dut.descriptor_count != 1 || dut.scheduled != B_MAX || dut.fifo_count != 0)
                $fatal(1, "F2_BRESP_FIRST_DESCRIPTOR_SETUP aw=%0d w=%0d commit=%0d b=%0d desc=%0d",
                       accepted_aw, accepted_w, committed_beats, response_count,
                       dut.descriptor_count);

            wready = 1'b0;
            for (integer beat = B_MAX; beat < 3*B_MAX; beat = beat + 1) begin
                expected[32'h3800/4+beat] = 32'hf220_0000 ^ beat;
                expected_valid[32'h3800/4+beat] = 1'b1;
                command(OP_STORE, 32, expected[32'h3800/4+beat], 32'h3800+beat*4, 0, 1'b0);
            end
            if (accepted_aw != 2 || accepted_w != B_MAX || dut.descriptor_count != 2 ||
                dut.scheduled != 2*B_MAX || dut.fifo_count != 2*B_MAX ||
                dut.reserved_data_count != B_MAX || !wvalid || awvalid ||
                response_count != 0 || bvalid)
                $fatal(1, "F2_BRESP_STALLED_SECOND_SETUP aw=%0d w=%0d desc=%0d scheduled=%0d fifo=%0d reserved=%0d",
                       accepted_aw, accepted_w, dut.descriptor_count, dut.scheduled,
                       dut.fifo_count, dut.reserved_data_count);
            bresp = 2'b10;
            hold_b_responses = 1'b0;
            timeout_count = 0;
            while (!fault && timeout_count < 200) begin
                @(negedge clock);
                timeout_count = timeout_count + 1;
            end
            if (timeout_count == 200 || !fault || done || response_count != 1 ||
                accepted_aw != 2 || accepted_w != B_MAX || !wvalid ||
                dut.descriptor_count != 1 || dut.scheduled != 2*B_MAX ||
                dut.fifo_count != 2*B_MAX || dut.reserved_data_count != B_MAX || awvalid)
                $fatal(1, "F2_BRESP_STICKY_FAULT_NOT_RAISED aw=%0d w=%0d b=%0d desc=%0d fifo=%0d reserved=%0d",
                       accepted_aw, accepted_w, response_count, dut.descriptor_count,
                       dut.fifo_count, dut.reserved_data_count);
            before_accepted = dut.accepted;
            before_scheduled = dut.scheduled;
            @(negedge clock); in1 = OP_STORE; in2 = 32; in3 = 32'hdead_f2b0;
            in4 = 32'h3800+3*B_MAX*4; in5 = 0; start = 1'b1;
            tick(3);
            if (!fault || done || dut.accepted != before_accepted ||
                dut.scheduled != before_scheduled || accepted_aw != 2 || accepted_w != B_MAX ||
                response_count != 1 || dut.descriptor_count != 1 || !wvalid || awvalid ||
                dut.fifo_count != 2*B_MAX || dut.reserved_data_count != B_MAX)
                $fatal(1, "F2_BRESP_FAULT_DID_NOT_HOLD_EXPOSED_DESCRIPTOR accepted=%0d scheduled=%0d aw=%0d w=%0d b=%0d descriptors=%0d fifo=%0d reserved=%0d",
                       dut.accepted, dut.scheduled, accepted_aw, accepted_w,
                       response_count, dut.descriptor_count, dut.fifo_count,
                       dut.reserved_data_count);
            start = 1'b0;
            bresp = 2'b00;
            wready = 1'b1;
            timeout_count = 0;
            while ((accepted_w != 2*B_MAX || response_count != 2) && timeout_count < 1000) begin
                @(negedge clock);
                timeout_count = timeout_count + 1;
            end
            if (timeout_count == 1000 || !fault || done || accepted_aw != 2 ||
                accepted_w != 2*B_MAX || committed_beats != 2*B_MAX || response_count != 2 ||
                dut.descriptor_count != 0 || dut.scheduled != 2*B_MAX ||
                dut.fifo_count != B_MAX || dut.reserved_data_count != 0 || awvalid || wvalid)
                $fatal(1, "F2_BRESP_TRANSPORT_DID_NOT_FINISH accepted=%0d scheduled=%0d aw=%0d w=%0d commit=%0d b=%0d desc=%0d fifo=%0d reserved=%0d",
                       dut.accepted, dut.scheduled, accepted_aw, accepted_w, committed_beats,
                       response_count, dut.descriptor_count, dut.fifo_count,
                       dut.reserved_data_count);
            for (integer beat = 0; beat < 2*B_MAX; beat = beat + 1)
                if (memory[32'h3800/4+beat] !== expected[32'h3800/4+beat])
                    $fatal(1, "F2_BRESP_EXPOSED_DATA_MISMATCH beat=%0d", beat);
            if (memory[32'h3800/4+2*B_MAX] !== 32'h5a5a_a5a5)
                $fatal(1, "F2_BRESP_UNASSIGNED_DATA_WAS_WRITTEN");
            if (dut.accepted != before_accepted || dut.scheduled != before_scheduled)
                $fatal(1, "F2_BRESP_ACCEPTED_STORE_AFTER_FAULT");
            @(negedge clock); in1 = OP_DRAIN; in2 = 0; in3 = 0; in4 = 0; in5 = 0; start = 1'b1;
            tick(3);
            if (!fault || done) $fatal(1, "F2_BRESP_DRAIN_REPORTED_SUCCESS");
            start = 1'b0;
        end
    endtask

    task automatic bresp_interleaving_witness(input integer mode);
        integer beat;
        integer expected_aw_count;
        integer expected_w_count;
        begin
            if (MAX_OUTSTANDING != 2 || FIFO_DEPTH < 2*B_MAX || B_MAX != 16)
                $fatal(1, "F3_BRESP_INTERLEAVING_REQUIRES_O2_FIFO32_B16");
            hold_b_responses = 1'b1;
            awready_driver = 1'b1;
            wready = 1'b1;
            command(OP_CONFIGURE, 32, 0, 32'h3a00, 4*B_MAX, 1'b0);
            for (beat = 0; beat < B_MAX; beat = beat + 1) begin
                expected[32'h3a00/4+beat] = 32'hf330_0000 ^ beat;
                expected_valid[32'h3a00/4+beat] = 1'b1;
                command(OP_STORE, 32, expected[32'h3a00/4+beat], 32'h3a00+beat*4, 0, 1'b0);
            end
            timeout_count = 0;
            while ((accepted_aw != 1 || accepted_w != B_MAX || committed_beats != B_MAX) &&
                   timeout_count < 1000) begin
                @(negedge clock);
                timeout_count = timeout_count + 1;
            end
            if (timeout_count == 1000 || response_count != 0 || bvalid ||
                dut.descriptor_count != 1 || dut.scheduled != B_MAX || dut.fifo_count != 0)
                $fatal(1, "F3_BRESP_FIRST_DESCRIPTOR_SETUP aw=%0d w=%0d commit=%0d b=%0d desc=%0d",
                       accepted_aw, accepted_w, committed_beats, response_count,
                       dut.descriptor_count);

            awready_driver = 1'b0;
            wready = (mode == 0) ? 1'b0 : 1'b1;
            for (beat = B_MAX; beat < 3*B_MAX; beat = beat + 1) begin
                expected[32'h3a00/4+beat] = 32'hf330_0000 ^ beat;
                expected_valid[32'h3a00/4+beat] = 1'b1;
                command(OP_STORE, 32, expected[32'h3a00/4+beat], 32'h3a00+beat*4, 0, 1'b0);
            end
            timeout_count = 0;
            if (mode == 0) begin
                while ((!awvalid || !wvalid || dut.descriptor_count != 2) && timeout_count < 200) begin
                    @(negedge clock);
                    timeout_count = timeout_count + 1;
                end
                if (timeout_count == 200 || accepted_aw != 1 || accepted_w != B_MAX ||
                    dut.scheduled != 2*B_MAX || dut.fifo_count != 2*B_MAX ||
                    dut.reserved_data_count != B_MAX || effective_awready || wready)
                    $fatal(1, "F3_BRESP_BOTH_STALLED_PRECONDITION aw=%0d w=%0d desc=%0d fifo=%0d reserved=%0d",
                           accepted_aw, accepted_w, dut.descriptor_count,
                           dut.fifo_count, dut.reserved_data_count);
            end else begin
                while ((accepted_w != 2*B_MAX || !awvalid || dut.descriptor_count != 2) &&
                       timeout_count < 200) begin
                    @(negedge clock);
                    timeout_count = timeout_count + 1;
                end
                if (timeout_count == 200 || accepted_aw != 1 || accepted_w != 2*B_MAX ||
                    dut.scheduled != 2*B_MAX || dut.fifo_count != B_MAX ||
                    dut.reserved_data_count != 0 || !awvalid || wvalid || effective_awready)
                    $fatal(1, "F3_BRESP_W_BEFORE_AW_PRECONDITION aw=%0d w=%0d desc=%0d fifo=%0d reserved=%0d awvalid=%b wvalid=%b",
                           accepted_aw, accepted_w, dut.descriptor_count, dut.fifo_count,
                           dut.reserved_data_count, awvalid, wvalid);
            end

            bresp = 2'b10;
            hold_b_responses = 1'b0;
            timeout_count = 0;
            while (!fault && timeout_count < 200) begin
                @(negedge clock);
                timeout_count = timeout_count + 1;
            end
            expected_w_count = (mode == 0) ? B_MAX : 2*B_MAX;
            if (timeout_count == 200 || !fault || done || response_count != 1 ||
                accepted_aw != 1 || accepted_w != expected_w_count || awvalid != 1'b1 ||
                (mode == 0 && !wvalid) || (mode == 1 && wvalid) ||
                dut.descriptor_count != 1 || dut.scheduled != 2*B_MAX ||
                dut.fifo_count != ((mode == 0) ? 2*B_MAX : B_MAX) ||
                dut.reserved_data_count != ((mode == 0) ? B_MAX : 0))
                $fatal(1, "F3_BRESP_FAULT_EDGE mode=%0d aw=%0d w=%0d b=%0d desc=%0d fifo=%0d reserved=%0d awvalid=%b wvalid=%b",
                       mode, accepted_aw, accepted_w, response_count, dut.descriptor_count,
                       dut.fifo_count, dut.reserved_data_count, awvalid, wvalid);

            held_aw = {awaddr, awlen};
            if (mode == 0) held_stalled_w = {wdata, wstrb, wlast};
            tick(3);
            if (!fault || !awvalid || {awaddr, awlen} !== held_aw ||
                (mode == 0 && (!wvalid || {wdata, wstrb, wlast} !== held_stalled_w)) ||
                (mode == 1 && wvalid) || accepted_aw != 1 || accepted_w != expected_w_count ||
                dut.descriptor_count != 1 || dut.fifo_count != ((mode == 0) ? 2*B_MAX : B_MAX) ||
                dut.reserved_data_count != ((mode == 0) ? B_MAX : 0))
                $fatal(1, "F3_BRESP_FAULT_DID_NOT_PRESERVE_SECOND_DESCRIPTOR mode=%0d", mode);

            bresp = 2'b00;
            awready_driver = 1'b1;
            wready = 1'b1;
            timeout_count = 0;
            while ((accepted_aw != 2 || accepted_w != 2*B_MAX || response_count != 2) &&
                   timeout_count < 1000) begin
                @(negedge clock);
                timeout_count = timeout_count + 1;
            end
            expected_aw_count = 2;
            if (timeout_count == 1000 || !fault || done || accepted_aw != expected_aw_count ||
                accepted_w != 2*B_MAX || committed_beats != 2*B_MAX || response_count != 2 ||
                dut.descriptor_count != 0 || dut.scheduled != 2*B_MAX ||
                dut.fifo_count != B_MAX || dut.reserved_data_count != 0 || awvalid || wvalid)
                $fatal(1, "F3_BRESP_RESERVED_TRANSPORT_DID_NOT_FINISH mode=%0d aw=%0d w=%0d commit=%0d b=%0d",
                       mode, accepted_aw, accepted_w, committed_beats, response_count);
            for (beat = 0; beat < 2*B_MAX; beat = beat + 1)
                if (memory[32'h3a00/4+beat] !== expected[32'h3a00/4+beat])
                    $fatal(1, "F3_BRESP_RESERVED_DATA_MISMATCH mode=%0d beat=%0d", mode, beat);
            if (memory[32'h3a00/4+2*B_MAX] !== 32'h5a5a_a5a5)
                $fatal(1, "F3_BRESP_UNASSIGNED_BURST_WRITTEN mode=%0d", mode);
            @(negedge clock); in1 = OP_DRAIN; in2 = 0; in3 = 0; in4 = 0; in5 = 0; start = 1'b1;
            tick(3);
            if (!fault || done || accepted_aw != 2 || accepted_w != 2*B_MAX)
                $fatal(1, "F3_BRESP_DRAIN_REPORTED_SUCCESS mode=%0d", mode);
            start = 1'b0;
        end
    endtask

    task automatic premature_configure_b_pending_witness;
        integer beat;
        begin
            hold_b_responses = 1'b1;
            awready_driver = 1'b1;
            wready = 1'b1;
            command(OP_CONFIGURE, 32, 0, 32'h3c00, B_MAX, 1'b0);
            for (beat = 0; beat < B_MAX; beat = beat + 1) begin
                expected[32'h3c00/4+beat] = 32'hf440_0000 ^ beat;
                expected_valid[32'h3c00/4+beat] = 1'b1;
                command(OP_STORE, 32, expected[32'h3c00/4+beat], 32'h3c00+beat*4, 0, 1'b0);
            end
            timeout_count = 0;
            while ((committed_beats != B_MAX || accepted_aw != 1 || accepted_w != B_MAX) &&
                   timeout_count < 1000) begin
                @(negedge clock);
                timeout_count = timeout_count + 1;
            end
            if (timeout_count == 1000 || response_count != 0 || bvalid ||
                !dut.region_active || dut.region_base !== 32'h3c00 ||
                dut.region_count != B_MAX || dut.accepted != B_MAX ||
                dut.scheduled != B_MAX || dut.completed != 0 || awvalid || wvalid ||
                dut.descriptor_count != 1)
                $fatal(1, "F3_PREMATURE_CONFIGURE_B_PENDING_SETUP_FAILED");
            command(OP_CONFIGURE, 32, 0, 32'h3d00, B_MAX, 1'b1);
            if (!fault || done || !dut.region_active || dut.region_base !== 32'h3c00 ||
                dut.region_count != B_MAX || dut.accepted != B_MAX || dut.scheduled != B_MAX ||
                dut.completed != 0 || accepted_aw != 1 || accepted_w != B_MAX ||
                committed_beats != B_MAX || response_count != 0 || awvalid || wvalid)
                $fatal(1, "F3_PREMATURE_CONFIGURE_B_PENDING_CHANGED_CONTEXT");
            hold_b_responses = 1'b0;
            timeout_count = 0;
            while (response_count != 1 && timeout_count < 200) begin
                @(negedge clock);
                timeout_count = timeout_count + 1;
            end
            if (timeout_count == 200 || !fault || done || response_count != 1 ||
                dut.region_base !== 32'h3c00 || dut.region_count != B_MAX ||
                dut.completed != B_MAX)
                $fatal(1, "F3_PREMATURE_CONFIGURE_B_PENDING_RETIREMENT_FAILED");
            for (beat = 0; beat < B_MAX; beat = beat + 1)
                if (memory[32'h3c00/4+beat] !== expected[32'h3c00/4+beat])
                    $fatal(1, "F3_PREMATURE_CONFIGURE_B_PENDING_DATA_MISMATCH beat=%0d", beat);
            @(negedge clock); in1 = OP_DRAIN; in2 = 0; in3 = 0; in4 = 0; in5 = 0; start = 1'b1;
            tick(3);
            if (!fault || done || response_count != 1)
                $fatal(1, "F3_PREMATURE_CONFIGURE_B_PENDING_DRAIN_SUCCEEDED");
            start = 1'b0;
        end
    endtask

    task automatic multipage_witness;
        integer page0;
        integer page1;
        integer page2;
        integer aw_index;
        begin
            awready_driver = 1'b1;
            wready = 1'b1;
            store_region(32'h0000_0ff0, 1050, 15);
            page0 = 0;
            page1 = 0;
            page2 = 0;
            for (aw_index = 0; aw_index < accepted_aw; aw_index = aw_index + 1) begin
                case (aw_queue_addr[aw_index] >> 12)
                    0: page0 = page0 + 1;
                    1: page1 = page1 + 1;
                    2: page2 = page2 + 1;
                    default: $fatal(1, "F3_MULTIPAGE_UNEXPECTED_PAGE addr=%h", aw_queue_addr[aw_index]);
                endcase
            end
            if (accepted_aw != 67 || accepted_w != 1050 || committed_beats != 1050 ||
                response_count != 67 || page0 != 1 || page1 != 64 || page2 != 2)
                $fatal(1, "F3_MULTIPAGE_COUNTS aw=%0d w=%0d committed=%0d b=%0d pages=%0d/%0d/%0d",
                       accepted_aw, accepted_w, committed_beats, response_count,
                       page0, page1, page2);
        end
    endtask

    task automatic prepare_pending_store(input logic [ADDR_BITS-1:0] base,
                                         input integer count, input integer tag);
        integer n;
        logic [31:0] pending_value;
        begin
            awready_driver = 1'b0;
            wready = 1'b0;
            command(OP_CONFIGURE, 32, 0, base, count, 1'b0);
            for (n = 0; n < FIFO_DEPTH; n = n + 1) begin
                expected[(base >> 2)+n] = 32'hd000_0000 ^ (tag << 8) ^ n;
                expected_valid[(base >> 2)+n] = 1'b1;
                command(OP_STORE, 32, expected[(base >> 2)+n], base+n*4, 0, 1'b0);
            end
            pending_value = 32'he000_0000 ^ (tag << 8) ^ FIFO_DEPTH;
            expected[(base >> 2)+FIFO_DEPTH] = pending_value;
            expected_valid[(base >> 2)+FIFO_DEPTH] = 1'b1;
            @(negedge clock);
            in1 = OP_STORE;
            in2 = 32;
            in3 = pending_value;
            in4 = base + FIFO_DEPTH*4;
            in5 = 0;
            start = 1'b1;
            tick(4);
            if (done || fault || !dut.store_pending || dut.accepted != FIFO_DEPTH)
                $fatal(1, "PENDING_STORE_SETUP_FAILED accepted=%0d pending=%b",
                       dut.accepted, dut.store_pending);
        end
    endtask

    task automatic reset_fixture;
        integer n;
        begin
            reset = RESET_ASSERTED; start = 1'b0; in1 = 0; in2 = 32; in3 = 0; in4 = 0; in5 = 0;
            awready_driver = 1'b0; wait_aw_for_w = 1'b0; wready = 1'b0;
            bresp = 0; bid = 0; bvalid = 0; response_delay_set = 0; hold_b_responses = 1'b0;
            tick(3);
            for (n = 0; n < MEM_WORDS; n = n + 1) begin
                memory[n] = 32'h5a5a_a5a5;
                expected[n] = 32'h5a5a_a5a5;
                expected_valid[n] = 1'b1;
            end
            reset = RESET_DEASSERTED;
            tick(2);
        end
    endtask

    task automatic asynchronous_reset_witness;
        integer n;
        begin
            if (!ASYNC_RESET)
                $fatal(1, "ASYNC_RESET_CASE_REQUIRES_ASYNC_VARIANT");
            awready_driver = 1'b0;
            wready = 1'b0;
            command(OP_CONFIGURE, 32, 0, 32'h2800, 32, 1'b0);
            for (n = 0; n < 32; n = n + 1)
                command(OP_STORE, 32, 32'hd000_0000+n, 32'h2800+n*4, 0, 1'b0);
            timeout_count = 0;
            while ((dut.descriptor_count != 2 || !awvalid || !wvalid) && timeout_count < 100) begin
                @(negedge clock);
                timeout_count = timeout_count + 1;
            end
            if (timeout_count == 100 || !dut.region_active || dut.descriptor_count != 2 ||
                !awvalid || !wvalid || awready || wready || dut.fifo_count != 32 ||
                dut.reserved_data_count != 32)
                $fatal(1, "ASYNC_RESET_SETUP_FAILED active=%b descriptors=%0d aw=%b w=%b fifo=%0d reserved=%0d",
                       dut.region_active, dut.descriptor_count, awvalid, wvalid,
                       dut.fifo_count, dut.reserved_data_count);
            @(negedge clock);
            in1 = OP_CONFIGURE; in2 = 32; in4 = 32'h2800; in5 = 32; start = 1'b1;
            @(posedge clock);
            #1;
            if (!fault || !awvalid || !wvalid || dut.descriptor_count != 2 ||
                !dut.region_active || dut.accepted != 32)
                $fatal(1, "ASYNC_RESET_FAULT_SETUP_FAILED fault=%b aw=%b w=%b descriptors=%0d region=%b accepted=%0d",
                       fault, awvalid, wvalid, dut.descriptor_count, dut.region_active, dut.accepted);
            start = 1'b0;
            #1 reset = RESET_ASSERTED;
            #1;
            if (fault || awvalid || wvalid || dut.descriptor_count != 0 || dut.region_active ||
                dut.store_pending || dut.accepted != 0 || dut.scheduled != 0 ||
                dut.completed != 0 || dut.fifo_count != 0 || dut.reserved_data_count != 0 ||
                dut.fifo_rd_ptr != 0 || dut.fifo_wr_ptr != 0 ||
                dut.descriptor_alloc_ptr != 0 || dut.descriptor_aw_ptr != 0 ||
                dut.descriptor_w_ptr != 0 || dut.descriptor_b_ptr != 0)
                $fatal(1, "ASYNC_RESET_NOT_IMMEDIATE fault=%b aw=%b w=%b descriptors=%0d region=%b pending=%b accepted=%0d scheduled=%0d completed=%0d fifo=%0d",
                       fault, awvalid, wvalid, dut.descriptor_count, dut.region_active,
                       dut.store_pending, dut.accepted, dut.scheduled, dut.completed,
                       dut.fifo_count);
            @(posedge clock);
            @(negedge clock);
            reset = RESET_DEASSERTED;
            tick(2);
            awready_driver = 1'b1;
            wready = 1'b1;
            expected[32'h2900/4] = 32'hface_cafe;
            expected_valid[32'h2900/4] = 1'b1;
            command(OP_CONFIGURE, 32, 0, 32'h2900, 1, 1'b0);
            command(OP_STORE, 32, expected[32'h2900/4], 32'h2900, 0, 1'b0);
            command(OP_DRAIN, 0, 0, 0, 0, 1'b0);
            timeout_count = 0;
            while (response_count < accepted_aw && timeout_count < 100) begin
                @(negedge clock);
                timeout_count = timeout_count + 1;
            end
            if (timeout_count == 100 || fault || dut.region_active || accepted_aw != 1 ||
                accepted_w != 1 || committed_beats != 1 ||
                memory[32'h2900/4] !== expected[32'h2900/4])
                $fatal(1, "ASYNC_RESET_RECOVERY_FAILED fault=%b region=%b aw=%0d w=%0d committed=%0d",
                       fault, dut.region_active, accepted_aw, accepted_w, committed_beats);
        end
    endtask

    task automatic expect_configure_change_fault(input integer changed_operand);
        logic [ADDR_BITS-1:0] original_base;
        logic [31:0] original_count;
        begin
            original_base = 32'h2200;
            original_count = 16;
            @(negedge clock);
            in1 = OP_CONFIGURE; in2 = 32; in3 = 0;
            in4 = original_base; in5 = original_count; start = 1'b1;
            #1;
            if (!done || fault) $fatal(1, "CONFIGURE_CHANGE_SETUP_NOT_VALID");
            @(posedge clock);
            #1;
            if (fault || !dut.region_active || dut.region_base !== original_base ||
                dut.region_count !== original_count)
                $fatal(1, "CONFIGURE_CHANGE_INITIAL_CAPTURE_FAILED");
            case (changed_operand)
                0: in2 = 32'h8000_0020;
                1: in4 = in4 | ({{(ADDR_BITS-1){1'b0}}, 1'b1} << 40);
                2: in5 = 32'h8000_0010;
                default: $fatal(1, "BAD_CONFIGURE_CHANGE_OPERAND %0d", changed_operand);
            endcase
            #1;
            if (done) $fatal(1, "CONFIGURE_CHANGE_POSITIVE_DONE operand=%0d", changed_operand);
            repeat (2) begin
                @(posedge clock);
                #1;
            end
            if (!fault || done || !dut.region_active ||
                dut.region_base !== original_base || dut.region_count !== original_count)
                $fatal(1, "CONFIGURE_CHANGE_NOT_REJECTED operand=%0d fault=%b done=%b active=%b base=%h count=%0d",
                       changed_operand, fault, done, dut.region_active,
                       dut.region_base, dut.region_count);
            @(negedge clock);
            start = 1'b0;
        end
    endtask

    initial begin
        if (!$value$plusargs("CASE=%s", selected_case))
            selected_case = "CORE";
        reset_fixture();
        if (selected_case == "PARAM") begin
            tick(2);
            $display("WRITE_BURST_ENGINE PARAM PASS B=%0d O=%0d FIFO=%0d", B_MAX, MAX_OUTSTANDING, FIFO_DEPTH);
            $finish;
        end
        else if (selected_case == "CFG_SIZE_CHANGE") begin
            expect_configure_change_fault(0);
        end else if (selected_case == "CFG_BASE_CHANGE") begin
            expect_configure_change_fault(1);
        end else if (selected_case == "CFG_COUNT_CHANGE") begin
            expect_configure_change_fault(2);
        end
        else if (selected_case == "INIT_DRAIN") begin
            command(OP_DRAIN, 32'hffffffff, 32'hffffffff, '1, 32'hffffffff, 1'b0);
        end else if (selected_case == "ASYNC_RESET") begin
            asynchronous_reset_witness();
        end else if (selected_case == "MULTI_OUTSTANDING") begin
            if (MAX_OUTSTANDING < 2)
                $fatal(1, "F2_MULTI_OUTSTANDING_REQUIRES_O_GT_1");
            multi_outstanding_witness();
        end else if (selected_case == "TWO_AW_BEFORE_B") begin
            two_aw_before_b_witness();
        end else if (selected_case == "INDEPENDENT_CURSORS") begin
            independent_cursor_witness();
        end else if (selected_case == "BRESP_MULTI") begin
            bresp_multi_witness();
        end else if (selected_case == "BRESP_AW_W_HELD") begin
            bresp_interleaving_witness(0);
        end else if (selected_case == "BRESP_W_BEFORE_AW") begin
            bresp_interleaving_witness(1);
        end else if (selected_case == "PREMATURE_CONFIGURE_B_PENDING") begin
            premature_configure_b_pending_witness();
        end else if (selected_case == "MULTIPAGE") begin
            if (ADDR_BITS != 32 || B_MAX != 16 || MAX_OUTSTANDING != 1)
                $fatal(1, "F3_MULTIPAGE_REQUIRES_A32_B16_O1");
            multipage_witness();
        end else if (selected_case == "CORE") begin
            awready_driver = 1'b1; wready = 1'b1;
            command(OP_DRAIN, 0, 0, 0, 0, 1'b0);
            store_region(32'h0000_0100, 0, 1);
            store_region(32'h0000_0200, 1, 2);
            store_region(32'h0000_0300, 15, 3);
            store_region(32'h0000_0400, 16, 4);
            store_region(32'h0000_0500, 17, 5);
            store_region(32'h0000_0600, 38, 6);
            store_region(32'h0000_0fe0, 16, 7);
            store_region(32'h0000_0ff0, 16, 8);
            store_region(32'h0000_0ff8, 16, 9);
            store_region(32'h0000_0200, 1, 99);
            if (out1 !== 0)
                $fatal(1, "WRITE_OUT1_NOT_ZERO");
        end else if (selected_case == "W_BEFORE_AW") begin
            awready_driver = 1'b0; wready = 1'b1;
            before_aw_count = accepted_aw;
            before_w_count = accepted_w;
            command(OP_CONFIGURE, 32, 0, 32'h800, 16, 1'b0);
            for (i = 0; i < 16; i = i + 1) begin
                expected[32'h800/4+i] = 32'hc001_0000 ^ (10 << 8) ^ i;
                expected_valid[32'h800/4+i] = 1'b1;
                command(OP_STORE, 32, expected[32'h800/4+i], 32'h800+i*4, 0, 1'b0);
            end
            tick(1);
            if (!awvalid || !wvalid || effective_awready ||
                accepted_w <= before_w_count || accepted_aw != before_aw_count)
                $fatal(1, "W_BEFORE_AW_HANDSHAKE_ORDER_INVALID aw=%0d w=%0d",
                       accepted_aw-before_aw_count, accepted_w-before_w_count);
            held_aw = {awaddr, awlen};
            tick(3);
            if (!awvalid || !wvalid || {awaddr, awlen} !== held_aw ||
                accepted_aw != before_aw_count)
                $fatal(1, "W_BEFORE_AW_PAYLOAD_OR_VALID_NOT_HELD");
            wait_aw_for_w = 1'b1;
            awready_driver = 1'b1;
            #1;
            if (!wvalid || effective_awready !== 1'b1)
                $fatal(1, "AWREADY_DID_NOT_WAIT_FOR_WVALID");
            command(OP_DRAIN, 0, 0, 0, 0, 1'b0);
            timeout_count = 0;
            while (response_count == 0 && timeout_count < 2000) begin @(negedge clock); timeout_count = timeout_count + 1; end
            if (timeout_count == 2000) $fatal(1, "W_BEFORE_AW_TIMEOUT");
        end else if (selected_case == "AW_BEFORE_W") begin
            awready_driver = 1'b1; wready = 1'b0;
            before_aw_count = accepted_aw;
            before_w_count = accepted_w;
            command(OP_CONFIGURE, 32, 0, 32'h900, 16, 1'b0);
            for (i = 0; i < 16; i = i + 1) begin
                expected[32'h900/4+i] = 32'hc001_0000 ^ (11 << 8) ^ i;
                expected_valid[32'h900/4+i] = 1'b1;
                command(OP_STORE, 32, expected[32'h900/4+i], 32'h900+i*4, 0, 1'b0);
            end
            tick(8);
            if (accepted_aw <= before_aw_count || accepted_w != before_w_count ||
                awvalid || !wvalid)
                $fatal(1, "AW_BEFORE_W_HANDSHAKE_ORDER_INVALID aw=%0d w=%0d awvalid=%b wvalid=%b",
                       accepted_aw-before_aw_count, accepted_w-before_w_count, awvalid, wvalid);
            wready = 1'b1;
            command(OP_DRAIN, 0, 0, 0, 0, 1'b0);
            timeout_count = 0;
            while (response_count == 0 && timeout_count < 2000) begin @(negedge clock); timeout_count = timeout_count + 1; end
            if (timeout_count == 2000) $fatal(1, "AW_BEFORE_W_TIMEOUT");
        end else if (selected_case == "STALLS") begin
            awready_driver = 1'b0; wready = 1'b0;
            command(OP_CONFIGURE, 32, 0, 32'ha00, 16, 1'b0);
            for (i = 0; i < 16; i = i + 1) begin
                expected[32'ha00/4+i] = 32'h1234_0000+i;
                command(OP_STORE, 32, expected[32'ha00/4+i], 32'ha00+i*4, 0, 1'b0);
            end
            tick(4);
            if (!awvalid || !wvalid) $fatal(1, "STALL_SETUP_FAILED");
            held_aw = {awaddr, awlen};
            held_stalled_w = {wdata, wstrb, wlast};
            tick(4);
            if (!awvalid || !wvalid || {awaddr, awlen} !== held_aw ||
                {wdata, wstrb, wlast} !== held_stalled_w)
                $fatal(1, "CHANNEL_PAYLOAD_CHANGED_DURING_STALL");
            awready_driver = 1'b1;
            wready = 1'b1;
            command(OP_DRAIN, 0, 0, 0, 0, 1'b0);
            timeout_count = 0;
            while (response_count == 0 && timeout_count < 2000) begin @(negedge clock); timeout_count = timeout_count + 1; end
            if (timeout_count == 2000) $fatal(1, "STALL_RECOVERY_TIMEOUT");
            if (accepted_aw != 1 || accepted_w != 16 || committed_beats != 16)
                $fatal(1, "STALL_RECOVERY_COUNT_MISMATCH aw=%0d w=%0d committed=%0d",
                       accepted_aw, accepted_w, committed_beats);
        end else if (selected_case == "PENDING_STORE") begin
            awready_driver = 1'b0; wready = 1'b0;
            command(OP_CONFIGURE, 32, 0, 32'h1700, 17, 1'b0);
            for (i = 0; i < 16; i = i + 1) begin
                expected[32'h1700/4+i] = 32'h1700_0000+i;
                command(OP_STORE, 32, expected[32'h1700/4+i], 32'h1700+i*4, 0, 1'b0);
            end
            expected[32'h1740/4] = 32'h1700_0010;
            @(negedge clock); in1 = OP_STORE; in2 = 32; in3 = 32'h1700_0010;
            in4 = 32'h1740; in5 = 0; start = 1'b1;
            tick(4);
            if (done || fault) $fatal(1, "PENDING_STORE_NOT_HELD");
            awready_driver = 1'b1; wready = 1'b1;
            wait_for_done_or_fault(2000, "PENDING_STORE", done_seen, fault_seen);
            if (!done_seen || fault_seen) $fatal(1, "PENDING_STORE_NOT_ACCEPTED_ONCE");
            start = 1'b0;
            tick(2);
            command(OP_DRAIN, 0, 0, 0, 0, 1'b0);
            timeout_count = 0;
            while (response_count < accepted_aw && timeout_count < 2000) begin
                @(negedge clock); timeout_count = timeout_count + 1;
            end
            if (timeout_count == 2000 || committed_beats != 17)
                $fatal(1, "PENDING_STORE_SCOREBOARD_MISMATCH committed=%0d", committed_beats);
        end else if (selected_case == "PENDING_OPERAND_CHANGE") begin
            awready_driver = 1'b0; wready = 1'b0;
            command(OP_CONFIGURE, 32, 0, 32'h1800, 17, 1'b0);
            for (i = 0; i < 16; i = i + 1) begin
                expected[32'h1800/4+i] = 32'h1800_0000+i;
                command(OP_STORE, 32, 32'h1800_0000+i, 32'h1800+i*4, 0, 1'b0);
            end
            expected[32'h1840/4] = 32'h1800_0010;
            @(negedge clock); in1 = OP_STORE; in2 = 32; in3 = 32'h1800_0010;
            in4 = 32'h1840; in5 = 0; start = 1'b1;
            tick(3);
            in3 = 32'hbad0_bad0;
            tick(2);
            if (!fault) $fatal(1, "PENDING_OPERAND_CHANGE_NOT_REJECTED");
        end else if (selected_case == "TRANSIENT_DRAIN_RESTORE") begin
            prepare_pending_store(32'h1850, FIFO_DEPTH+1, 1);
            tick(3);
            in1 = OP_DRAIN;
            tick(2);
            if (fault || done || !dut.store_pending)
                $fatal(1, "TRANSIENT_DRAIN_STOLE_PENDING_STORE");
            in1 = OP_STORE; in2 = 32;
            in3 = 32'he000_0000 ^ (1 << 8) ^ FIFO_DEPTH;
            in4 = 32'h1850 + FIFO_DEPTH*4;
            awready_driver = 1'b1; wready = 1'b1;
            wait_for_done_or_fault(2000, "TRANSIENT_DRAIN_RESTORE", done_seen, fault_seen);
            if (!done_seen || fault_seen) $fatal(1, "TRANSIENT_DRAIN_RESTORE_LOST_SNAPSHOT");
            start = 1'b0;
            tick(2);
            command(OP_DRAIN, 0, 0, 0, 0, 1'b0);
            timeout_count = 0;
            while (response_count < accepted_aw && timeout_count < 2000) begin
                @(negedge clock); timeout_count = timeout_count + 1;
            end
            if (timeout_count == 2000 || committed_beats != 17 || dut.accepted != 17)
                $fatal(1, "TRANSIENT_DRAIN_RESTORE_SCOREBOARD_MISMATCH");
            for (i = 0; i < 17; i = i + 1)
                if (memory[32'h1850/4+i] !== expected[32'h1850/4+i])
                    $fatal(1, "TRANSIENT_DRAIN_RESTORE_DATA_MISMATCH index=%0d", i);
        end else if (selected_case == "TRANSIENT_CONFIGURE_PENDING") begin
            prepare_pending_store(32'h1c00, FIFO_DEPTH+1, 2);
            in1 = OP_CONFIGURE; in2 = 32; in4 = 32'h1d00; in5 = 3;
            tick(2);
            if (fault || done || !dut.store_pending || dut.region_base !== 32'h1c00 ||
                dut.region_count !== FIFO_DEPTH+1)
                $fatal(1, "TRANSIENT_CONFIGURE_CHANGED_PENDING_CONTEXT");
            in1 = OP_STORE; in2 = 32; in3 = 32'he000_0000 ^ (2 << 8) ^ FIFO_DEPTH;
            in4 = 32'h1c00 + FIFO_DEPTH*4; in5 = 0;
            awready_driver = 1'b1; wready = 1'b1;
            wait_for_done_or_fault(2000, "TRANSIENT_CONFIGURE_PENDING", done_seen, fault_seen);
            if (!done_seen || fault_seen || dut.accepted != FIFO_DEPTH+1)
                $fatal(1, "TRANSIENT_CONFIGURE_LOST_PENDING_STORE done_seen=%b fault_seen=%b done=%b fault=%b accepted=%0d pending=%b fifo_count=%0d opcode=%0d start=%b data=%h address=%h pending_data=%h pending_address=%h region_base=%h region_count=%0d store_input=%b pending_changed=%b",
                       done_seen, fault_seen, done, fault, dut.accepted, dut.store_pending,
                       dut.fifo_count, in1, start, in3, in4, dut.pending_data,
                       dut.pending_address, dut.region_base, dut.region_count,
                       dut.store_input, dut.pending_store_changed);
            start = 1'b0;
            tick(2);
            command(OP_DRAIN, 0, 0, 0, 0, 1'b0);
            timeout_count = 0;
            while (response_count < accepted_aw && timeout_count < 2000) begin
                @(negedge clock); timeout_count = timeout_count + 1;
            end
            if (timeout_count == 2000 || committed_beats != FIFO_DEPTH+1)
                $fatal(1, "TRANSIENT_CONFIGURE_TRANSPORT_MISMATCH");
            for (i = 0; i <= FIFO_DEPTH; i = i + 1)
                if (memory[32'h1c00/4+i] !== expected[32'h1c00/4+i])
                    $fatal(1, "TRANSIENT_CONFIGURE_DATA_MISMATCH index=%0d", i);
        end else if (selected_case == "DRAIN_PENDING_FULL_COUNT") begin
            prepare_pending_store(32'h1e00, FIFO_DEPTH+1, 3);
            in1 = OP_DRAIN; in2 = 0; in3 = 0; in4 = 0; in5 = 0;
            tick(2);
            if (fault || done || !dut.store_pending)
                $fatal(1, "DRAIN_DID_NOT_DEFER_FOR_PENDING_STORE");
            awready_driver = 1'b1; wready = 1'b1;
            wait_for_done_or_fault(2000, "DRAIN_PENDING_STORE_DONE", done_seen, fault_seen);
            if (!done_seen || fault_seen || dut.accepted != FIFO_DEPTH+1 || done ||
                response_count >= accepted_aw)
                $fatal(1, "PENDING_STORE_ACK_OR_DRAIN_ORDER_INVALID");
            wait_for_done_or_fault(2000, "DRAIN_PENDING_FINAL_B", done_seen, fault_seen);
            if (!done_seen || fault_seen || dut.accepted != FIFO_DEPTH+1 ||
                committed_beats != FIFO_DEPTH+1 || response_count != accepted_aw || bvalid)
                $fatal(1, "DRAIN_PENDING_COMPLETED_BEFORE_FINAL_B");
            start = 1'b0;
        end else if (selected_case == "DRAIN_PENDING_SHORT_COUNT") begin
            prepare_pending_store(32'h1f00, FIFO_DEPTH+2, 4);
            in1 = OP_DRAIN; in2 = 0; in3 = 0; in4 = 0; in5 = 0;
            tick(2);
            if (fault || done || !dut.store_pending)
                $fatal(1, "SHORT_DRAIN_FAULTED_BEFORE_PENDING_STORE");
            awready_driver = 1'b1; wready = 1'b1;
            wait_for_done_or_fault(2000, "DRAIN_PENDING_SHORT_STORE_DONE", done_seen, fault_seen);
            if (!done_seen || fault_seen || dut.accepted != FIFO_DEPTH+1 || dut.store_pending)
                $fatal(1, "SHORT_DRAIN_PREEMPTED_PENDING_STORE");
            wait_for_done_or_fault(2000, "DRAIN_PENDING_SHORT_FAULT", done_seen, fault_seen);
            if (!fault_seen || done_seen || dut.accepted != FIFO_DEPTH+1 || dut.store_pending)
                $fatal(1, "SHORT_DRAIN_DID_NOT_FAULT_AFTER_PENDING_STORE");
        end else if (selected_case == "PENDING_OPERAND_CHANGE_ON_FREE") begin
            prepare_pending_store(32'h2000, FIFO_DEPTH+1, 5);
            in3 = in3 ^ 32'h0000_0100;
            wready = 1'b1;
            #1;
            if (done) $fatal(1, "PENDING_OPERAND_CHANGE_COMPLETED_BEFORE_FIFO_FREE");
            @(posedge clock);
            #1;
            if (!fault || done || dut.accepted != FIFO_DEPTH || !dut.store_pending)
                $fatal(1, "PENDING_OPERAND_CHANGE_ON_FREE_NOT_REJECTED atomically");
            @(negedge clock);
            start = 1'b0;
        end else if (selected_case == "DRAIN_BEFORE_B") begin
            awready_driver = 1'b1; wready = 1'b1; response_delay_set = 12;
            command(OP_CONFIGURE, 32, 0, 32'h1880, 1, 1'b0);
            expected[32'h1880/4] = 32'h1880_0001;
            command(OP_STORE, 32, 32'h1880_0001, 32'h1880, 0, 1'b0);
            @(negedge clock); in1 = OP_DRAIN; start = 1'b1;
            tick(3);
            if (done || fault) $fatal(1, "DRAIN_COMPLETED_BEFORE_B");
            wait_for_done_or_fault(2000, "DRAIN_BEFORE_B", done_seen, fault_seen);
            if (!done_seen || fault_seen) $fatal(1, "DRAIN_NOT_COMPLETED_AFTER_B");
            start = 1'b0;
        end else if (selected_case == "DRAIN_ONE_SHOT") begin
            awready_driver = 1'b1; wready = 1'b1; response_delay_set = 24;
            command(OP_CONFIGURE, 32, 0, 32'h1880, 1, 1'b0);
            expected[32'h1880/4] = 32'h1880_0001;
            expected_valid[32'h1880/4] = 1'b1;
            command(OP_STORE, 32, 32'h1880_0001, 32'h1880, 0, 1'b0);
            @(negedge clock); in1 = OP_DRAIN; start = 1'b1;
            @(negedge clock); start = 1'b0;
            timeout_count = 0;
            while (committed_beats != 1 && timeout_count < 200) begin
                @(negedge clock);
                timeout_count = timeout_count + 1;
            end
            if (timeout_count == 200 || response_count != 0 || bvalid || done || fault ||
                !dut.drain_pending || !dut.region_active)
                $fatal(1, "DRAIN_ONE_SHOT_NOT_PENDING_BEFORE_B");
            wait_for_done_or_fault(2000, "DRAIN_ONE_SHOT_FINAL_B", done_seen, fault_seen);
            if (!done_seen || fault_seen || dut.region_active || dut.drain_pending ||
                committed_beats != 1 || response_count != 1)
                $fatal(1, "DRAIN_ONE_SHOT_NOT_CLOSED_AFTER_B");
            command(OP_CONFIGURE, 32, 0, 32'h18a0, 1, 1'b0);
            expected[32'h18a0/4] = 32'h18a0_0002;
            expected_valid[32'h18a0/4] = 1'b1;
            command(OP_STORE, 32, 32'h18a0_0002, 32'h18a0, 0, 1'b0);
            response_delay_set = 0;
            command(OP_DRAIN, 0, 0, 0, 0, 1'b0);
            if (fault || dut.region_active || dut.drain_pending ||
                memory[32'h18a0/4] !== expected[32'h18a0/4])
                $fatal(1, "DRAIN_ONE_SHOT_NEXT_REGION_FAILED");
        end else if (selected_case == "PREMATURE_CONFIGURE") begin
            awready_driver = 1'b0; wready = 1'b0;
            command(OP_CONFIGURE, 32, 0, 32'h1a00, 16, 1'b0);
            for (i = 0; i < 16; i = i + 1) begin
                expected[32'h1a00/4+i] = 32'h1a00_0000+i;
                command(OP_STORE, 32, expected[32'h1a00/4+i], 32'h1a00+i*4, 0, 1'b0);
            end
            tick(3);
            if (!awvalid || !wvalid) $fatal(1, "PREMATURE_CONFIGURE_SETUP_FAILED");
            command(OP_CONFIGURE, 32, 0, 32'h1b00, 16, 1'b1);
            awready_driver = 1'b1; wready = 1'b1;
            timeout_count = 0;
            while (response_count == 0 && timeout_count < 2000) begin
                @(negedge clock); timeout_count = timeout_count + 1;
            end
            if (timeout_count == 2000 || committed_beats != 16 || accepted_aw != 1)
                $fatal(1, "PREMATURE_CONFIGURE_DROPPED_EXPOSED_TRANSPORT");
        end else if (selected_case == "STORE_ACK_PULSE") begin
            awready_driver = 1'b1; wready = 1'b1;
            command(OP_CONFIGURE, 32, 0, 32'h1d00, 4, 1'b0);
            expected[32'h1d00/4] = 32'h1d00_0001;
            expected_valid[32'h1d00/4] = 1'b1;
            @(negedge clock);
            in1 = OP_STORE; in2 = 32; in3 = 32'h1d00_0001; in4 = 32'h1d00; start = 1'b1;
            @(posedge clock); #1; start = 1'b0; #1;
            if (!done || fault || dut.accepted != 1)
                $fatal(1, "STORE_ACK_NOT_VISIBLE_AFTER_START_DROP done=%b fault=%b accepted=%0d",
                       done, fault, dut.accepted);
            @(posedge clock); #1;
            if (done || fault || dut.accepted != 1)
                $fatal(1, "STORE_ACK_PULSE_NOT_SINGLE_CYCLE done=%b accepted=%0d", done, dut.accepted);

            expected[32'h1d00/4+1] = 32'h1d00_0002;
            expected_valid[32'h1d00/4+1] = 1'b1;
            @(negedge clock);
            in1 = OP_STORE; in2 = 32; in3 = 32'h1d00_0002; in4 = 32'h1d04; start = 1'b1;
            tick(1);
            if (fault || dut.accepted != 2)
                $fatal(1, "SECOND_STORE_NOT_ACCEPTED accepted=%0d", dut.accepted);

            expected[32'h1d00/4+2] = 32'h1d00_0002;
            expected_valid[32'h1d00/4+2] = 1'b1;
            in3 = 32'h1d00_0002; in4 = 32'h1d08;
            @(posedge clock); #1;
            if (!done || fault || dut.accepted != 3)
                $fatal(1, "HELD_START_CHANGED_ADDRESS_NOT_REARMED done=%b accepted=%0d", done, dut.accepted);
            tick(3);
            if (done || fault || dut.accepted != 3)
                $fatal(1, "HELD_STORE_REPEATED_OR_ACK_STUCK accepted=%0d", dut.accepted);
            start = 1'b0; tick(1);

            expected[32'h1d00/4+3] = 32'h1d00_0002;
            expected_valid[32'h1d00/4+3] = 1'b1;
            @(negedge clock);
            in1 = OP_STORE; in2 = 32; in3 = 32'h1d00_0002; in4 = 32'h1d0c; start = 1'b1;
            @(posedge clock); #1; start = 1'b0; #1;
            if (!done || fault || dut.accepted != 4)
                $fatal(1, "IDENTICAL_PAYLOAD_NEXT_STORE_NOT_REARMED done=%b accepted=%0d", done, dut.accepted);
            @(posedge clock); #1;
            if (done || fault || dut.accepted != 4)
                $fatal(1, "IDENTICAL_PAYLOAD_STORE_DUPLICATED accepted=%0d", dut.accepted);
            command(OP_DRAIN, 0, 0, 0, 0, 1'b0);
            timeout_count = 0;
            while (response_count < accepted_aw && timeout_count < 1000) begin
                @(negedge clock); timeout_count = timeout_count + 1;
            end
            if (timeout_count == 1000 || fault || dut.region_active || accepted_w != 4 ||
                committed_beats != 4 || response_count != 1 ||
                memory[32'h1d00/4] !== 32'h1d00_0001 ||
                memory[32'h1d00/4+1] !== 32'h1d00_0002 ||
                memory[32'h1d00/4+2] !== 32'h1d00_0002 ||
                memory[32'h1d00/4+3] !== 32'h1d00_0002)
                $fatal(1, "STORE_ACK_PULSE_DRAIN_FAILED");
        end else if (selected_case == "DUPLICATE_START") begin
            awready_driver = 1'b1; wready = 1'b1;
            @(negedge clock); in1 = OP_CONFIGURE; in2 = 32; in4 = 32'h1900;
            in5 = 1; start = 1'b1;
            tick(5);
            if (fault || dut.accepted != 0) $fatal(1, "HELD_CONFIGURE_NOT_IDEMPOTENT");
            start = 1'b0;
            tick(1);
            @(negedge clock); in1 = OP_STORE; in2 = 32; in3 = 32'hfeed_0001;
            in4 = 32'h1900; start = 1'b1;
            expected[32'h1900/4] = 32'hfeed_0001;
            expected_valid[32'h1900/4] = 1'b1;
            wait_for_done_or_fault(100, "DUPLICATE_START_STORE", done_seen, fault_seen);
            if (!done_seen || fault_seen) $fatal(1, "HELD_STORE_NO_COMPLETION");
            tick(4);
            if (dut.accepted != 1) $fatal(1, "HELD_STORE_DUPLICATED accepted=%0d", dut.accepted);
            start = 1'b0;
            command(OP_DRAIN, 0, 0, 0, 0, 1'b0);
            timeout_count = 0;
            while (response_count == 0 && timeout_count < 1000) begin
                @(negedge clock); timeout_count = timeout_count + 1;
            end
            if (timeout_count == 1000 || memory[32'h1900/4] !== 32'hfeed_0001)
                $fatal(1, "HELD_STORE_SCOREBOARD_FAILURE");
        end else if (selected_case == "BAD_CONFIG_SIZE") begin
            command(OP_CONFIGURE, 31, 0, 32'h1000, 1, 1'b1);
        end else if (selected_case == "BAD_CONFIG_ALIGN") begin
            command(OP_CONFIGURE, 32, 0, 32'h1001, 1, 1'b1);
        end else if (selected_case == "COUNT_MSB") begin
            @(negedge clock);
            in1 = OP_CONFIGURE; in2 = 32; in3 = 0;
            in4 = 32'h1000; in5 = 32'h8000_0000; start = 1'b1;
            timeout_count = 0;
            while (!fault && !done && timeout_count < 1000) begin
                @(posedge clock);
                #1;
                if (!fault && !done) @(negedge clock);
                timeout_count = timeout_count + 1;
            end
            start = 1'b0;
            if (!fault)
                $fatal(1, "F3_COUNT_MSB_GUARD_INSTALLED_CONTEXT_OR_TRAFFIC");
            if (!fault || done || dut.region_active || dut.region_base != 0 ||
                dut.region_count != 0 || dut.accepted != 0 || dut.scheduled != 0 ||
                dut.completed != 0 || dut.descriptor_count != 0 || dut.fifo_count != 0 ||
                dut.reserved_data_count != 0 || awvalid || wvalid || accepted_aw != 0 ||
                accepted_w != 0 || committed_beats != 0 || response_count != 0)
                $fatal(1, "F3_COUNT_MSB_GUARD_INSTALLED_CONTEXT_OR_TRAFFIC");
        end else if (selected_case == "COUNT_MAX_64") begin
            if (ADDR_BITS != 64) $fatal(1, "F3_COUNT_MAX_REQUIRES_A64");
            command(OP_CONFIGURE, 32, 0, 64'h0000_0000_0000_1000, 32'h7fff_ffff, 1'b0);
            if (fault || !dut.region_active ||
                dut.region_base !== 64'h0000_0000_0000_1000 ||
                dut.region_count !== 32'h7fff_ffff || dut.accepted != 0 ||
                dut.scheduled != 0 || dut.completed != 0 || dut.descriptor_count != 0 ||
                dut.fifo_count != 0 || dut.reserved_data_count != 0 || awvalid || wvalid ||
                accepted_aw != 0 || accepted_w != 0 || committed_beats != 0 || response_count != 0)
                $fatal(1, "F3_COUNT_MAX_CONFIGURE_MISMATCH active=%b base=%h count=%h accepted=%0d scheduled=%0d completed=%0d",
                       dut.region_active, dut.region_base, dut.region_count,
                       dut.accepted, dut.scheduled, dut.completed);
        end else if (selected_case == "OVERFLOW32") begin
            command(OP_CONFIGURE, 32, 0, 32'hffff_fffc, 2, 1'b1);
        end else if (selected_case == "OVERFLOW64") begin
            command(OP_CONFIGURE, 32, 0, {ADDR_BITS{1'b1}}-3, 2, 1'b1);
        end else if (selected_case == "BAD_STORE_SIZE") begin
            command(OP_CONFIGURE, 32, 0, 32'h1100, 1, 1'b0);
            command(OP_STORE, 16, 32'h1234, 32'h1100, 0, 1'b1);
        end else if (selected_case == "BAD_STORE_SIZE8") begin
            command(OP_CONFIGURE, 32, 0, 32'h1180, 1, 1'b0);
            command(OP_STORE, 8, 32'h12, 32'h1180, 0, 1'b1);
        end else if (selected_case == "BAD_STORE_ADDRESS") begin
            command(OP_CONFIGURE, 32, 0, 32'h1200, 2, 1'b0);
            command(OP_STORE, 32, 32'h1234, 32'h1204, 0, 1'b1);
        end else if (selected_case == "EARLY_DRAIN") begin
            command(OP_CONFIGURE, 32, 0, 32'h1300, 2, 1'b0);
            command(OP_STORE, 32, 32'h1234, 32'h1300, 0, 1'b0);
            command(OP_DRAIN, 0, 0, 0, 0, 1'b1);
        end else if (selected_case == "BID") begin
            bid = 1'b1;
            command(OP_CONFIGURE, 32, 0, 32'h1400, 16, 1'b0);
            awready_driver = 1'b1; wready = 1'b1;
            for (i = 0; i < 16; i = i + 1) begin
                expected[32'h1400/4+i] = 32'hb1d0_0000+i;
                command(OP_STORE, 32, 32'hb1d0_0000+i, 32'h1400+i*4, 0, 1'b0);
            end
            timeout_count = 0;
            while (!fault && timeout_count < 2000) begin @(negedge clock); timeout_count = timeout_count + 1; end
            if (!fault) $fatal(1, "BID_FAULT_NOT_RAISED");
        end else if (selected_case == "BRESP") begin
            bresp = 2'b10;
            command(OP_CONFIGURE, 32, 0, 32'h1500, 16, 1'b0);
            awready_driver = 1'b1; wready = 1'b1;
            for (i = 0; i < 16; i = i + 1) begin
                expected[32'h1500/4+i] = 32'hb2e5_0000+i;
                command(OP_STORE, 32, 32'hb2e5_0000+i, 32'h1500+i*4, 0, 1'b0);
            end
            timeout_count = 0;
            while (!fault && timeout_count < 2000) begin @(negedge clock); timeout_count = timeout_count + 1; end
            if (!fault) $fatal(1, "BRESP_FAULT_NOT_RAISED");
        end else if (selected_case == "BRESP_NO_NEXT_DESCRIPTOR") begin
            response_delay_set = 1000;
            awready_driver = 1'b1; wready = 1'b1;
            command(OP_CONFIGURE, 32, 0, 32'h2300, 32, 1'b0);
            for (i = 0; i < 32; i = i + 1) begin
                expected[32'h2300/4+i] = 32'hb3e5_0000+i;
                expected_valid[32'h2300/4+i] = 1'b1;
                command(OP_STORE, 32, expected[32'h2300/4+i], 32'h2300+i*4, 0, 1'b0);
            end
            if (dut.accepted != 32 || dut.fifo_count != 16 || accepted_aw != 1 ||
                accepted_w != 16 || response_count != 0 || bvalid || response_delay <= 0)
                $fatal(1, "BRESP_NEXT_DESCRIPTOR_SETUP accepted=%0d fifo=%0d aw=%0d w=%0d responses=%0d delay=%0d",
                       dut.accepted, dut.fifo_count, accepted_aw, accepted_w,
                       response_count, response_delay);
            bresp = 2'b10;
            timeout_count = 0;
            while (!fault && timeout_count < 2000) begin
                @(negedge clock);
                timeout_count = timeout_count + 1;
            end
            if (timeout_count == 2000 || !fault || done || response_count != 1 ||
                accepted_aw != 1 || accepted_w != 16 || awvalid || wvalid)
                $fatal(1, "BRESP_NEXT_DESCRIPTOR_FAULT state fault=%b done=%b responses=%0d aw=%0d w=%0d awvalid=%b wvalid=%b",
                       fault, done, response_count, accepted_aw, accepted_w, awvalid, wvalid);
            before_aw_count = accepted_aw;
            before_w_count = accepted_w;
            tick(4);
            if (!fault || accepted_aw != before_aw_count || accepted_w != before_w_count ||
                awvalid || wvalid)
                $fatal(1, "BRESP_NEXT_DESCRIPTOR_TRANSPORT_CONTINUED");
            @(negedge clock);
            in1 = OP_DRAIN; in2 = 0; in3 = 0; in4 = 0; in5 = 0; start = 1'b1;
            tick(3);
            if (!fault || done || accepted_aw != before_aw_count || accepted_w != before_w_count)
                $fatal(1, "BRESP_NEXT_DESCRIPTOR_DRAIN_NOT_REJECTED");
            start = 1'b0;

            reset_fixture();
            awready_driver = 1'b1; wready = 1'b1;
            expected[32'h2400/4] = 32'hcafef00d;
            expected_valid[32'h2400/4] = 1'b1;
            command(OP_CONFIGURE, 32, 0, 32'h2400, 1, 1'b0);
            command(OP_STORE, 32, expected[32'h2400/4], 32'h2400, 0, 1'b0);
            command(OP_DRAIN, 0, 0, 0, 0, 1'b0);
            timeout_count = 0;
            while (response_count < accepted_aw && timeout_count < 2000) begin
                @(negedge clock);
                timeout_count = timeout_count + 1;
            end
            if (timeout_count == 2000 || fault || committed_beats != 1 ||
                accepted_aw != 1 || accepted_w != 1 ||
                memory[32'h2400/4] !== expected[32'h2400/4])
                $fatal(1, "BRESP_RESET_RECOVERY_FAILED fault=%b committed=%0d aw=%0d w=%0d",
                       fault, committed_beats, accepted_aw, accepted_w);
        end else if (selected_case == "BOUNDARY") begin
            awready_driver = 1'b1; wready = 1'b1;
            store_region(32'h0000_0ff0, 38, 14);
        end else if (selected_case == "MAX_COUNT") begin
            command(OP_CONFIGURE, 32, 0, 32'h2000, 64, 1'b0);
        end else begin
            $fatal(1, "UNKNOWN_CASE %s", selected_case);
        end
        tick(3);
        $display("WRITE_BURST_ENGINE PASS CASE=%s B=%0d FIFO=%0d AW=%0d W=%0d BRESP=%0d",
                 selected_case, B_MAX, FIFO_DEPTH, accepted_aw, accepted_w, response_count);
        $finish;
    end
endmodule
