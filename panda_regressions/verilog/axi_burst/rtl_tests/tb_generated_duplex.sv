`timescale 1ns / 1ps
module tb_generated_duplex #(
    parameter integer ADDRESS_BITS = 32,
    parameter integer READ_B_MAX = 16,
    parameter integer READ_OUTSTANDING = 1,
    parameter integer READ_FIFO_DEPTH = 32,
    parameter integer WRITE_B_MAX = 16,
    parameter integer WRITE_OUTSTANDING = 1,
    parameter integer WRITE_FIFO_DEPTH = 16
);
    localparam logic [1:0] OP_LOAD = 2'd0;
    localparam logic [1:0] OP_STORE = 2'd1;
    localparam logic [1:0] OP_CONFIGURE = 2'd2;
    localparam logic [1:0] OP_DRAIN = 2'd3;
    localparam integer WORDS = 4096;
    localparam logic [ADDRESS_BITS-1:0] READ_BASE = ADDRESS_BITS == 64 ? 64'h0000000100000100 : ADDRESS_BITS'(32'h00000100);
    localparam logic [ADDRESS_BITS-1:0] WRITE_BASE = ADDRESS_BITS == 64 ? 64'h0000000100000400 : ADDRESS_BITS'(32'h00000400);

    logic clock = 1'b0;
    always #5 clock = ~clock;
    logic reset = 1'b0;
    logic cache_reset = 1'b0;
    logic read_start = 1'b0;
    logic write_start = 1'b0;
    logic [1:0] read_opcode = OP_CONFIGURE;
    logic [1:0] write_opcode = OP_CONFIGURE;
    logic [31:0] read_size = 32;
    logic [31:0] read_data = 0;
    logic [ADDRESS_BITS-1:0] read_address = 0;
    logic [31:0] read_count = 0;
    logic [31:0] write_size = 32;
    logic [31:0] write_data = 0;
    logic [ADDRESS_BITS-1:0] write_address = 0;
    logic [31:0] write_count = 0;
    wire read_done, write_done;
    wire [31:0] read_result, write_result;
    wire core_read_start, core_write_start;
    wire [1:0] core_read_opcode, core_write_opcode;
    wire [31:0] core_read_size, core_read_data, core_read_count;
    wire [ADDRESS_BITS-1:0] core_read_address;
    wire [31:0] core_write_size, core_write_data, core_write_count;
    wire [ADDRESS_BITS-1:0] core_write_address;
    wire core_read_done, core_write_done;
    wire [31:0] core_read_result, core_write_result;

    wire [5:0] awid, arid;
    wire [ADDRESS_BITS-1:0] awaddr, araddr;
    wire [31:0] wdata, rdata;
    wire [7:0] awlen, arlen;
    wire [2:0] awsize, arsize;
    wire [1:0] awburst, arburst;
    wire [3:0] awcache, arcache;
    wire [3:0] wstrb;
    wire wlast, awvalid, wvalid, bready, arvalid, rready;
    logic awready, wready, bvalid;
    logic arready, rvalid;
    wire rlast;
    logic [5:0] bid = 0, rid = 0;
    logic [1:0] bresp = 0, rresp = 0;

    shared_bambu_artificial_ParmMgr_duplex_read_endpoint read_endpoint (
        .clock(clock), .reset(reset), .start_port(read_start), .in1(read_opcode),
        .in2(read_size), .in3(read_data), .in4(read_address), .in5(read_count),
        .done_port(read_done), .out1(read_result), .duplex_start(core_read_start),
        .duplex_opcode(core_read_opcode), .duplex_size(core_read_size),
        .duplex_data(core_read_data), .duplex_address(core_read_address),
        .duplex_count(core_read_count), .duplex_done(core_read_done),
        .duplex_result(core_read_result)
    );

    shared_bambu_artificial_ParmMgr_duplex_write_endpoint write_endpoint (
        .clock(clock), .reset(reset), .start_port(write_start), .in1(write_opcode),
        .in2(write_size), .in3(write_data), .in4(write_address), .in5(write_count),
        .done_port(write_done), .out1(write_result), .duplex_start(core_write_start),
        .duplex_opcode(core_write_opcode), .duplex_size(core_write_size),
        .duplex_data(core_write_data), .duplex_address(core_write_address),
        .duplex_count(core_write_count), .duplex_done(core_write_done),
        .duplex_result(core_write_result)
    );

    shared_bambu_artificial_ParmMgr_duplex_aggregator_modgen aggregator (
        .clock(clock), .reset(reset), .cache_reset(cache_reset),
        .read_start(core_read_start), .read_opcode(core_read_opcode),
        .read_size(core_read_size), .read_data(core_read_data),
        .read_address(core_read_address), .read_count(core_read_count),
        .read_done(core_read_done), .read_result(core_read_result),
        .write_start(core_write_start), .write_opcode(core_write_opcode),
        .write_size(core_write_size), .write_data(core_write_data),
        .write_address(core_write_address), .write_count(core_write_count),
        .write_done(core_write_done), .write_result(core_write_result),
        .p_m_axi_shared_awready(awready), .p_m_axi_shared_wready(wready),
        .p_m_axi_shared_bid(bid), .p_m_axi_shared_bresp(bresp),
        .p_m_axi_shared_buser(1'b0), .p_m_axi_shared_bvalid(bvalid),
        .p_m_axi_shared_arready(arready), .p_m_axi_shared_rid(rid),
        .p_m_axi_shared_rdata(rdata), .p_m_axi_shared_rresp(rresp),
        .p_m_axi_shared_rlast(rlast), .p_m_axi_shared_ruser(1'b0),
        .p_m_axi_shared_rvalid(rvalid), .p_m_axi_shared_awid(awid),
        .p_m_axi_shared_awaddr(awaddr), .p_m_axi_shared_awlen(awlen),
        .p_m_axi_shared_awsize(awsize), .p_m_axi_shared_awburst(awburst),
        .p_m_axi_shared_awlock(), .p_m_axi_shared_awcache(awcache),
        .p_m_axi_shared_awprot(), .p_m_axi_shared_awqos(),
        .p_m_axi_shared_awregion(), .p_m_axi_shared_awuser(),
        .p_m_axi_shared_awvalid(awvalid), .p_m_axi_shared_wdata(wdata),
        .p_m_axi_shared_wstrb(wstrb), .p_m_axi_shared_wlast(wlast),
        .p_m_axi_shared_wuser(), .p_m_axi_shared_wvalid(wvalid),
        .p_m_axi_shared_bready(bready), .p_m_axi_shared_arid(arid),
        .p_m_axi_shared_araddr(araddr), .p_m_axi_shared_arlen(arlen),
        .p_m_axi_shared_arsize(arsize), .p_m_axi_shared_arburst(arburst),
        .p_m_axi_shared_arlock(), .p_m_axi_shared_arcache(arcache),
        .p_m_axi_shared_arprot(), .p_m_axi_shared_arqos(),
        .p_m_axi_shared_arregion(), .p_m_axi_shared_aruser(),
        .p_m_axi_shared_arvalid(arvalid), .p_m_axi_shared_rready(rready)
    );

    logic [31:0] memory [0:WORDS-1];
    logic [31:0] expected [0:WORDS-1];
    logic expected_valid [0:WORDS-1];
    logic [ADDRESS_BITS-1:0] ar_queue_addr [0:255];
    integer ar_queue_left [0:255];
    integer ar_wr = 0, ar_rd = 0;
    logic [ADDRESS_BITS-1:0] aw_queue_addr [0:255];
    integer aw_queue_left [0:255];
    integer aw_wr = 0, aw_rd = 0;
    integer w_queue_data [0:4095];
    logic w_queue_last [0:4095];
    integer w_wr = 0, w_rd = 0;
    integer read_beats = 0, accepted_ar = 0, accepted_aw = 0;
    integer accepted_w = 0, committed_w = 0, accepted_b = 0;
    integer ar_outstanding = 0, aw_outstanding = 0;
    integer peak_ar_outstanding = 0, peak_aw_outstanding = 0;
    integer b_pending = 0;
    integer simultaneous_rw = 0;
    integer simultaneous_done = 0;
    integer fault_r_seen = 0, fault_b_seen = 0;
    integer accepted_load_ops = 0;
    logic b_fault_read_success = 0;
    logic last_load_immediate = 0;
    logic [31:0] last_load_value = 0;
    logic stall_ar = 0, stall_r = 0, stall_aw = 0, stall_w = 0, stall_b = 0;
    logic prev_ar_stalled = 0, prev_aw_stalled = 0, prev_w_stalled = 0;
    logic [ADDRESS_BITS+18:0] prev_ar_payload, prev_aw_payload;
    logic [36:0] prev_w_payload;
    integer i;
    logic [31:0] rdata_model;

    function automatic integer memory_index(input logic [ADDRESS_BITS-1:0] address);
        memory_index = integer'(address[13:2]);
    endfunction

    always_comb
    begin
        arready = !stall_ar;
        awready = !stall_aw;
        wready = !stall_w;
        rvalid = (ar_rd < ar_wr) && !stall_r;
        rdata_model = rvalid ? memory[memory_index(ar_queue_addr[ar_rd])] : 32'b0;
        bvalid = (b_pending > 0) && !stall_b;
    end
    assign rdata = rdata_model;
    assign rlast = rvalid && ar_queue_left[ar_rd] == 1;
    assign rid = 6'b0;
    assign bid = 6'b0;

    always @(posedge clock)
    begin
        integer k;
        integer beat_count;
        if (!reset)
        begin
            ar_wr = 0; ar_rd = 0; aw_wr = 0; aw_rd = 0; w_wr = 0; w_rd = 0;
            read_beats = 0; accepted_ar = 0; accepted_aw = 0; accepted_w = 0;
            committed_w = 0; accepted_b = 0; b_pending = 0; simultaneous_rw = 0; simultaneous_done = 0;
            ar_outstanding = 0; aw_outstanding = 0;
            peak_ar_outstanding = 0; peak_aw_outstanding = 0;
            fault_r_seen = 0; fault_b_seen = 0;
            accepted_load_ops = 0; b_fault_read_success = 0;
            prev_ar_stalled = 0; prev_aw_stalled = 0; prev_w_stalled = 0;
        end
        else
        begin
            if (core_read_start && core_read_opcode == OP_LOAD)
                accepted_load_ops = accepted_load_ops + 1;
            if (fault_b_seen != 0 && read_done)
                b_fault_read_success = 1;
            if (bvalid && bready && bresp !== 2'b00 && read_done)
                b_fault_read_success = 1;
            if (prev_ar_stalled && (!arvalid || {arid,araddr,arlen,arsize,arburst} !== prev_ar_payload))
                $fatal(1, "GENERATED_AR_CHANGED_WHILE_STALLED");
            if (prev_aw_stalled && (!awvalid || {awid,awaddr,awlen,awsize,awburst} !== prev_aw_payload))
                $fatal(1, "GENERATED_AW_CHANGED_WHILE_STALLED");
            if (prev_w_stalled && (!wvalid || {wdata,wstrb,wlast} !== prev_w_payload))
                $fatal(1, "GENERATED_W_CHANGED_WHILE_STALLED");
            prev_ar_stalled = arvalid && !arready;
            prev_aw_stalled = awvalid && !awready;
            prev_w_stalled = wvalid && !wready;
            if (prev_ar_stalled) prev_ar_payload = {arid,araddr,arlen,arsize,arburst};
            if (prev_aw_stalled) prev_aw_payload = {awid,awaddr,awlen,awsize,awburst};
            if (prev_w_stalled) prev_w_payload = {wdata,wstrb,wlast};
            if (rvalid && rready && wvalid && wready)
                simultaneous_rw = simultaneous_rw + 1;
            if (read_done && write_done)
                simultaneous_done = simultaneous_done + 1;
            if (arvalid && arready)
            begin
                if (arsize !== 3'd2 || arburst !== 2'b01 || arid !== 0 || arcache !== 4'b0010)
                    $fatal(1, "GENERATED_AR_PROFILE_INVALID");
                if (ar_outstanding + 1 - ((rvalid && rready && ar_queue_left[ar_rd] == 1) ? 1 : 0) > READ_OUTSTANDING)
                    $fatal(1, "GENERATED_AR_OUTSTANDING_EXCEEDED");
                if (ar_outstanding + 1 - ((rvalid && rready && ar_queue_left[ar_rd] == 1) ? 1 : 0) > peak_ar_outstanding)
                    peak_ar_outstanding = ar_outstanding + 1 - ((rvalid && rready && ar_queue_left[ar_rd] == 1) ? 1 : 0);
                ar_queue_addr[ar_wr] = araddr;
                ar_queue_left[ar_wr] = integer'(arlen) + 1;
                ar_wr = ar_wr + 1;
                accepted_ar = accepted_ar + 1;
                ar_outstanding = ar_outstanding + 1;
            end
            if (rvalid && rready)
            begin
                if (rresp !== 2'b00) fault_r_seen = fault_r_seen + 1;
                read_beats = read_beats + 1;
                if (ar_queue_left[ar_rd] == 1)
                begin
                    ar_rd = ar_rd + 1;
                    ar_outstanding = ar_outstanding - 1;
                end
                else
                begin
                    ar_queue_left[ar_rd] = ar_queue_left[ar_rd] - 1;
                    ar_queue_addr[ar_rd] = ar_queue_addr[ar_rd] + ADDRESS_BITS'(4);
                end
            end
            if (awvalid && awready)
            begin
                if (awsize !== 3'd2 || awburst !== 2'b01 || awid !== 0 || awcache !== 4'b0010)
                    $fatal(1, "GENERATED_AW_PROFILE_INVALID");
                if (aw_outstanding + 1 - ((bvalid && bready) ? 1 : 0) > WRITE_OUTSTANDING)
                    $fatal(1, "GENERATED_AW_OUTSTANDING_EXCEEDED");
                if (aw_outstanding + 1 - ((bvalid && bready) ? 1 : 0) > peak_aw_outstanding)
                    peak_aw_outstanding = aw_outstanding + 1 - ((bvalid && bready) ? 1 : 0);
                aw_queue_addr[aw_wr] = awaddr;
                aw_queue_left[aw_wr] = integer'(awlen) + 1;
                aw_wr = aw_wr + 1;
                accepted_aw = accepted_aw + 1;
                aw_outstanding = aw_outstanding + 1;
            end
            if (wvalid && wready)
            begin
                if (wstrb !== 4'b1111) $fatal(1, "GENERATED_WSTRB_INVALID");
                w_queue_data[w_wr] = wdata;
                w_queue_last[w_wr] = wlast;
                w_wr = w_wr + 1;
                accepted_w = accepted_w + 1;
            end
            if (aw_rd < aw_wr && w_wr - w_rd >= aw_queue_left[aw_rd])
            begin
                beat_count = aw_queue_left[aw_rd];
                for (k = 0; k < beat_count; k = k + 1)
                begin
                    if (w_queue_last[w_rd+k] !== (k == beat_count-1))
                        $fatal(1, "GENERATED_WLAST_INVALID");
                    if (memory_index(aw_queue_addr[aw_rd])+k < 0 || memory_index(aw_queue_addr[aw_rd])+k >= WORDS)
                        $fatal(1, "GENERATED_WRITE_ADDRESS_OUT_OF_RANGE");
                    if (!expected_valid[memory_index(aw_queue_addr[aw_rd])+k] ||
                        expected[memory_index(aw_queue_addr[aw_rd])+k] !== w_queue_data[w_rd+k])
                        $fatal(1, "GENERATED_WRITE_DATA_MISMATCH word=%0d", memory_index(aw_queue_addr[aw_rd])+k);
                    memory[memory_index(aw_queue_addr[aw_rd])+k] = w_queue_data[w_rd+k];
                end
                w_rd = w_rd + beat_count;
                aw_rd = aw_rd + 1;
                committed_w = committed_w + beat_count;
                b_pending = b_pending + 1;
            end
            if (bvalid && bready)
            begin
                if (bresp !== 2'b00) fault_b_seen = fault_b_seen + 1;
                accepted_b = accepted_b + 1;
                b_pending = b_pending - 1;
                aw_outstanding = aw_outstanding - 1;
            end
        end
    end

    task automatic tick(input integer n);
        repeat (n) @(negedge clock);
    endtask

    task automatic wait_read_done;
        integer limit;
        begin
            limit = 0;
            while (!read_done && limit < 10000)
            begin
              @(negedge clock); limit = limit + 1;
            end
            if (!read_done) $fatal(1, "GENERATED_READ_TIMEOUT");
        end
    endtask

    task automatic wait_write_done;
        integer limit;
        begin
            limit = 0;
            while (!write_done && limit < 10000)
            begin
              @(negedge clock); limit = limit + 1;
            end
            if (!write_done) $fatal(1, "GENERATED_WRITE_TIMEOUT");
        end
    endtask

    task automatic configure(input bit is_read, input logic [ADDRESS_BITS-1:0] base, input logic [31:0] count);
        begin
            @(negedge clock);
            if (is_read)
            begin
                read_opcode = OP_CONFIGURE; read_size = 32; read_address = base; read_count = count; read_start = 1;
                @(posedge clock); @(negedge clock); read_start = 0;
            end
            else
            begin
                write_opcode = OP_CONFIGURE; write_size = 32; write_address = base; write_count = count; write_start = 1;
                @(posedge clock); @(negedge clock); write_start = 0;
            end
        end
    endtask

    task automatic load_word(input logic [ADDRESS_BITS-1:0] address);
        logic [31:0] expected_read;
        logic [31:0] captured_read;
        bit completed;
        integer limit;
        begin
            expected_read = memory[memory_index(address)];
            @(negedge clock);
            // A preceding CONFIGURE can still expose its acknowledgement.
            // Let it retire before interpreting done as a LOAD result.
            while (read_done) @(negedge clock);
            read_opcode = OP_LOAD; read_size = 32; read_address = address; read_start = 1;
            #1;
            completed = read_done;
            last_load_immediate = completed;
            if (completed) captured_read = read_result;
            if (!completed)
            begin
                @(posedge clock); #1;
                completed = read_done;
                if (completed) captured_read = read_result;
            end
            if (!completed)
            begin
                @(negedge clock); read_start = 0;
                limit = 0;
                while (!read_done && limit < 10000)
                begin
                  @(negedge clock); #1; limit = limit + 1;
                end
                if (!read_done) $fatal(1, "GENERATED_READ_TIMEOUT");
                captured_read = read_result;
                @(posedge clock); @(negedge clock);
            end
            else
            begin
                @(posedge clock); @(negedge clock); read_start = 0;
            end
            if (captured_read !== expected_read)
                $fatal(1, "GENERATED_READ_DATA_MISMATCH address=%h got=%h expected=%h", address, captured_read, expected_read);
            last_load_value = captured_read;
        end
    endtask

    task automatic store_word(input logic [ADDRESS_BITS-1:0] address, input logic [31:0] value);
        integer limit;
        begin
            expected[memory_index(address)] = value; expected_valid[memory_index(address)] = 1;
            @(negedge clock); write_opcode = OP_STORE; write_size = 32; write_address = address; write_data = value; write_start = 1;
            // STORE can acknowledge combinationally. Sample after the driven
            // operands settle, then keep the command through its accepting edge.
            #1;
            limit = 0;
            while (!write_done && limit < 10000)
            begin
              @(negedge clock); #1; limit = limit + 1;
            end
            if (!write_done) $fatal(1, "GENERATED_STORE_ACK_TIMEOUT");
            @(posedge clock); @(negedge clock); write_start = 0;
        end
    endtask

    task automatic drain;
        begin
            @(negedge clock); write_opcode = OP_DRAIN; write_start = 1;
            @(posedge clock); @(negedge clock); write_start = 0;
            wait_write_done();
        end
    endtask

    task automatic verify_word(input integer word_index);
        if (!expected_valid[word_index] || memory[word_index] !== expected[word_index])
            $fatal(1, "GENERATED_MEMORY_MISMATCH index=%0d got=%h expected=%h",
                   word_index, memory[word_index], expected[word_index]);
    endtask

    task automatic verify_memory;
        integer word_index;
        begin
            for (word_index=0;word_index<WORDS;word_index=word_index+1)
            begin
                if (expected_valid[word_index] && memory[word_index] !== expected[word_index])
                    $fatal(1, "GENERATED_MEMORY_MISMATCH index=%0d", word_index);
                if (!expected_valid[word_index] && memory[word_index] !== (32'h5a5a0000 ^ word_index))
                    $fatal(1, "GENERATED_CANARY_CORRUPTED index=%0d", word_index);
            end
        end
    endtask

    task automatic run_case(input string name);
        integer before_ar, before_aw, before_r, before_rw, before_done, t, expected_write_beats, expected_b_responses;
        logic [31:0] saved_payload;
        logic [ADDRESS_BITS-1:0] saved_address;
        logic [31:0] initial_words [0:7];
        begin
            expected_write_beats = 0;
            expected_b_responses = 0;
            case (name)
                "LOAD_WAIT_STORE":
                begin
                    expected_write_beats=4; expected_b_responses=1;
                    stall_r = 1; configure(1, READ_BASE, 4); configure(0, WRITE_BASE, 4);
                    @(negedge clock); read_opcode = OP_LOAD; read_address = READ_BASE; read_start = 1;
                    for (t=0;t<4;t=t+1) store_word(WRITE_BASE+t*4, 32'h11000000+t);
                    if (read_done || read_beats != 0 || accepted_aw != 0)
                        $fatal(1, "GENERATED_LOAD_WAIT_STORE_INVALID");
                    drain();
                    if (committed_w != 4 || accepted_b != 1 || !write_done || read_done)
                        $fatal(1, "GENERATED_STORE_DRAIN_DID_NOT_PROGRESS_WHILE_READ_STALLED");
                    stall_r = 0; wait_read_done(); if (read_result !== memory[64]) $fatal(1, "GENERATED_LOAD_RESUME_MISMATCH");
                    read_start = 0;
                end
                "STORE_WAIT_LOAD":
                begin
                    expected_write_beats=17; expected_b_responses=2;
                    stall_aw = 1; stall_w = 1; configure(0, WRITE_BASE, 17); configure(1, READ_BASE, 1);
                    before_r=read_beats;
                    t=0; while (read_beats==before_r && t<2000)
                    begin
                      @(negedge clock); t=t+1;
                    end
                    if (read_beats!=before_r+1 || read_done)
                        $fatal(1, "GENERATED_READ_FIFO_PREFETCH_NOT_AVAILABLE");
                    for (t=0;t<16;t=t+1) store_word(WRITE_BASE+t*4, 32'h12000000+t);
                    repeat (3) @(negedge clock);
                    if (!awvalid || !wvalid) $fatal(1, "GENERATED_STORE_WAIT_NOT_STALLED");
                    saved_payload=wdata; saved_address=awaddr;
                    repeat (3)
                    begin
                      @(negedge clock); if (!awvalid || !wvalid || awaddr!==saved_address || wdata!==saved_payload) $fatal(1, "GENERATED_WRITE_PAYLOAD_UNSTABLE");
                    end
                    expected[memory_index(WRITE_BASE+16*4)]=32'h12000010;
                    expected_valid[memory_index(WRITE_BASE+16*4)]=1;
                    @(negedge clock); write_opcode=OP_STORE; write_size=32; write_address=WRITE_BASE+16*4;
                    write_data=32'h12000010; write_start=1;
                    repeat (4) @(negedge clock);
                    if (write_done || !write_start || !awvalid || !wvalid)
                        $fatal(1, "GENERATED_FULL_FIFO_PENDING_STORE_NOT_BLOCKED");
                    before_r=read_beats; stall_ar=0; stall_r=0; load_word(READ_BASE);
                    if (!last_load_immediate || read_beats != before_r || write_done || !write_start)
                        $fatal(1, "GENERATED_LOAD_DID_NOT_PROGRESS_BEFORE_PENDING_STORE");
                    stall_aw=0; stall_w=0; wait_write_done();
                    @(negedge clock); write_start=0; drain();
                end
                "WRITE_STALL_READ_PROGRESS":
                begin
                    expected_write_beats=4; expected_b_responses=1;
                    stall_ar=1; stall_r=1; stall_aw=1; stall_w=1;
                    configure(0,WRITE_BASE,4); configure(1,READ_BASE,4);
                    for (t=0;t<4;t=t+1) store_word(WRITE_BASE+t*4,32'h13000000+t);
                    before_r=read_beats; stall_ar=0; stall_r=0; load_word(READ_BASE);
                    if (read_beats <= before_r || !awvalid || !wvalid) $fatal(1, "GENERATED_READ_PROGRESS_WHILE_WRITE_STALL_FAILED");
                    stall_aw=0; stall_w=0; drain();
                end
                "READ_STALL_WRITE_PROGRESS":
                begin
                    expected_write_beats=4; expected_b_responses=1;
                    configure(1,READ_BASE,4); configure(0,WRITE_BASE,4); stall_ar=1; stall_r=1;
                    @(negedge clock); read_opcode=OP_LOAD; read_address=READ_BASE; read_start=1;
                    for (t=0;t<4;t=t+1) store_word(WRITE_BASE+t*4,32'h14000000+t);
                    drain();
                    if (read_done || accepted_b == 0) $fatal(1, "GENERATED_WRITE_PROGRESS_WHILE_READ_STALL_FAILED");
                    read_start=0; stall_ar=0; stall_r=0; wait_read_done(); drain();
                end
                "SIMULTANEOUS":
                begin
                    expected_write_beats=1; expected_b_responses=1;
                    stall_r=1; stall_aw=1; stall_w=1; stall_b=1;
                    before_ar=accepted_ar; before_aw=accepted_aw; before_r=read_beats;
                    before_rw=simultaneous_rw;
                    before_done=simultaneous_done;
                    configure(1,READ_BASE,2); configure(0,WRITE_BASE,1);
                    t=0; while (accepted_ar==before_ar && t<2000)
                    begin
                      @(negedge clock); t=t+1;
                    end
                    if (accepted_ar==before_ar || ar_queue_addr[ar_wr-1] !== READ_BASE || ar_rd==ar_wr)
                        $fatal(1, "GENERATED_SIMULTANEOUS_AR_ADDRESS_INVALID");
                    expected[memory_index(WRITE_BASE)]=32'h15000000; expected_valid[memory_index(WRITE_BASE)]=1;
                    @(negedge clock); write_opcode=OP_STORE; write_size=32; write_address=WRITE_BASE;
                    write_data=32'h15000000; write_start=1;
                    #1;
                    t=0; while (!write_done && t<2000)
                    begin
                      @(negedge clock); #1; t=t+1;
                    end
                    if (!write_done) $fatal(1, "GENERATED_SIMULTANEOUS_FIRST_STORE_ACK_MISSING");
                    @(posedge clock); @(negedge clock); write_start=0;
                    t=0; while ((!awvalid || !wvalid) && t<2000)
                    begin
                      @(negedge clock); t=t+1;
                    end
                    if (!awvalid || !wvalid || awaddr !== WRITE_BASE)
                        $fatal(1, "GENERATED_SIMULTANEOUS_WRITE_NOT_PENDING");
                    stall_r=0; stall_aw=0; stall_w=0;
                    @(posedge clock); #1;
                    if (read_beats!=before_r+1 || accepted_w!=1 || simultaneous_rw==before_rw)
                        $fatal(1, "GENERATED_SIMULTANEOUS_FIRST_HANDSHAKES_NOT_COINCIDENT");
                    @(negedge clock); stall_r=1;
                    load_word(READ_BASE);
                    if (last_load_value!==memory[memory_index(READ_BASE)])
                        $fatal(1, "GENERATED_SIMULTANEOUS_FIRST_LOAD_MISMATCH");
                    @(negedge clock); read_opcode=OP_LOAD; read_size=32; read_address=READ_BASE+4; read_start=1;
                    @(posedge clock); #1;
                    if (read_done || read_beats!=before_r+1 || accepted_load_ops!=2 || ar_rd==ar_wr)
                        $fatal(1, "GENERATED_SIMULTANEOUS_SECOND_LOAD_NOT_PENDING");
                    @(negedge clock); read_start=0;
                    t=0; while (simultaneous_rw==before_rw && t<2000)
                    begin
                      @(negedge clock); t=t+1;
                    end
                    if (simultaneous_rw==before_rw) $fatal(1, "GENERATED_SIMULTANEOUS_HANDSHAKES_NOT_COINCIDENT");
                    @(negedge clock); write_opcode=OP_DRAIN; write_start=1;
                    @(posedge clock); @(negedge clock); write_start=0;
                    t=0; while (b_pending==0 && t<2000)
                    begin
                      @(negedge clock); t=t+1;
                    end
                    if (b_pending==0) $fatal(1, "GENERATED_SIMULTANEOUS_B_NOT_PENDING");
                    if (ar_rd==ar_wr || !stall_r || !stall_b)
                        $fatal(1, "GENERATED_SIMULTANEOUS_R_OR_B_NOT_PENDING");
                    stall_b=0;
                    stall_r=0;
                    @(posedge clock); #1;
                    if (!read_done || !write_done || read_result!==memory[memory_index(READ_BASE+4)])
                        $fatal(1, "GENERATED_SIMULTANEOUS_DONE_NOT_COINCIDENT");
                    t=0; while (!write_done && t<2000)
                    begin
                      @(negedge clock); t=t+1;
                    end
                    if (!write_done || accepted_b!=1)
                        $fatal(1, "GENERATED_SIMULTANEOUS_DRAIN_COMPLETION_INVALID");
                    @(posedge clock); #1;
                    if (simultaneous_done==before_done)
                        $fatal(1, "GENERATED_SIMULTANEOUS_DONE_NOT_SAMPLED");
                    @(negedge clock);
                    if (accepted_ar!=before_ar+1 || accepted_aw!=before_aw+1 || accepted_w!=1 || committed_w!=1 || read_beats!=before_r+2)
                        $fatal(1, "GENERATED_SIMULTANEOUS_COUNTS_INVALID");
                end
                "CONFIG_ISOLATION":
                begin
                    expected_write_beats=2; expected_b_responses=1;
                    before_ar=accepted_ar; before_r=read_beats;
                    configure(1,READ_BASE,3); configure(0,WRITE_BASE,2);
                    load_word(READ_BASE); store_word(WRITE_BASE,32'h16000000);
                    load_word(READ_BASE+4); store_word(WRITE_BASE+4,32'h16000001);
                    load_word(READ_BASE+8); drain();
                    if (accepted_ar!=before_ar+(3+READ_B_MAX-1)/READ_B_MAX || read_beats!=before_r+3)
                        $fatal(1, "GENERATED_CONFIG_READ_COUNT_CROSSTALK ar=%0d r=%0d expected_ar=%0d expected_r=3",
                               accepted_ar-before_ar, read_beats-before_r, (3+READ_B_MAX-1)/READ_B_MAX);
                    if (memory[memory_index(WRITE_BASE)] !== 32'h16000000 ||
                        memory[memory_index(WRITE_BASE+4)] !== 32'h16000001)
                        $fatal(1, "GENERATED_CONFIG_ISOLATION_FAILED");
                end
                "ZERO_READ":
                begin
                    expected_write_beats=2; expected_b_responses=1;
                    configure(1,READ_BASE,0); configure(0,WRITE_BASE,2);
                    for (t=0;t<2;t=t+1) store_word(WRITE_BASE+t*4,32'h17000000+t);
                    drain(); if (accepted_ar != 0 || committed_w != 2) $fatal(1, "GENERATED_ZERO_READ_TRAFFIC");
                end
                "ZERO_WRITE":
                begin
                    configure(1,READ_BASE,2); configure(0,WRITE_BASE,0);
                    load_word(READ_BASE); load_word(READ_BASE+4); drain();
                    if (accepted_aw != 0 || accepted_w != 0 || accepted_b != 0) $fatal(1, "GENERATED_ZERO_WRITE_TRAFFIC");
                end
                "UNPAIRED_DESCRIPTORS":
                begin
                    expected_write_beats=16; expected_b_responses=1;
                    before_ar=accepted_ar; before_aw=accepted_aw; before_r=read_beats;
                    configure(1,READ_BASE,40); configure(0,WRITE_BASE,16);
                    fork
                        begin
                            for (integer q=0;q<40;q=q+1) load_word(READ_BASE+q*4);
                        end
                        begin
                            for (integer q=0;q<16;q=q+1) store_word(WRITE_BASE+q*4,32'h18000000+q);
                            drain();
                        end
                    join
                    if (accepted_ar-before_ar != (40+READ_B_MAX-1)/READ_B_MAX ||
                        accepted_aw-before_aw != 1 || read_beats-before_r != 40 ||
                        accepted_w != 16 || committed_w != 16)
                        $fatal(1, "GENERATED_UNPAIRED_DESCRIPTOR_COUNTS_INVALID ar=%0d aw=%0d r=%0d w=%0d committed=%0d",
                               accepted_ar-before_ar, accepted_aw-before_aw, read_beats-before_r, accepted_w, committed_w);
                end
                "INPLACE_POINTWISE":
                begin
                    expected_write_beats=8; expected_b_responses=1;
                    configure(1,READ_BASE,8); configure(0,READ_BASE,8);
                    for (t=0;t<8;t=t+1) initial_words[t]=memory[64+t];
                    for (t=0;t<8;t=t+1)
                    begin
                        load_word(READ_BASE+t*4);
                        if (last_load_value !== initial_words[t]) $fatal(1, "GENERATED_INPLACE_READ_BEFORE_WRITE");
                        expected[64+t]=last_load_value ^ (32'h19000000+t); expected_valid[64+t]=1;
                        store_word(READ_BASE+t*4, expected[64+t]);
                    end
                    drain(); configure(1,READ_BASE,8);
                    for (t=0;t<8;t=t+1)
                    begin
                      load_word(READ_BASE+t*4); if (last_load_value!==expected[64+t]) $fatal(1, "GENERATED_INPLACE_READ_AFTER_WRITE");
                    end
                end
                "BRESP_PEER_DRAIN":
                begin
                    expected_write_beats=4; expected_b_responses=1;
                    stall_ar=1; stall_r=1; stall_b=1;
                    before_ar=accepted_ar;
                    before_done=accepted_load_ops;
                    configure(1,READ_BASE,8); configure(0,WRITE_BASE,4);
                    @(negedge clock); read_opcode=OP_LOAD; read_address=READ_BASE; read_start=1;
                    @(posedge clock); #1;
                    if (accepted_load_ops!=before_done+1 || read_done || read_beats!=0 || !stall_r)
                        $fatal(1, "GENERATED_BRESP_LOAD_NOT_PENDING_ON_STALLED_R");
                    @(negedge clock); read_start=0;
                    if (accepted_load_ops!=before_done+1)
                        $fatal(1, "GENERATED_BRESP_LOAD_NOT_ACCEPTED");
                    stall_ar=0;
                    t=0; while ((accepted_ar==before_ar || ar_rd==ar_wr) && t<2000)
                    begin
                      @(negedge clock); t=t+1;
                    end
                    if (accepted_ar==before_ar || ar_rd==ar_wr || !stall_r || read_beats!=0)
                        $fatal(1, "GENERATED_BRESP_R_NOT_PENDING_BEFORE_FAULT");
                    for (t=0;t<4;t=t+1) store_word(WRITE_BASE+t*4,32'h1a000000+t);
                    @(negedge clock); write_opcode=OP_DRAIN; write_start=1;
                    @(posedge clock); @(negedge clock); write_start=0;
                    t=0; while (b_pending==0 && t<2000)
                    begin
                      @(negedge clock); t=t+1;
                    end
                    if (b_pending==0) $fatal(1, "GENERATED_BRESP_NOT_PENDING");
                    bresp=2'b10; stall_b=0;
                    t=0; while (accepted_b==0 && t<2000)
                    begin
                      @(negedge clock); t=t+1;
                    end
                    if (accepted_b==0 || fault_b_seen==0 || write_done || read_done)
                        $fatal(1, "GENERATED_BRESP_FALSE_GLOBAL_SUCCESS");
                    stall_r=0;
                    t=0; while (ar_rd!=ar_wr && t<2000)
                    begin
                      @(negedge clock); t=t+1;
                    end
                    repeat (2) @(negedge clock);
                    if (ar_rd!=ar_wr || read_beats==0 || b_pending!=0 || b_fault_read_success)
                        $fatal(1, "GENERATED_BRESP_PEER_DRAIN_FAILED false_read_done=%0d", b_fault_read_success);
                end
                "RRESP_PEER_DRAIN":
                begin
                    expected_write_beats=8; expected_b_responses=1;
                    stall_r=1; stall_aw=1; stall_w=1;
                    configure(1,READ_BASE,8); configure(0,WRITE_BASE,8);
                    for (t=0;t<8;t=t+1) store_word(WRITE_BASE+t*4,32'h1b000000+t);
                    @(negedge clock); write_opcode=OP_DRAIN; write_start=1;
                    @(posedge clock); @(negedge clock); write_start=0;
                    read_opcode=OP_LOAD; read_address=READ_BASE; read_start=1;
                    rresp=2'b10; stall_r=0;
                    t=0; while (read_beats==0 && t<2000)
                    begin
                      @(negedge clock); t=t+1;
                    end
                    if (read_beats==0 || fault_r_seen==0 || read_done || write_done)
                        $fatal(1, "GENERATED_RRESP_FALSE_GLOBAL_SUCCESS");
                    read_start=0; stall_aw=0; stall_w=0;
                    t=0; while (b_pending<1 && t<2000)
                    begin
                      @(negedge clock); t=t+1;
                    end
                    if (b_pending<1) $fatal(1, "GENERATED_RRESP_RESERVED_WRITE_NOT_DRAINING");
                    stall_b=0;
                    t=0; while (accepted_b==0 && t<2000)
                    begin
                      @(negedge clock); t=t+1;
                    end
                    if (accepted_b==0 || write_done) $fatal(1, "GENERATED_RRESP_WRITE_FALSE_SUCCESS");
                end
                "FAULT_RESET_RESTART":
                begin
                    configure(1,READ_BASE,4); configure(0,WRITE_BASE,4); rresp=2'b10;
                    @(negedge clock); read_opcode=OP_LOAD; read_address=READ_BASE; read_start=1;
                    t=0; while (read_beats==0 && t<2000)
                    begin
                      @(negedge clock); t=t+1;
                    end
                    if (read_beats==0 || fault_r_seen==0 || read_done) $fatal(1, "GENERATED_FAULT_NOT_OBSERVED");
                    read_start=0; reset=0; cache_reset=1; tick(3); reset=1; cache_reset=0; rresp=0;
                    configure(1,READ_BASE,2); load_word(READ_BASE); load_word(READ_BASE+4);
                end
                default: $fatal(1, "GENERATED_DUPLEX_UNKNOWN_CASE=%s", name);
            endcase
            if (accepted_w != expected_write_beats || committed_w != expected_write_beats ||
                accepted_b != expected_b_responses)
                $fatal(1, "GENERATED_WRITE_COUNT_MISMATCH accepted_w=%0d committed=%0d b=%0d expected_w=%0d expected_b=%0d",
                       accepted_w, committed_w, accepted_b, expected_write_beats, expected_b_responses);
        end
    endtask

    initial
    begin : test
        string selected_case;
        string selected_profile;
        if (!$value$plusargs("CASE=%s", selected_case)) $fatal(1, "GENERATED_DUPLEX_CASE_REQUIRED");
        if (!$value$plusargs("PROFILE=%s", selected_profile)) $fatal(1, "GENERATED_DUPLEX_PROFILE_REQUIRED");
        if (ADDRESS_BITS != 32 && ADDRESS_BITS != 64) $fatal(1, "GENERATED_DUPLEX_ADDRESS_BITS_INVALID");
        if ((ADDRESS_BITS == 32 && selected_profile != "same_base_pointwise") ||
            (ADDRESS_BITS == 64 && selected_profile != "same_base_asymmetric_64"))
            $fatal(1, "GENERATED_DUPLEX_PROFILE_NAME_MISMATCH");
        if ((ADDRESS_BITS == 32 && (READ_B_MAX != 16 || READ_OUTSTANDING != 1 || READ_FIFO_DEPTH != 32 || WRITE_B_MAX != 16 || WRITE_OUTSTANDING != 1 || WRITE_FIFO_DEPTH != 16)) ||
            (ADDRESS_BITS == 64 && (READ_B_MAX != 8 || READ_OUTSTANDING != 2 || READ_FIFO_DEPTH != 32 || WRITE_B_MAX != 16 || WRITE_OUTSTANDING != 1 || WRITE_FIFO_DEPTH != 16)))
            $fatal(1, "GENERATED_DUPLEX_PROFILE_PARAMETERS_INVALID");
        if (aggregator.read_engine.B_MAX != READ_B_MAX ||
            aggregator.read_engine.MAX_OUTSTANDING != READ_OUTSTANDING ||
            aggregator.read_engine.FIFO_DEPTH != READ_FIFO_DEPTH ||
            aggregator.read_engine.BITSIZE_m_axi_araddr != ADDRESS_BITS ||
            aggregator.write_engine.B_MAX != WRITE_B_MAX ||
            aggregator.write_engine.MAX_OUTSTANDING != WRITE_OUTSTANDING ||
            aggregator.write_engine.FIFO_DEPTH != WRITE_FIFO_DEPTH ||
            aggregator.write_engine.BITSIZE_m_axi_awaddr != ADDRESS_BITS)
            $fatal(1, "GENERATED_PROFILE_MISMATCH");
        for (i=0;i<WORDS;i=i+1)
        begin
          memory[i]=32'h5a5a0000 ^ i; expected[i]=memory[i]; expected_valid[i]=0;
        end
        repeat (3) @(negedge clock);
        reset=1; cache_reset=0;
        run_case(selected_case);
        verify_memory();
        $display("GENERATED_DUPLEX PASS CASE=%s PROFILE=%s", selected_case, selected_profile);
        $finish;
    end
endmodule
