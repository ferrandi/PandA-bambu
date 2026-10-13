module tb_testbench_memaxi_model;
    localparam integer QUEUE_SIZE = 8;
    localparam integer READ_DELAY = 2;
    parameter integer WRITE_DELAY = 2;
    localparam integer MODEL_C_W = ((QUEUE_SIZE - 1) > (WRITE_DELAY + 1)) ? (QUEUE_SIZE - 1) : (WRITE_DELAY + 1);
    localparam logic [31:0] READ_BASE = 32'h0000_1040;

    logic clock = 1'b0;
    always #5 clock = ~clock;
    logic reset = 1'b0;
    logic [0:0] arid = 0;
    logic [31:0] araddr = 0;
    logic [7:0] arlen = 0;
    logic [2:0] arsize = 2;
    logic [1:0] arburst = 1;
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
    wire rlast, rvalid;
    logic rready = 0;
    logic [0:0] awid = 0;
    logic [31:0] awaddr = 0;
    logic [7:0] awlen = 0;
    logic [2:0] awsize = 2;
    logic [1:0] awburst = 1;
    logic [0:0] awlock = 0;
    logic [3:0] awcache = 0;
    logic [2:0] awprot = 0;
    logic [3:0] awqos = 0;
    logic [3:0] awregion = 0;
    logic [0:0] awuser = 0;
    logic awvalid = 0;
    wire awready;
    logic [31:0] wdata = 0;
    logic [3:0] wstrb = 4'hf;
    logic wlast = 1;
    logic wvalid = 0;
    logic [0:0] wuser = 0;
    wire wready;
    wire [0:0] bid;
    wire [1:0] bresp, buser;
    wire bvalid;
    logic bready = 1;

    integer ar_handshakes = 0;
    integer r_handshakes = 0;
    integer aw_handshakes = 0;
    integer w_handshakes = 0;
    integer b_handshakes = 0;
    logic [0:0] observed_bid [0:1023];
    integer observed_b_count = 0;
    integer failure_count = 0;
    integer unexpected_count = 0;
    integer queued_w_stall_cycles = 0;
    integer mode_qualify = 0;
    string negative_case = "";
    time last_aw_time = 0;
    time last_w_time = 0;
    time last_b_time = 0;
    logic stall_monitor_enable = 0;
    logic [37:0] stall_payload;
    logic stall_payload_changed = 0;
    integer stall_rvalid_lost = 0;

    TestbenchMEMAXI #(
        .index(0), .QUEUE_SIZE(QUEUE_SIZE),
        .READ_DELAY(READ_DELAY), .WRITE_DELAY(WRITE_DELAY)
    ) memory (
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

    always @(posedge clock) begin
        if (reset) begin
            if (arvalid && arready) ar_handshakes = ar_handshakes + 1;
            if (rvalid && rready) r_handshakes = r_handshakes + 1;
            if (awvalid && awready) begin aw_handshakes = aw_handshakes + 1; last_aw_time = $time; end
            if (wvalid && wready) begin w_handshakes = w_handshakes + 1; last_w_time = $time; end
            if (bvalid && bready) begin
                b_handshakes = b_handshakes + 1;
                if (observed_b_count < 1024) observed_bid[observed_b_count] = bid;
                observed_b_count = observed_b_count + 1;
                last_b_time = $time;
            end
            if (stall_monitor_enable) begin
                if (rvalid !== 1'b1) stall_rvalid_lost = 1;
                if ({rid, rdata, rresp, ruser, rlast} !== stall_payload)
                    stall_payload_changed = 1;
            end
        end
    end

    task automatic reset_case;
        begin
            @(negedge clock);
            reset = 1'b0;
            arvalid = 0;
            rready = 0;
            awvalid = 0;
            wvalid = 0;
            bready = 1;
            last_aw_time = 0;
            last_w_time = 0;
            last_b_time = 0;
            repeat (3) @(negedge clock);
            reset = 1'b1;
        end
    endtask

    task automatic send_ar(input logic [31:0] address, input integer beats);
        integer elapsed;
        logic accepted;
        begin
            @(negedge clock);
            araddr = address;
            arlen = beats - 1;
            arvalid = 1;
            elapsed = 0;
            accepted = 0;
            while (!accepted && elapsed < 100) begin
                @(posedge clock);
                accepted = arready;
                elapsed = elapsed + 1;
            end
            if (!accepted) record_violation("AR_HANDSHAKE_TIMEOUT", $sformatf("address=%h", address));
            @(negedge clock);
            arvalid = 0;
        end
    endtask

    task automatic send_aw(input logic [31:0] address, input integer beats);
        integer elapsed;
        logic accepted;
        begin
            @(negedge clock);
            awaddr = address;
            awlen = beats - 1;
            awvalid = 1;
            elapsed = 0;
            accepted = 0;
            while (!accepted && elapsed < 100) begin
                @(posedge clock);
                accepted = awready;
                elapsed = elapsed + 1;
            end
            if (!accepted) record_violation("AW_HANDSHAKE_TIMEOUT", $sformatf("address=%h", address));
            @(negedge clock);
            awvalid = 0;
        end
    endtask

    task automatic send_w(input logic [31:0] data, input logic last);
        integer elapsed;
        logic accepted;
        begin
            @(negedge clock);
            wdata = data;
            wstrb = 4'hf;
            wlast = last;
            wvalid = 1;
            elapsed = 0;
            accepted = 0;
            while (!accepted && elapsed < 100) begin
                @(posedge clock);
                accepted = wready;
                elapsed = elapsed + 1;
            end
            if (!accepted) record_violation("W_HANDSHAKE_TIMEOUT", $sformatf("data=%h", data));
            @(negedge clock);
            wvalid = 0;
        end
    endtask

    task automatic record_violation(input string marker, input string detail);
        begin
            if (mode_qualify) begin
                unexpected_count = unexpected_count + 1;
                $display("F0_QUALIFY_FAILURE %s %s", marker, detail);
            end else begin
                failure_count = failure_count + 1;
                $display("F0_EXPECTED_VIOLATION %s %s", marker, detail);
            end
        end
    endtask

    task automatic check_condition(input logic condition, input string marker, input string detail);
        begin
            if (condition) begin
                if (mode_qualify) begin
                    unexpected_count = unexpected_count + 1;
                    $display("F0_QUALIFY_FAILURE %s %s", marker, detail);
                end else begin
                    record_violation(marker, detail);
                end
            end else if (mode_qualify) begin
                $display("F0_WITNESS_PASS %s %s", marker, detail);
            end else begin
                $display("F0_OBSERVED %s %s", marker, detail);
            end
        end
    endtask

    task automatic read_and_stall(input integer beats, input integer stall_beat, input logic enqueue_ar);
        integer beat;
        integer wait_cycles;
        integer reads_before_stall;
        integer writes_before_stall;
        integer handshakes_before;
        integer handshakes_before_case;
        integer ar_before_stall;
        integer aw_before_stall;
        integer w_before_stall;
        logic [31:0] held_data;
        logic [0:0] held_id;
        logic [1:0] held_resp, held_user;
        logic held_last;
        logic payload_changed;
        begin
            handshakes_before_case = r_handshakes;
            send_ar(READ_BASE, beats);
            beat = 0;
            while (beat < beats) begin
                wait_cycles = 0;
                while (!rvalid && wait_cycles < 40) begin
                    @(negedge clock);
                    wait_cycles = wait_cycles + 1;
                end
                if (!rvalid) begin
                    record_violation("R_BEAT_MISSING", $sformatf("beat=%0d beats=%0d", beat, beats));
                    beat = beats;
                end else begin
                if (rlast !== (beat == beats - 1))
                    record_violation("R_LAST_POSITION", $sformatf("beat=%0d beats=%0d rlast=%0b", beat, beats, rlast));
                if (rdata !== (32'hcafe_0000 + beat))
                    record_violation("R_DATA_SEQUENCE", $sformatf("beat=%0d data=%h", beat, rdata));
                if (rid !== arid || rresp !== 2'b00 || ruser !== 2'b00)
                    record_violation("R_SIDEBAND_VALUE", $sformatf("beat=%0d rid=%b resp=%b user=%b", beat, rid, rresp, ruser));
                if (beat == stall_beat) begin
                    held_data = rdata;
                    held_id = rid;
                    held_resp = rresp;
                    held_user = ruser;
                    held_last = rlast;
                    stall_payload = {rid, rdata, rresp, ruser, rlast};
                    stall_payload_changed = 0;
                    stall_rvalid_lost = 0;
                    stall_monitor_enable = 1;
                    reads_before_stall = memory.m_utils.read_calls;
                    writes_before_stall = memory.m_utils.write_calls;
                    handshakes_before = r_handshakes;
                    ar_before_stall = ar_handshakes;
                    aw_before_stall = aw_handshakes;
                    w_before_stall = w_handshakes;
                    payload_changed = 0;
                    rready = 0;
                    if (enqueue_ar) begin
                        send_ar(READ_BASE + 32'h100, 1);
                        check_condition(ar_handshakes - ar_before_stall != 1,
                            "AR_NEXT_ACCEPTED_DURING_FINAL_STALL",
                            $sformatf("accepted=%0d", ar_handshakes - ar_before_stall));
                    end
                    send_aw(READ_BASE + beat*4, 1);
                    send_w(32'hdead_0000 + beat, 1'b1);
                    check_condition(aw_handshakes - aw_before_stall != 1 || w_handshakes - w_before_stall != 1,
                        "AW_W_INDEPENDENT_HANDSHAKES",
                        $sformatf("aw=%0d w=%0d", aw_handshakes - aw_before_stall, w_handshakes - w_before_stall));
                    wait_cycles = 0;
                    while (memory.m_utils.write_calls == writes_before_stall && wait_cycles < WRITE_DELAY + 16) begin
                        @(negedge clock);
                        wait_cycles = wait_cycles + 1;
                    end
                    repeat (3) @(negedge clock);
                    payload_changed = stall_payload_changed || stall_rvalid_lost ||
                        (rvalid !== 1'b1) || ({rid, rdata, rresp, ruser, rlast} !==
                        {held_id, held_data, held_resp, held_user, held_last});
                    stall_monitor_enable = 0;
                    check_condition(payload_changed,
                        "R_STALL_PAYLOAD_CHANGED",
                        $sformatf("beat=%0d held=%h current=%h", beat, held_data, rdata));
                    check_condition(memory.m_utils.read_calls != reads_before_stall,
                        "R_BACKEND_READ_DURING_STALL",
                        $sformatf("before=%0d after=%0d", reads_before_stall, memory.m_utils.read_calls));
                    check_condition(r_handshakes != handshakes_before,
                        "R_HANDSHAKE_DURING_STALL", $sformatf("before=%0d after=%0d", handshakes_before, r_handshakes));
                    $display("F0_OBSERVED R_STALL_CASE beat=%0d beats=%0d same_address_write=1", beat, beats);
                    check_condition(memory.m_utils.write_calls == writes_before_stall ||
                                    memory.m_utils.words[((READ_BASE + beat*4) >> 2) % 4096] !== (32'hdead_0000 + beat),
                        "R_STALL_SAME_ADDRESS_WRITE_COMMIT",
                        $sformatf("calls_before=%0d calls_after=%0d word=%h", writes_before_stall,
                                  memory.m_utils.write_calls,
                                  memory.m_utils.words[((READ_BASE + beat*4) >> 2) % 4096]));
                end
                rready = 1;
                @(posedge clock);
                @(negedge clock);
                rready = 0;
                beat = beat + 1;
                end
            end
            if (r_handshakes - handshakes_before_case != beats)
                record_violation("R_BEAT_HANDSHAKE_COUNT", $sformatf("expected=%0d observed=%0d", beats, r_handshakes - handshakes_before_case));
            if (enqueue_ar) begin
                wait_cycles = 0;
                while (!rvalid && wait_cycles < WRITE_DELAY + 20) begin
                    @(negedge clock);
                    wait_cycles = wait_cycles + 1;
                end
                if (!rvalid) begin
                    record_violation("AR_QUEUED_RESPONSE_MISSING", "final-stall AR did not produce a response after drain");
                end else begin
                    if (rid !== arid || rdata !== 32'hcafe_1000 || rresp !== 0 || ruser !== 0 || rlast !== 1)
                        record_violation("AR_QUEUED_RESPONSE_PAYLOAD", $sformatf("rid=%b data=%h resp=%b user=%b last=%b", rid, rdata, rresp, ruser, rlast));
                    rready = 1;
                    @(posedge clock);
                    @(negedge clock);
                    rready = 0;
                    if (r_handshakes - handshakes_before_case != beats + 1)
                        record_violation("AR_QUEUED_RESPONSE_COUNT", $sformatf("expected=%0d observed=%0d", beats + 1, r_handshakes - handshakes_before_case));
                end
            end
        end
    endtask

    task automatic run_read_case(input integer beats, input integer stall_beat, input logic enqueue_ar);
        begin
            reset_case();
            memory.m_utils.set_word(READ_BASE, 32'hcafe_0000);
            memory.m_utils.set_word(READ_BASE + 4, 32'hcafe_0001);
            memory.m_utils.set_word(READ_BASE + 8, 32'hcafe_0002);
            memory.m_utils.set_word(READ_BASE + 12, 32'hcafe_0003);
            memory.m_utils.set_word(READ_BASE + 32'h100, 32'hcafe_1000);
            read_and_stall(beats, stall_beat, enqueue_ar);
        end
    endtask

    task automatic run_multi_beat_drain_case;
        integer observed;
        integer cycles_seen;
        integer this_beat;
        logic last_seen;
        begin
            reset_case();
            memory.m_utils.set_word(READ_BASE, 32'hcafe_0000);
            memory.m_utils.set_word(READ_BASE + 4, 32'hcafe_0001);
            memory.m_utils.set_word(READ_BASE + 8, 32'hcafe_0002);
            memory.m_utils.set_word(READ_BASE + 12, 32'hcafe_0003);
            send_ar(READ_BASE, 4);
            rready = 1;
            observed = 0;
            cycles_seen = 0;
            last_seen = 0;
            while (observed < 4 && cycles_seen < 40) begin
                @(posedge clock);
                cycles_seen = cycles_seen + 1;
                if (rvalid && rready) begin
                    this_beat = observed;
                    if (rdata !== (32'hcafe_0000 + this_beat))
                        record_violation("R_DRAIN_DATA_SEQUENCE", $sformatf("beat=%0d data=%h", this_beat, rdata));
                    if (rlast !== (this_beat == 3))
                        record_violation("R_DRAIN_RLAST_POSITION", $sformatf("beat=%0d rlast=%b", this_beat, rlast));
                    if (this_beat == 3) last_seen = rlast;
                    observed = observed + 1;
                end
            end
            @(negedge clock);
            rready = 0;
            check_condition(observed != 4 || !last_seen,
                "R_MULTI_BEAT_DRAIN",
                $sformatf("expected=4 observed=%0d final_rlast=%0b", observed, last_seen));
        end
    endtask

    task automatic run_write_case;
        integer calls_before;
        integer commits_during_stall;
        integer aw_before;
        integer w_before;
        integer b_before;
        begin
            reset_case();
            bready = 0;
            calls_before = memory.m_utils.write_calls;
            aw_before = aw_handshakes;
            w_before = w_handshakes;
            b_before = b_handshakes;
            send_aw(32'h0000_2080, 3);
            send_w(32'h1111_0001, 0);
            send_w(32'h2222_0002, 0);
            send_w(32'h3333_0003, 1);
            check_condition(aw_handshakes - aw_before != 1 || w_handshakes - w_before != 3,
                "AW_W_DISTINCT_HANDSHAKES",
                $sformatf("aw=%0d w=%0d", aw_handshakes - aw_before, w_handshakes - w_before));
            repeat (WRITE_DELAY + 4) @(negedge clock);
            commits_during_stall = memory.m_utils.write_calls - calls_before;
            check_condition(commits_during_stall != 3,
                "W_COMMIT_DUPLICATION_WHILE_B_STALLED",
                $sformatf("commit_calls=%0d", commits_during_stall));
            bready = 1;
            repeat (WRITE_DELAY + 12) @(negedge clock);
            if (b_handshakes - b_before != 1)
                record_violation("B_RESPONSE_COUNT", $sformatf("expected=1 observed=%0d", b_handshakes - b_before));
            if (memory.m_utils.write_calls - calls_before != 3)
                record_violation("W_COMMIT_COUNT", $sformatf("expected=3 observed=%0d", memory.m_utils.write_calls - calls_before));
            if (memory.m_utils.words[(32'h0000_2080 >> 2) % 4096] !== 32'h1111_0001 ||
                memory.m_utils.words[(32'h0000_2084 >> 2) % 4096] !== 32'h2222_0002 ||
                memory.m_utils.words[(32'h0000_2088 >> 2) % 4096] !== 32'h3333_0003)
                record_violation("W_DATA_COMMIT_SEQUENCE", "final memory does not contain three distinct beat values");
        end
    endtask

    function automatic [31:0] queued_write_data(input integer beat);
        queued_write_data = 32'h7100_0000 + beat;
    endfunction

    function automatic [3:0] queued_write_strobe(input integer beat);
        queued_write_strobe = (beat % 15) + 1;
    endfunction

    task automatic send_aw_with_id(input logic [0:0] id, input logic [31:0] address, input integer beats);
        begin
            @(negedge clock);
            awid = id;
            awaddr = address;
            awlen = beats - 1;
            awvalid = 1;
            do @(posedge clock); while (!awready);
            @(negedge clock);
            awvalid = 0;
        end
    endtask

    task automatic send_aw_with_blocked_w(input logic [31:0] address, input integer beats, input integer beat);
        logic aw_accepted;
        logic w_accepted;
        integer elapsed;
        begin
            @(negedge clock);
            awid = 0;
            awaddr = address;
            awlen = beats - 1;
            awvalid = 1;
            wdata = queued_write_data(beat);
            wstrb = queued_write_strobe(beat);
            wlast = 0;
            wvalid = 1;
            elapsed = 0;
            aw_accepted = 0;
            w_accepted = 0;
            while (!aw_accepted && elapsed < 1000) begin
                @(posedge clock);
                if (awvalid && awready) aw_accepted = 1;
                if (wvalid && wready) w_accepted = 1;
                if (wvalid && !wready) queued_w_stall_cycles = queued_w_stall_cycles + 1;
                elapsed = elapsed + 1;
            end
            if (!aw_accepted) record_violation("QUEUED_FIRST_AW_TIMEOUT", "AW not accepted while W queue full");
            @(negedge clock);
            awvalid = 0;
            while (!w_accepted && elapsed < 1000) begin
                @(posedge clock);
                if (wvalid && wready) w_accepted = 1;
                if (wvalid && !wready) queued_w_stall_cycles = queued_w_stall_cycles + 1;
                elapsed = elapsed + 1;
            end
            if (!w_accepted) record_violation("QUEUED_BLOCKED_W_TIMEOUT", $sformatf("beat=%0d", beat));
            @(negedge clock);
            wvalid = 0;
        end
    endtask

    task automatic send_queued_w(input integer beat, input logic last);
        integer elapsed;
        logic accepted;
        begin
            @(negedge clock);
            wdata = queued_write_data(beat);
            wstrb = queued_write_strobe(beat);
            wlast = last;
            wvalid = 1;
            elapsed = 0;
            accepted = 0;
            while (!accepted && elapsed < 1000) begin
                @(posedge clock);
                accepted = wready;
                if (!wready) queued_w_stall_cycles = queued_w_stall_cycles + 1;
                elapsed = elapsed + 1;
            end
            if (!accepted) record_violation("QUEUED_W_HANDSHAKE_TIMEOUT", $sformatf("beat=%0d", beat));
            @(negedge clock);
            wvalid = 0;
        end
    endtask

    task automatic run_queued_write_backpressure_case(input logic stall_bresponse);
        localparam integer FIRST_BURST_BEATS = 66;
        localparam integer SECOND_BURST_BEATS = 2;
        integer calls_before;
        integer aw_before;
        integer w_before;
        integer b_before;
        integer observed_b_before;
        integer observed_b_before_w;
        integer observed_b_during_w;
        integer beat;
        integer elapsed;
        integer stalls_seen;
        integer response_seen;
        logic [0:0] held_bid;
        logic [1:0] held_bresp, held_buser;
        logic b_payload_changed;
        begin
            reset_case();
            bready = !stall_bresponse;
            calls_before = memory.m_utils.write_calls;
            aw_before = aw_handshakes;
            w_before = w_handshakes;
            b_before = b_handshakes;
            observed_b_before = observed_b_count;
            observed_b_before_w = observed_b_count;
            queued_w_stall_cycles = 0;
            for (beat = 0; beat < MODEL_C_W; beat = beat + 1) begin
                send_queued_w(beat, beat == FIRST_BURST_BEATS - 1);
            end
            send_aw_with_blocked_w(32'h0000_3000, FIRST_BURST_BEATS, MODEL_C_W);
            send_aw_with_id(1'b1, 32'h0000_4000, SECOND_BURST_BEATS);
            for (beat = MODEL_C_W + 1; beat < FIRST_BURST_BEATS; beat = beat + 1)
                send_queued_w(beat, beat == FIRST_BURST_BEATS - 1);
            for (beat = 0; beat < SECOND_BURST_BEATS; beat = beat + 1)
                send_queued_w(FIRST_BURST_BEATS + beat, beat == SECOND_BURST_BEATS - 1);
            observed_b_during_w = observed_b_count - observed_b_before_w;
            if (aw_handshakes - aw_before != 2 || w_handshakes - w_before != FIRST_BURST_BEATS + SECOND_BURST_BEATS)
                record_violation("QUEUED_WRITE_HANDSHAKE_COUNTS", $sformatf("aw=%0d w=%0d", aw_handshakes-aw_before, w_handshakes-w_before));
            stalls_seen = queued_w_stall_cycles;
            if (stalls_seen == 0)
                record_violation("QUEUED_WRITE_BACKPRESSURE_NOT_EXERCISED", $sformatf("C_W=%0d", MODEL_C_W));
            elapsed = 0;
            if (stall_bresponse) begin
                while ((!bvalid || memory.m_utils.write_calls - calls_before != FIRST_BURST_BEATS + SECOND_BURST_BEATS) && elapsed < 1000) begin
                    @(negedge clock);
                    elapsed = elapsed + 1;
                end
                if (!bvalid) record_violation("QUEUED_WRITE_BVALID_MISSING", "no response for first outstanding burst");
                held_bid = bid;
                held_bresp = bresp;
                held_buser = buser;
                b_payload_changed = 0;
                repeat (5) begin
                    @(negedge clock);
                    if (!bvalid || {bid, bresp, buser} !== {held_bid, held_bresp, held_buser})
                        b_payload_changed = 1;
                end
                if (held_bid !== 1'b0 || held_bresp !== 2'b00 || held_buser !== 2'b00 || b_payload_changed)
                    record_violation("QUEUED_WRITE_B_STABILITY", $sformatf("id=%b resp=%b user=%b changed=%0d", held_bid, held_bresp, held_buser, b_payload_changed));
                bready = 1;
            end
            elapsed = 0;
            while (observed_b_count - observed_b_before < 2 && elapsed < 1000) begin
                @(negedge clock);
                elapsed = elapsed + 1;
            end
            @(negedge clock);
            if (b_handshakes - b_before != 2 || observed_b_count - observed_b_before != 2)
                record_violation("QUEUED_WRITE_B_COUNT", $sformatf("expected=2 observed=%0d logged=%0d", b_handshakes-b_before, observed_b_count-observed_b_before));
            if (observed_bid[observed_b_before] !== 1'b0 || observed_bid[observed_b_before+1] !== 1'b1)
                record_violation("QUEUED_WRITE_BID_ORDER", $sformatf("first=%b second=%b", observed_bid[observed_b_before], observed_bid[observed_b_before+1]));
            if (memory.m_utils.write_calls - calls_before != FIRST_BURST_BEATS + SECOND_BURST_BEATS)
                record_violation("QUEUED_WRITE_COMMIT_COUNT", $sformatf("expected=%0d observed=%0d", FIRST_BURST_BEATS+SECOND_BURST_BEATS, memory.m_utils.write_calls-calls_before));
            for (beat = 0; beat < FIRST_BURST_BEATS + SECOND_BURST_BEATS; beat = beat + 1) begin
                if (beat < FIRST_BURST_BEATS) begin
                    if (memory.m_utils.write_addr[calls_before+beat] !== 32'h0000_3000 + beat*4 ||
                        memory.m_utils.write_data[calls_before+beat] !== queued_write_data(beat) ||
                        memory.m_utils.write_mask[calls_before+beat] !== queued_write_strobe(beat))
                        record_violation("QUEUED_WRITE_BACKEND_ORDER", $sformatf("entry=%0d addr=%h data=%h strb=%h", beat, memory.m_utils.write_addr[calls_before+beat], memory.m_utils.write_data[calls_before+beat], memory.m_utils.write_mask[calls_before+beat]));
                end else begin
                    if (memory.m_utils.write_addr[calls_before+beat] !== 32'h0000_4000 + (beat-FIRST_BURST_BEATS)*4 ||
                        memory.m_utils.write_data[calls_before+beat] !== queued_write_data(beat) ||
                        memory.m_utils.write_mask[calls_before+beat] !== queued_write_strobe(beat))
                        record_violation("QUEUED_WRITE_BACKEND_ORDER", $sformatf("entry=%0d addr=%h data=%h strb=%h", beat, memory.m_utils.write_addr[calls_before+beat], memory.m_utils.write_data[calls_before+beat], memory.m_utils.write_mask[calls_before+beat]));
                end
            end
            if (stall_bresponse)
                $display("F0_OBSERVED QUEUED_WRITE_BACKPRESSURE_BREADY_LOW beats=%0d C_W=%0d aw_before_w_complete=1 stalls=%0d", FIRST_BURST_BEATS+SECOND_BURST_BEATS, MODEL_C_W, stalls_seen);
            else
                $display("F0_OBSERVED QUEUED_WRITE_BACKPRESSURE_BREADY_HIGH beats=%0d C_W=%0d aw_before_w_complete=1 stalls=%0d b_during_w=%0d", FIRST_BURST_BEATS+SECOND_BURST_BEATS, MODEL_C_W, stalls_seen, observed_b_during_w);
        end
    endtask

    task automatic run_w_before_aw_case;
        integer calls_before;
        begin
            reset_case();
            calls_before = memory.m_utils.write_calls;
            memory.m_utils.set_word(32'h0000_2200, 32'h0000_0000);
            send_w(32'hfeed_cafe, 1'b1);
            send_aw(32'h0000_2200, 1);
            repeat (WRITE_DELAY + 4) @(negedge clock);
            check_condition(memory.m_utils.words[(32'h0000_2200 >> 2) % 4096] !== 32'hfeed_cafe ||
                            memory.m_utils.write_calls - calls_before != 1,
                "W_BEFORE_AW_ASSOCIATION",
                $sformatf("commits=%0d value=%h", memory.m_utils.write_calls - calls_before,
                          memory.m_utils.words[(32'h0000_2200 >> 2) % 4096]));
        end
    endtask

    task automatic send_aw_w_together(input logic [31:0] address, input logic [31:0] data);
        begin
            @(negedge clock);
            awaddr = address;
            awlen = 0;
            awvalid = 1;
            wdata = data;
            wstrb = 4'hf;
            wlast = 1;
            wvalid = 1;
            do @(posedge clock); while (!(awready && wready));
            @(negedge clock);
            awvalid = 0;
            wvalid = 0;
        end
    endtask

    task automatic run_write_timing_case(input integer order);
        integer calls_before;
        integer b_before;
        integer elapsed;
        time expected_commit_time;
        begin
            reset_case();
            bready = 1;
            calls_before = memory.m_utils.write_calls;
            b_before = b_handshakes;
            case (order)
                0: send_aw_w_together(32'h0000_2400, 32'hface_0000);
                1: begin
                    send_aw(32'h0000_2410, 1);
                    send_w(32'hface_0001, 1);
                end
                2: begin
                    send_w(32'hface_0002, 1);
                    send_aw(32'h0000_2420, 1);
                end
                default: $fatal(1, "invalid AW/W timing order %0d", order);
            endcase
            elapsed = 0;
            while (memory.m_utils.write_calls == calls_before && elapsed < WRITE_DELAY + 8) begin
                @(negedge clock);
                elapsed = elapsed + 1;
            end
            if (memory.m_utils.write_calls != calls_before + 1) begin
                record_violation("W_TIMING_COMMIT_COUNT", $sformatf("order=%0d calls=%0d", order,
                    memory.m_utils.write_calls - calls_before));
            end else begin
                expected_commit_time = (last_aw_time > last_w_time ? last_aw_time : last_w_time) +
                                       (WRITE_DELAY - 1) * 10;
                if (memory.m_utils.write_time[calls_before] != expected_commit_time)
                    record_violation("W_TIMING_COMMIT_EDGE", $sformatf("order=%0d aw=%0t w=%0t commit=%0t expected=%0t delay=%0d",
                        order, last_aw_time, last_w_time, memory.m_utils.write_time[calls_before],
                        expected_commit_time, WRITE_DELAY));
                elapsed = 0;
                while (b_handshakes == b_before && elapsed < 8) begin
                    @(negedge clock);
                    elapsed = elapsed + 1;
                end
                if (b_handshakes != b_before + 1 || last_b_time != memory.m_utils.write_time[calls_before] + 10)
                    record_violation("B_AFTER_COMMIT_EDGE", $sformatf("order=%0d commit=%0t b=%0t count=%0d",
                        order, memory.m_utils.write_time[calls_before], last_b_time, b_handshakes - b_before));
                if (mode_qualify)
                    $display("F0_WRITE_TIMING_PASS order=%0d delay=%0d aw=%0t w=%0t commit=%0t b=%0t",
                        order, WRITE_DELAY, last_aw_time, last_w_time,
                        memory.m_utils.write_time[calls_before], last_b_time);
            end
        end
    endtask

    task automatic run_negative_case(input string test_case);
        begin
            reset_case();
            $display("F0_NEGATIVE_START case=%s delay=%0d", test_case, WRITE_DELAY);
            case (test_case)
                "early_wlast": begin
                    send_aw(32'h0000_2500, 2);
                    send_w(32'hbad0_0000, 1'b1);
                end
                "missing_final_wlast": begin
                    send_aw(32'h0000_2510, 2);
                    send_w(32'hbad1_0000, 1'b0);
                    send_w(32'hbad1_0001, 1'b0);
                end
                "read_wrap": begin
                    arburst = 2'b10;
                    send_ar(32'h0000_2520, 2);
                end
                "read_reserved": begin
                    arburst = 2'b11;
                    send_ar(32'h0000_2528, 2);
                end
                "write_wrap": begin
                    awburst = 2'b10;
                    send_aw(32'h0000_2530, 2);
                    send_w(32'hbad3_0000, 1'b0);
                    send_w(32'hbad3_0001, 1'b1);
                end
                "write_reserved": begin
                    awburst = 2'b11;
                    send_aw(32'h0000_2538, 2);
                    send_w(32'hbad4_0000, 1'b0);
                    send_w(32'hbad4_0001, 1'b1);
                end
                default: $fatal(1, "unknown negative test case: %s", test_case);
            endcase
            repeat (READ_DELAY + WRITE_DELAY + 8) @(negedge clock);
            $fatal(1, "F0_NEGATIVE_DID_NOT_ABORT case=%s delay=%0d", test_case, WRITE_DELAY);
        end
    endtask

    initial begin : characterize_model
        integer failures_before;
        if ($test$plusargs("MODE=qualify")) mode_qualify = 1;
        if ($value$plusargs("NEGATIVE=%s", negative_case)) begin
            if (QUEUE_SIZE < 2 || READ_DELAY < 1 || WRITE_DELAY < 1)
                $fatal(1, "invalid fixture parameters");
            run_negative_case(negative_case);
            disable characterize_model;
        end
        if (QUEUE_SIZE < 2 || READ_DELAY < 1 || WRITE_DELAY < 1)
            $fatal(1, "invalid fixture parameters");
        failures_before = failure_count;

        run_multi_beat_drain_case();
        run_read_case(4, 0, 0);
        run_read_case(4, 1, 0);
        run_read_case(4, 3, 1);
        run_read_case(1, 0, 0);
        run_write_case();
        run_w_before_aw_case();

        if (mode_qualify) begin
            run_queued_write_backpressure_case(1'b1);
            run_queued_write_backpressure_case(1'b0);
            run_write_timing_case(0);
            run_write_timing_case(1);
            run_write_timing_case(2);
            if (unexpected_count != 0)
                $fatal(1, "F0_QUALIFY_FAIL failures=%0d", unexpected_count);
            $display("F0_QUALIFY_PASS read=%0d write=%0d", r_handshakes, memory.m_utils.write_calls);
        end else begin
            if (failure_count == failures_before)
                $fatal(1, "F0_CHARACTERIZATION_FOUND_NO_EXPECTED_DEFECTS");
            $display("F0_CHARACTERIZATION_EXPECTED_DEFECTS count=%0d", failure_count);
        end
        $finish;
    end

    initial begin
        #200000;
        $fatal(1, "F0 fixture watchdog expired");
    end
endmodule
