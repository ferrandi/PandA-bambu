module tb_duplex_cores #(
    parameter integer READ_B_MAX = 16,
    parameter integer READ_OUTSTANDING = 3,
    parameter integer READ_FIFO_DEPTH = 64,
    parameter integer WRITE_B_MAX = 8,
    parameter integer WRITE_OUTSTANDING = 2,
    parameter integer WRITE_FIFO_DEPTH = 32,
    parameter integer PEER_FAULT_TEST = 0
);
    logic clock = 1'b0;
    always #5 clock = ~clock;

    localparam logic [1:0] OP_LOAD = 2'd0;
    localparam logic [1:0] OP_STORE = 2'd1;
    localparam logic [1:0] OP_CONFIGURE = 2'd2;
    localparam logic [1:0] OP_DRAIN = 2'd3;
    localparam integer MEM_WORDS = 4096;
    localparam integer READ_BASE = 256;
    localparam integer WRITE_BASE = 2048;

    logic reset = 1'b0;
    logic read_start = 1'b0, write_start = 1'b0;
    logic [1:0] read_op = OP_CONFIGURE, write_op = OP_CONFIGURE;
    logic [31:0] read_size = 32'd32, read_payload = 0, read_base = READ_BASE*4, read_count = 0;
    logic [31:0] write_size = 32'd32, write_payload = 0, write_base = WRITE_BASE*4, write_count = 0;
    wire read_done, write_done, read_fault, write_fault;
    wire [31:0] read_data;

    wire [31:0] araddr;
    wire [7:0] arlen;
    wire [2:0] arsize;
    wire [1:0] arburst;
    wire arid, arvalid, rready;
    wire arready;
    wire [31:0] rdata;
    wire [1:0] rresp;
    wire rid, rlast, rvalid;

    wire [31:0] awaddr;
    wire [7:0] awlen;
    wire [2:0] awsize;
    wire [1:0] awburst;
    wire awid, awvalid, wvalid, wlast, bready;
    wire [31:0] wdata;
    wire [3:0] wstrb;
    logic awready, wready;
    logic inject_bresp_fault = 0;
    logic inject_rresp_fault = 0;
    logic inject_peer_fault_reader = 0;
    logic inject_peer_fault_writer = 0;
    wire [1:0] bresp = inject_bresp_fault ? 2'b10 : 2'b00;
    logic bid = 0, bvalid = 0;

    logic stall_ar = 1'b0, stall_r = 1'b0;
    logic stall_aw = 1'b0, stall_w = 1'b0, stall_b = 1'b0;

    MinimalAXI4MasterPipelined #(
        .B_MAX(READ_B_MAX), .MAX_OUTSTANDING(READ_OUTSTANDING), .FIFO_DEPTH(READ_FIFO_DEPTH),
        .PEER_FAULT_ENABLE(PEER_FAULT_TEST)
    ) reader (
        .clock(clock), .reset(reset), .start(read_start), .in1(read_op), .in2(read_size),
        .in3(read_payload), .in4(read_base), .in5(read_count), .done(read_done),
        .out1(read_data), .peer_fault(write_fault | inject_peer_fault_reader), .fault(read_fault),
        .m_axi_araddr(araddr), .m_axi_arlen(arlen),
        .m_axi_arsize(arsize), .m_axi_arburst(arburst), .m_axi_arid(arid),
        .m_axi_arvalid(arvalid), .m_axi_arready(arready), .m_axi_rdata(rdata),
        .m_axi_rresp(rresp), .m_axi_rid(rid), .m_axi_rlast(rlast), .m_axi_rvalid(rvalid),
        .m_axi_rready(rready)
    );

    MinimalAXI4MasterPipelinedWrite #(
        .B_MAX(WRITE_B_MAX), .MAX_OUTSTANDING(WRITE_OUTSTANDING), .FIFO_DEPTH(WRITE_FIFO_DEPTH),
        .PEER_FAULT_ENABLE(PEER_FAULT_TEST)
    ) writer (
        .clock(clock), .reset(reset), .start(write_start), .in1(write_op), .in2(write_size),
        .in3(write_payload), .in4(write_base), .in5(write_count), .done(write_done),
        .out1(), .peer_fault(read_fault | inject_peer_fault_writer), .fault(write_fault),
        .m_axi_awaddr(awaddr), .m_axi_awlen(awlen),
        .m_axi_awsize(awsize), .m_axi_awburst(awburst), .m_axi_awid(awid),
        .m_axi_awvalid(awvalid), .m_axi_awready(awready), .m_axi_wdata(wdata),
        .m_axi_wstrb(wstrb), .m_axi_wlast(wlast), .m_axi_wvalid(wvalid),
        .m_axi_wready(wready), .m_axi_bresp(bresp), .m_axi_bid(bid),
        .m_axi_bvalid(bvalid), .m_axi_bready(bready)
    );

    logic [31:0] memory [0:MEM_WORDS-1];
    logic [31:0] expected [0:MEM_WORDS-1];
    logic [31:0] initial_memory [0:MEM_WORDS-1];
    logic [31:0] read_snapshot [0:MEM_WORDS-1];
    logic expected_valid [0:MEM_WORDS-1];
    integer ar_addr_q [0:8191], ar_left_q [0:8191], ar_wr=0, ar_rd=0;
    integer aw_addr_q [0:8191], aw_len_q [0:8191], aw_wr=0, aw_rd=0;
    logic [31:0] w_data_q [0:8191];
    logic w_last_q [0:8191];
    integer w_wr=0, w_rd=0;
    integer accepted_ar=0, accepted_aw=0, accepted_w=0, committed_w=0, accepted_b=0;
    integer read_beats=0;
    integer b_pending=0;
    integer b_delay=0;
    integer coincident_completions=0;
    integer simultaneous_rw_handshakes=0;
    integer r_beat_limit=-1;
    integer last_ar_len=-1, last_aw_len=-1;
    logic [31:0] last_ar_addr=0, last_aw_addr=0;

    assign arready = !stall_ar && ar_wr-ar_rd < 8190;
    assign rvalid = (ar_rd < ar_wr) && !stall_r &&
                    (r_beat_limit < 0 || read_beats < r_beat_limit);
    assign rdata = rvalid ? memory[ar_addr_q[ar_rd] >> 2] : 32'b0;
    assign rid = 1'b0;
    assign rresp = inject_rresp_fault ? 2'b10 : 2'b00;
    assign rlast = rvalid && ar_left_q[ar_rd] == 1;

    always_comb begin
        awready = !stall_aw;
        wready = !stall_w;
    end

    always @(posedge clock) begin : shared_memory
        integer idx;
        integer beat_count;
        if (!reset) begin
            ar_wr<=0; ar_rd<=0; aw_wr=0; aw_rd=0; w_wr=0; w_rd=0;
            accepted_ar=0; accepted_aw=0; accepted_w=0; committed_w=0; accepted_b=0;
            read_beats=0; bvalid<=0; b_pending=0; b_delay=0; coincident_completions=0;
            simultaneous_rw_handshakes=0;
            last_ar_len=-1; last_aw_len=-1;
            last_ar_addr=0; last_aw_addr=0;
        end else begin
            if (read_done && bvalid && bready)
                coincident_completions=coincident_completions+1;
            if (rvalid && rready && wvalid && wready)
                simultaneous_rw_handshakes=simultaneous_rw_handshakes+1;
            if (arvalid && arready) begin
                if (arsize !== 3'd2) $fatal(1, "AR_SIZE_NOT_FULL_WORD value=%b", arsize);
                if (arburst !== 2'b01) $fatal(1, "AR_BURST_NOT_INCR value=%b", arburst);
                if (arid !== 1'b0) $fatal(1, "AR_ID_NOT_ZERO value=%b", arid);
                ar_addr_q[ar_wr]<=araddr;
                ar_left_q[ar_wr]<=arlen+1;
                ar_wr<=ar_wr+1;
                accepted_ar=accepted_ar+1;
                last_ar_len=arlen;
                last_ar_addr=araddr;
            end
            if (rvalid && rready) begin
                if (ar_rd >= ar_wr) $fatal(1, "READ_WITHOUT_ACCEPTED_AR");
                read_beats=read_beats+1;
                if (ar_left_q[ar_rd] == 1) ar_rd<=ar_rd+1;
                else begin
                    ar_addr_q[ar_rd]<=ar_addr_q[ar_rd]+4;
                    ar_left_q[ar_rd]<=ar_left_q[ar_rd]-1;
                end
            end
            if (awvalid && awready) begin
                if (awsize !== 3'd2) $fatal(1, "AW_SIZE_NOT_FULL_WORD value=%b", awsize);
                if (awburst !== 2'b01) $fatal(1, "AW_BURST_NOT_INCR value=%b", awburst);
                if (awid !== 1'b0) $fatal(1, "AW_ID_NOT_ZERO value=%b", awid);
                aw_addr_q[aw_wr]=awaddr;
                aw_len_q[aw_wr]=awlen+1;
                aw_wr=aw_wr+1;
                accepted_aw=accepted_aw+1;
                last_aw_len=awlen;
                last_aw_addr=awaddr;
            end
            if (wvalid && wready) begin
                if (wstrb !== 4'b1111) $fatal(1, "SHARED_WSTRB_NOT_FULL value=%b", wstrb);
                w_data_q[w_wr]=wdata;
                w_last_q[w_wr]=wlast;
                w_wr=w_wr+1;
                accepted_w=accepted_w+1;
            end
            if (aw_rd < aw_wr && w_wr-w_rd >= aw_len_q[aw_rd]) begin
                beat_count=aw_len_q[aw_rd];
                for (idx=0; idx<beat_count; idx=idx+1) begin
                    if (w_last_q[w_rd+idx] !== (idx == beat_count-1))
                        $fatal(1, "SHARED_WLAST_MISMATCH");
                    if (aw_addr_q[aw_rd]/4+idx < 0 || aw_addr_q[aw_rd]/4+idx >= MEM_WORDS)
                        $fatal(1, "SHARED_WRITE_ADDRESS_RANGE");
                    if (!expected_valid[aw_addr_q[aw_rd]/4+idx] ||
                        w_data_q[w_rd+idx] !== expected[aw_addr_q[aw_rd]/4+idx])
                        $fatal(1, "SHARED_WRITE_DATA_MISMATCH address=%h", aw_addr_q[aw_rd]+idx*4);
                    memory[aw_addr_q[aw_rd]/4+idx]=w_data_q[w_rd+idx];
                end
                w_rd=w_rd+beat_count;
                aw_rd=aw_rd+1;
                committed_w=committed_w+beat_count;
                b_pending=b_pending+1;
                b_delay=2;
            end
            if (!stall_b && !bvalid && b_pending > 0) begin
                if (b_delay > 0) b_delay=b_delay-1;
                else bvalid<=1;
            end
            if (bvalid && bready) begin
                bvalid<=0;
                b_pending=b_pending-1;
                accepted_b=accepted_b+1;
            end
        end
    end

    task automatic reset_cores;
        begin
            read_start=0; write_start=0;
            reset=0;
            repeat (3) @(posedge clock);
            @(negedge clock); reset=1;
            repeat (2) @(posedge clock);
        end
    endtask

    task automatic configure_read(input [31:0] base, input [31:0] count);
        begin
            @(negedge clock); read_op=OP_CONFIGURE; read_size=32; read_base=base; read_count=count; read_start=1;
            #1;
            if (!read_done || read_fault) $fatal(1, "READ_CONFIGURE_NOT_ACKNOWLEDGED_BEFORE_CAPTURE");
            @(posedge clock); #1;
            if (read_fault) $fatal(1, "READ_CONFIGURE_FAULTED_ON_CAPTURE");
            @(negedge clock); read_start=0;
        end
    endtask

    task automatic configure_write(input [31:0] base, input [31:0] count);
        begin
            @(negedge clock); write_op=OP_CONFIGURE; write_size=32; write_base=base; write_count=count; write_start=1;
            #1;
            if (!write_done || write_fault) $fatal(1, "WRITE_CONFIGURE_NOT_ACKNOWLEDGED_BEFORE_CAPTURE");
            @(posedge clock); #1;
            if (write_fault) $fatal(1, "WRITE_CONFIGURE_FAULTED_ON_CAPTURE");
            @(negedge clock); write_start=0;
        end
    endtask

    task automatic drain_write(input bit expect_completion);
        integer cycles;
        begin
            @(negedge clock); write_op=OP_DRAIN; write_start=1;
            #1;
            if (expect_completion && !write_done)
                $fatal(1, "DRAIN_COMPLETION_NOT_VISIBLE");
            if (!expect_completion && write_done)
                $fatal(1, "DRAIN_COMPLETED_BEFORE_B_RESPONSE");
            @(posedge clock); @(negedge clock); write_start=0;
            if (expect_completion) begin
                cycles=0;
                while (!write_done && !write_fault && cycles<4000) begin
                    @(negedge clock); #1; cycles=cycles+1;
                end
                if (!write_done || write_fault) $fatal(1, "DRAIN_COMPLETION_TIMEOUT");
            end else if (write_done) $fatal(1, "DRAIN_COMPLETED_WITH_B_BLOCKED");
        end
    endtask

    task automatic issue_load(input [31:0] address);
        begin
            @(negedge clock); read_op=OP_LOAD; read_size=32; read_base=address; read_start=1;
            @(posedge clock); @(negedge clock); read_start=0;
        end
    endtask

    task automatic issue_store(input [31:0] address, input [31:0] payload);
        integer cycles;
        begin
            expected[address/4]=payload;
            expected_valid[address/4]=1'b1;
            @(negedge clock); write_op=OP_STORE; write_size=32; write_base=address;
            write_payload=payload; write_start=1;
            cycles=0;
            while (!write_done && cycles<2000) begin @(negedge clock); cycles=cycles+1; end
            if (!write_done) $fatal(1, "STORE_ACK_TIMEOUT address=%h", address);
            @(negedge clock); write_start=0;
        end
    endtask

    task automatic issue_load_and_check(input [31:0] address);
        integer cycles;
        bit completed;
        begin
            @(negedge clock); read_op=OP_LOAD; read_size=32; read_base=address; read_start=1;
            #1;
            completed=read_done;
            if (completed && read_data !== read_snapshot[address/4])
                $fatal(1, "LOAD_DATA_MISMATCH address=%h got=%h expected=%h", address, read_data, read_snapshot[address/4]);
            if (!completed) begin
                @(posedge clock); #1;
                completed=read_done;
                if (completed && read_data !== read_snapshot[address/4])
                    $fatal(1, "LOAD_DATA_MISMATCH address=%h got=%h expected=%h", address, read_data, read_snapshot[address/4]);
            end
            if (!completed) begin
                @(negedge clock); read_start=0;
                cycles=0;
                while (!read_done && cycles<4000) begin @(negedge clock); #1; cycles=cycles+1; end
                if (!read_done) $fatal(1, "LOAD_DONE_TIMEOUT address=%h", address);
                if (read_data !== read_snapshot[address/4])
                    $fatal(1, "LOAD_DATA_MISMATCH address=%h got=%h expected=%h", address, read_data, read_snapshot[address/4]);
                @(posedge clock);
                @(negedge clock);
            end else begin
                @(posedge clock);
                @(negedge clock); read_start=0;
            end
        end
    endtask

    task automatic issue_load_capture(input [31:0] address, output logic [31:0] captured_data);
        integer cycles;
        bit completed;
        begin
            @(negedge clock); read_op=OP_LOAD; read_size=32; read_base=address; read_start=1;
            #1;
            completed=read_done;
            if (completed) captured_data=read_data;
            if (!completed) begin
                @(posedge clock); #1;
                completed=read_done;
                if (completed) captured_data=read_data;
            end
            if (!completed) begin
                @(negedge clock); read_start=0;
                cycles=0;
                while (!read_done && cycles<4000) begin @(negedge clock); #1; cycles=cycles+1; end
                if (!read_done) $fatal(1, "LOAD_DONE_TIMEOUT address=%h", address);
                captured_data=read_data;
                @(posedge clock);
                @(negedge clock);
            end else begin
                @(posedge clock);
                @(negedge clock); read_start=0;
            end
        end
    endtask

    task automatic check_write_counts(input integer expected_beats);
        begin
            if (accepted_w !== expected_beats || committed_w !== expected_beats)
                $fatal(1, "WRITE_BEAT_COUNT_MISMATCH accepted=%0d committed=%0d expected=%0d",
                       accepted_w, committed_w, expected_beats);
        end
    endtask

    task automatic issue_store_region(input [31:0] base, input integer count, input [31:0] seed);
        integer idx;
        begin
            for (idx=0;idx<count;idx=idx+1) begin
                issue_store(base+idx*4, seed+idx);
            end
        end
    endtask

    task automatic wait_read_beats(input integer target, input integer limit);
        integer cycles;
        begin
            cycles=0;
            while (read_beats < target && cycles < limit) begin @(negedge clock); cycles=cycles+1; end
            if (read_beats < target) $fatal(1, "READ_PROGRESS_TIMEOUT target=%0d got=%0d", target, read_beats);
        end
    endtask

    task automatic wait_write_complete(input integer target, input integer limit);
        integer cycles;
        begin
            cycles=0;
            while (accepted_b < target && cycles < limit) begin @(negedge clock); cycles=cycles+1; end
            if (accepted_b < target) $fatal(1, "WRITE_COMPLETION_TIMEOUT target=%0d b=%0d", target, accepted_b);
        end
    endtask

    task automatic verify_region(input integer base_word, input integer count);
        integer idx;
        begin
            // The backend checks address and payload before committing each write here;
            // these final scans are defense in depth, not independent write witnesses.
            for (idx=0; idx<count; idx=idx+1)
                if (memory[base_word+idx] !== expected[base_word+idx])
                    $fatal(1, "MEMORY_MISMATCH word=%0d got=%h expected=%h", base_word+idx,
                           memory[base_word+idx], expected[base_word+idx]);
        end
    endtask

    task automatic verify_full_memory;
        integer idx;
        begin
            // Write address and payload are already checked by the backend before commit;
            // this full-memory scan is defense in depth, not an independent witness.
            for (idx=0; idx<MEM_WORDS; idx=idx+1) begin
                if (expected_valid[idx] && memory[idx] !== expected[idx])
                    $fatal(1, "MEMORY_MISMATCH word=%0d got=%h expected=%h", idx, memory[idx], expected[idx]);
                if (!expected_valid[idx] && memory[idx] !== initial_memory[idx])
                    $fatal(1, "CANARY_CORRUPTED word=%0d got=%h expected=%h", idx, memory[idx], initial_memory[idx]);
            end
        end
    endtask

    task automatic run_selected(input string name);
        integer before_ar, before_aw, before_w, before_r, before_b, timeout_cycles;
        integer expected_write_beats;
        integer expected_ar_count, expected_r_beats, before_scheduled;
        integer before_descriptors, before_issued;
        logic [31:0] saved_araddr;
        logic [31:0] saved_wdata;
        logic [31:0] transformed;
        logic [7:0] saved_arlen;
        logic saved_wlast;
        begin
            reset_cores();
            expected_write_beats=0;
            stall_ar=0; stall_r=0; stall_aw=0; stall_w=0; stall_b=0;
            inject_bresp_fault=0; inject_rresp_fault=0;
            inject_peer_fault_reader=0; inject_peer_fault_writer=0;
            case (name)
                "LOAD_WAIT_STORE": begin
                    stall_r=1;
                    configure_read(READ_BASE*4, 8); configure_write(WRITE_BASE*4, 8);
                    before_b=accepted_b;
                    issue_load(READ_BASE*4); issue_store_region(WRITE_BASE*4,8,32'h80000000);
                    repeat (3) @(negedge clock);
                    if (accepted_ar == 0 || read_done || read_beats != 0)
                        $fatal(1, "LOAD_WAIT_STALL_NOT_REACHED ar=%0d r=%0d done=%b", accepted_ar, read_beats, read_done);
                    if (write_fault || accepted_aw == 0) $fatal(1, "STORE_DID_NOT_PROGRESS_WHILE_READ_WAITED");
                    before_r=read_beats;
                    wait_write_complete(before_b+1, 1000);
                    if (read_beats != before_r || read_done)
                        $fatal(1, "READ_PROGRESS_BEFORE_R_RELEASE");
                    stall_r=0;
                    timeout_cycles=0;
                    while (!read_done && !read_fault && timeout_cycles<1000) begin
                        @(negedge clock); timeout_cycles=timeout_cycles+1;
                    end
                    if (read_fault || read_beats <= before_r || !read_done || read_data !== read_snapshot[READ_BASE])
                        $fatal(1, "LOAD_DID_NOT_RESUME_AFTER_R done=%b fault=%b beats=%0d data=%h expected=%h",
                               read_done, read_fault, read_beats, read_data, read_snapshot[READ_BASE]);
                    drain_write(1); expected_write_beats=8; check_write_counts(expected_write_beats);
                    verify_region(WRITE_BASE,8);
                end
                "LOAD_DURING_WRITE_BLOCK": begin
                    before_r=read_beats;
                    stall_ar=1; stall_r=1; stall_aw=1; stall_w=1; stall_b=1;
                    configure_read(READ_BASE*4, 4); configure_write(WRITE_BASE*4, 40);
                    for (integer k=0;k<32;k=k+1) begin
                        expected[WRITE_BASE+k]=32'h81000000+k;
                        issue_store(WRITE_BASE*4+k*4,32'h81000000+k);
                    end
                    expected[WRITE_BASE+32]=32'h81000020;
                    expected_valid[WRITE_BASE+32]=1'b1;
                    @(negedge clock); write_op=OP_STORE; write_size=32; write_base=(WRITE_BASE+32)*4;
                    write_payload=32'h81000020; write_start=1;
                    repeat (3) @(negedge clock);
                    if (!write_start || write_done || !awvalid || !wvalid)
                        $fatal(1, "WRITE_FIFO_FULL_PENDING_NOT_REACHED");
                    stall_ar=0; stall_r=0;
                    wait_read_beats(before_r+4, 2000);
                    if (!awvalid || !wvalid || awready || wready)
                        $fatal(1, "LOAD_PROGRESS_NOT_INDEPENDENT_OF_BLOCKED_WRITE");
                    for (integer k=0;k<4;k=k+1) issue_load_and_check(READ_BASE*4+k*4);
                    stall_aw=0; stall_w=0;
                    timeout_cycles=0;
                    while (!write_done && timeout_cycles<2000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (!write_done) $fatal(1, "FULL_FIFO_STORE_NOT_ACCEPTED_AFTER_SPACE");
                    @(negedge clock); write_start=0;
                    for (integer k=33;k<40;k=k+1) begin
                        expected[WRITE_BASE+k]=32'h81000000+k;
                        issue_store(WRITE_BASE*4+k*4,32'h81000000+k);
                    end
                    stall_b=0; wait_write_complete(5, 5000); drain_write(1);
                    expected_write_beats=40; check_write_counts(expected_write_beats);
                    verify_region(WRITE_BASE,40);
                end
                "AW_W_B_BLOCK_AR_PROGRESS": begin
                    before_ar=accepted_ar; before_r=read_beats;
                    before_aw=accepted_aw; before_w=accepted_w; before_b=accepted_b;
                    stall_ar=1; stall_r=1; stall_aw=1; stall_w=1; stall_b=1;
                    configure_read(READ_BASE*4, 8); configure_write(WRITE_BASE*4, 8);
                    issue_store_region(WRITE_BASE*4,8,32'h82000000);
                    repeat (3) @(negedge clock);
                    if (!awvalid || !wvalid || awready || wready)
                        $fatal(1, "AW_W_BLOCK_NOT_REACHED");
                    stall_ar=0;
                    timeout_cycles=0;
                    while (accepted_ar==before_ar && timeout_cycles<1000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (accepted_ar==before_ar || accepted_aw!=before_aw || accepted_w!=before_w)
                        $fatal(1, "AR_PROGRESS_NOT_ISOLATED_FROM_BLOCKED_WRITES");
                    stall_r=0;
                    wait_read_beats(before_r+8, 2000);
                    if (accepted_aw!=before_aw || accepted_w!=before_w ||
                        !awvalid || !wvalid || awready || wready)
                        $fatal(1, "R_PROGRESS_DID_NOT_OCCUR_DURING_WRITE_BLOCK");
                    for (integer k=0;k<8;k=k+1) issue_load_and_check(READ_BASE*4+k*4);
                    stall_aw=0; stall_w=0; stall_b=0;
                    wait_write_complete(before_b+1, 2000); drain_write(1);
                    expected_write_beats=8; check_write_counts(expected_write_beats); verify_region(WRITE_BASE,8);
                end
                "AR_R_BLOCK_WRITE_PROGRESS": begin
                    stall_ar=1; stall_r=1;
                    configure_read(READ_BASE*4, 4); configure_write(WRITE_BASE*4, 8);
                    issue_load(READ_BASE*4);
                    repeat (3) @(negedge clock);
                    if (!arvalid || read_done) $fatal(1, "AR_R_BLOCK_NOT_REACHED");
                    issue_store_region(WRITE_BASE*4,8,32'h83000000);
                    wait_write_complete(1, 3000); drain_write(1);
                    expected_write_beats=8; check_write_counts(expected_write_beats);
                    verify_region(WRITE_BASE,8);
                    if (read_done) $fatal(1, "LOAD_COMPLETED_WITHOUT_READ_DATA");
                end
                "SIMULTANEOUS": begin
                    before_ar=accepted_ar; before_aw=accepted_aw;
                    configure_read(READ_BASE*4, 4); configure_write(WRITE_BASE*4, 4);
                    before_r=simultaneous_rw_handshakes;
                    stall_r=1; stall_w=1; stall_b=1; r_beat_limit=3;
                    issue_store_region(WRITE_BASE*4,4,32'h84000000);
                    timeout_cycles=0;
                    while (!wvalid && timeout_cycles<1000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (!wvalid) $fatal(1, "SIMULTANEOUS_WRITE_BEAT_NOT_STALLED");
                    stall_r=0;
                    timeout_cycles=0;
                    while (!(rvalid && wvalid) && timeout_cycles<1000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (!(rvalid && wvalid)) $fatal(1, "SIMULTANEOUS_RW_CHANNELS_NOT_READY");
                    stall_w=0;
                    timeout_cycles=0;
                    while (simultaneous_rw_handshakes == before_r && timeout_cycles<1000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (simultaneous_rw_handshakes == before_r)
                        $fatal(1, "SIMULTANEOUS_RW_HANDSHAKE_NOT_OBSERVED");
                    for (integer k=0;k<3;k=k+1) issue_load_and_check(READ_BASE*4+k*4);
                    if (read_beats != 3) $fatal(1, "SIMULTANEOUS_R_BEAT_LIMIT_MISMATCH beats=%0d", read_beats);
                    issue_load(READ_BASE*4+12);
                    if (read_done) $fatal(1, "FINAL_LOAD_COMPLETED_BEFORE_FINAL_R");
                    b_delay=0;
                    r_beat_limit=4;
                    stall_b=0;
                    @(posedge clock); #1;
                    if (!read_done || read_data !== read_snapshot[READ_BASE+3])
                        $fatal(1, "SIMULTANEOUS_READ_NOT_READY");
                    timeout_cycles=0;
                    while (coincident_completions == 0 && timeout_cycles<2000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (coincident_completions == 0 || accepted_b<1)
                        $fatal(1, "SIMULTANEOUS_COMPLETION_NOT_COINCIDENT");
                    @(negedge clock); read_start=0;
                    drain_write(1); expected_write_beats=4; check_write_counts(expected_write_beats);
                    verify_region(WRITE_BASE,4);
                    if (accepted_ar<=before_ar || accepted_aw<=before_aw) $fatal(1, "SIMULTANEOUS_ENDPOINTS_NOT_ACTIVE");
                end
                "ZERO_READ_NONZERO_WRITE": begin
                    configure_read(READ_BASE*4,0); configure_write(WRITE_BASE*4,4);
                    issue_store_region(WRITE_BASE*4,4,32'h85000000); wait_write_complete(1,2000); drain_write(1);
                    expected_write_beats=4; check_write_counts(expected_write_beats); verify_region(WRITE_BASE,4);
                    if (accepted_ar != 0 || read_fault) $fatal(1, "ZERO_READ_ISSUED_TRAFFIC");
                end
                "ZERO_WRITE_NONZERO_READ": begin
                    configure_read(READ_BASE*4,4); configure_write(WRITE_BASE*4,0);
                    issue_load_and_check(READ_BASE*4);
                    wait_read_beats(4,2000);
                    repeat (3) @(negedge clock);
                    if (accepted_aw != 0 || accepted_w != 0 || accepted_b != 0 ||
                        awvalid || wvalid || write_fault)
                        $fatal(1, "ZERO_WRITE_ISSUED_TRAFFIC aw=%0d w=%0d b=%0d awvalid=%b wvalid=%b fault=%b",
                               accepted_aw, accepted_w, accepted_b, awvalid, wvalid, write_fault);
                    drain_write(1); check_write_counts(0);
                end
                "INPLACE_POINTWISE": begin
                    logic [31:0] expected_output;
                    configure_read(READ_BASE*4,16); configure_write(READ_BASE*4,16);
                    for (integer k=0;k<16;k=k+1) begin
                        issue_load_capture(READ_BASE*4+k*4, transformed);
                        if (transformed !== read_snapshot[READ_BASE+k])
                            $fatal(1, "LOAD_DATA_MISMATCH address=%h got=%h expected=%h",
                                   READ_BASE*4+k*4, transformed, read_snapshot[READ_BASE+k]);
                        expected_output=initial_memory[READ_BASE+k] ^ (32'h13579bdf + k*32'h01010101);
                        transformed=transformed ^ (32'h13579bdf + k*32'h01010101);
                        issue_store(READ_BASE*4+k*4, transformed);
                        expected[READ_BASE+k]=expected_output;
                    end
                    wait_write_complete((16+WRITE_B_MAX-1)/WRITE_B_MAX,4000); drain_write(1);
                    expected_write_beats=16; check_write_counts(expected_write_beats);
                    before_r=read_beats;
                    configure_read(READ_BASE*4,16);
                    for (integer k=0;k<16;k=k+1) begin
                        issue_load_capture(READ_BASE*4+k*4, transformed);
                        expected_output=initial_memory[READ_BASE+k] ^ (32'h13579bdf + k*32'h01010101);
                        if (transformed !== expected_output)
                            $fatal(1, "INPLACE_READ_AFTER_WRITE_MISMATCH index=%0d got=%h expected=%h",
                                   k, transformed, expected_output);
                    end
                    if (read_beats != before_r+16)
                        $fatal(1, "INPLACE_R_BEAT_DELTA_MISMATCH before=%0d after=%0d expected_delta=16",
                               before_r, read_beats);
                end
                "RRESP_FAULT_LOCAL": begin
                    configure_read(READ_BASE*4,4); configure_write(WRITE_BASE*4,4);
                    stall_r=1; stall_b=1; inject_rresp_fault=1;
                    issue_load(READ_BASE*4);
                    issue_store_region(WRITE_BASE*4,4,32'h89000000);
                    drain_write(0);
                    stall_r=0;
                    timeout_cycles=0;
                    while (!read_fault && timeout_cycles<2000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (!read_fault || read_done || write_fault || write_done)
                        $fatal(1, "RRESP_FAULT_NOT_LOCAL_OR_BARRIER_BROKEN read=%b write=%b done=%b b=%0d",
                               read_fault, write_fault, write_done, accepted_b);
                    wait_read_beats(4,2000);
                    if (!read_fault || read_done || write_fault || write_done)
                        $fatal(1, "RRESP_FAULT_DRAIN_OR_NO_SUCCESS_VIOLATION read=%b rdone=%b write=%b wdone=%b b=%0d",
                               read_fault, read_done, write_fault, write_done, accepted_b);
                    stall_b=0;
                    timeout_cycles=0;
                    while (!write_done && !write_fault && timeout_cycles<3000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (!write_done || write_fault || !read_fault)
                        $fatal(1, "WRITE_DRAIN_DID_NOT_SURVIVE_READ_FAULT read=%b write=%b done=%b",
                               read_fault, write_fault, write_done);
                    expected_write_beats=4; check_write_counts(expected_write_beats);
                    verify_region(WRITE_BASE,4);
                end
                "BRESP_FAULT_LOCAL": begin
                    stall_ar=1;
                    configure_read(READ_BASE*4,4); configure_write(WRITE_BASE*4,4);
                    stall_b=1; inject_bresp_fault=1;
                    timeout_cycles=0;
                    while (!arvalid && timeout_cycles<1000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (!arvalid || arready || read_fault || read_done || araddr !== READ_BASE*4 || arlen != 3)
                        $fatal(1, "BRESP_CASE_READ_AR_NOT_STALLED arvalid=%b arready=%b fault=%b done=%b addr=%h len=%0d",
                               arvalid, arready, read_fault, read_done, araddr, arlen);
                    issue_store_region(WRITE_BASE*4,4,32'h8a000000);
                    repeat (3) @(negedge clock);
                    drain_write(0);
                    stall_b=0;
                    timeout_cycles=0;
                    while (!write_fault && timeout_cycles<2000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (!write_fault || read_fault || write_done)
                        $fatal(1, "BRESP_FAULT_NOT_LOCAL_OR_DRAIN_COMPLETED read=%b write=%b done=%b",
                               read_fault, write_fault, write_done);
                    if (!arvalid || arready || read_done || araddr !== READ_BASE*4 || arlen != 3)
                        $fatal(1, "BRESP_FAULT_BLOCKED_OR_COMPLETED_READ_AR");
                    stall_ar=0;
                    wait_read_beats(4,2000);
                    for (integer k=0;k<4;k=k+1)
                        issue_load_and_check(READ_BASE*4+k*4);
                    timeout_cycles=0;
                    if (read_fault || !write_fault || write_done)
                        $fatal(1, "READ_DID_NOT_COMPLETE_DURING_WRITE_FAULT read=%b write=%b wdone=%b",
                               read_fault, write_fault, write_done);
                    expected_write_beats=4; check_write_counts(expected_write_beats);
                    verify_region(WRITE_BASE,4);
                end
                "PEER_BRESP_DRAIN": begin
                    if (PEER_FAULT_TEST != 1) $fatal(1, "PEER_CASE_REQUIRES_PEER_FAULT_PROFILE");
                    stall_r=1;
                    configure_read(READ_BASE*4,40);
                    configure_write(WRITE_BASE*4,1);
                    timeout_cycles=0;
                    while (accepted_ar<2 && timeout_cycles<1000) begin
                        @(negedge clock); timeout_cycles=timeout_cycles+1;
                    end
                    if (accepted_ar != 2 || reader.descriptor_count != 2)
                        $fatal(1, "PEER_BRESP_NEEDS_TWO_ACCEPTED_READ_DESCRIPTORS ar=%0d desc=%0d",
                               accepted_ar, reader.descriptor_count);
                    stall_ar=1;
                    timeout_cycles=0;
                    while (!arvalid && timeout_cycles<1000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (!arvalid || arready || read_done)
                        $fatal(1, "PEER_BRESP_NEEDS_EXPOSED_STALLED_AR");
                    expected_ar_count=accepted_ar+1;
                    expected_r_beats=integer'(arlen)+1;
                    for (integer q=ar_rd;q<ar_wr;q=q+1)
                        expected_r_beats=expected_r_beats+ar_left_q[q];
                    before_b=accepted_b;
                    stall_b=1;
                    issue_store(WRITE_BASE*4,32'h8b000000);
                    drain_write(0);
                    inject_bresp_fault=1;
                    stall_b=0;
                    timeout_cycles=0;
                    while (!write_fault && timeout_cycles<1000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (!write_fault || accepted_b!=before_b+1 || b_pending!=0)
                        $fatal(1, "PEER_BRESP_NOT_ACCEPTED_ON_FAULT_HANDSHAKE b=%0d pending=%0d",
                               accepted_b-before_b, b_pending);
                    timeout_cycles=0;
                    while (!read_fault && timeout_cycles<1000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (!read_fault || read_done || read_beats!=0)
                        $fatal(1, "PEER_BRESP_DID_NOT_ABORT_NEW_READ_SUCCESS");
                    stall_ar=0; stall_r=0;
                    timeout_cycles=0;
                    while ((accepted_ar<expected_ar_count || read_beats<expected_r_beats) && timeout_cycles<3000) begin
                        @(negedge clock); timeout_cycles=timeout_cycles+1;
                    end
                    if (timeout_cycles>=3000 || accepted_ar!=expected_ar_count || read_beats!=expected_r_beats ||
                        reader.descriptor_count!=0)
                        $fatal(1, "PEER_BRESP_ACCEPTED_READ_TRANSPORT_NOT_DRAINED ar=%0d/%0d r=%0d/%0d desc=%0d",
                               accepted_ar, expected_ar_count, read_beats, expected_r_beats, reader.descriptor_count);
                    before_ar=accepted_ar;
                    @(negedge clock); read_op=OP_LOAD; read_size=32; read_base=READ_BASE*4; read_start=1; #1;
                    if (read_done || !read_fault) $fatal(1, "PEER_BRESP_ALLOWED_LOAD_AFTER_FAULT");
                    @(posedge clock); @(negedge clock); read_start=0;
                    repeat (3) @(negedge clock);
                    if (accepted_ar!=before_ar || read_done)
                        $fatal(1, "PEER_BRESP_CREATED_POST_FAULT_READ_RESERVATION");
                end
                "PEER_RRESP_RESERVED_WRITE_DRAIN": begin
                    if (PEER_FAULT_TEST != 1) $fatal(1, "PEER_CASE_REQUIRES_PEER_FAULT_PROFILE");
                    stall_r=1;
                    configure_read(READ_BASE*4,4);
                    configure_write(WRITE_BASE*4,24);
                    stall_aw=1; stall_w=1; stall_b=1;
                    issue_store_region(WRITE_BASE*4,24,32'h8c000000);
                    timeout_cycles=0;
                    while ((writer.descriptor_count!=2 || !awvalid || !wvalid) && timeout_cycles<2000) begin
                        @(negedge clock); timeout_cycles=timeout_cycles+1;
                    end
                    if (timeout_cycles>=2000 || integer'(writer.scheduled)!=16 ||
                        writer.fifo_count-writer.reserved_data_count!=8)
                        $fatal(1, "PEER_RRESP_WRITE_DESCRIPTORS_NOT_FULLY_RESERVED desc=%0d scheduled=%0d aw=%b w=%b",
                               writer.descriptor_count, writer.scheduled, awvalid, wvalid);
                    before_scheduled=integer'(writer.scheduled);
                    before_aw=accepted_aw; before_w=accepted_w; before_b=accepted_b;
                    before_r=read_beats;
                    issue_load(READ_BASE*4);
                    stall_r=0;
                    timeout_cycles=0;
                    while (read_beats<before_r+1 && timeout_cycles<1000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (timeout_cycles>=1000) $fatal(1, "PEER_RRESP_FIRST_GOOD_BEAT_TIMEOUT");
                    inject_rresp_fault=1;
                    timeout_cycles=0;
                    while (!read_fault && timeout_cycles<1000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (!read_fault || read_done || integer'(writer.scheduled)!=before_scheduled)
                        $fatal(1, "PEER_RRESP_DID_NOT_STOP_NEW_WRITER_RESERVATIONS");
                    expected_r_beats=before_r+4;
                    timeout_cycles=0;
                    while ((read_beats<expected_r_beats || reader.descriptor_count!=0) && timeout_cycles<1000) begin
                        @(negedge clock); timeout_cycles=timeout_cycles+1;
                    end
                    if (timeout_cycles>=1000 || read_beats!=expected_r_beats || reader.descriptor_count!=0)
                        $fatal(1, "PEER_RRESP_READ_TRANSPORT_NOT_DRAINED r=%0d/%0d desc=%0d",
                               read_beats-before_r, expected_r_beats-before_r, reader.descriptor_count);
                    timeout_cycles=0;
                    while (!write_fault && timeout_cycles<1000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (!write_fault || !awvalid || !wvalid || integer'(writer.scheduled)!=before_scheduled)
                        $fatal(1, "PEER_RRESP_DID_NOT_RETAIN_RESERVED_WRITE_TRANSPORT");
                    saved_araddr=awaddr; saved_arlen=awlen; saved_wdata=wdata; saved_wlast=wlast;
                    repeat (3) begin
                        @(negedge clock);
                        if (!awvalid || !wvalid || awaddr!==saved_araddr || awlen!==saved_arlen ||
                            wdata!==saved_wdata || wlast!==saved_wlast ||
                            integer'(writer.scheduled)!=before_scheduled)
                            $fatal(1, "PEER_RRESP_STALLED_RESERVED_WRITE_CHANGED");
                    end
                    stall_aw=0; stall_w=0;
                    timeout_cycles=0;
                    while ((b_pending<2 || accepted_aw<before_aw+2 || accepted_w<before_w+16) && timeout_cycles<3000) begin
                        @(negedge clock); timeout_cycles=timeout_cycles+1;
                    end
                    if (timeout_cycles>=3000 || b_pending!=2 || accepted_b!=before_b)
                        $fatal(1, "PEER_RRESP_B_RESPONSES_NOT_PENDING_BEFORE_RELEASE");
                    stall_b=0;
                    timeout_cycles=0;
                    while (accepted_b<before_b+2 && timeout_cycles<3000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (timeout_cycles>=3000 || accepted_aw!=before_aw+2 || accepted_w!=before_w+16 ||
                        accepted_b!=before_b+2 || writer.descriptor_count!=0 ||
                        integer'(writer.scheduled)!=before_scheduled ||
                        writer.fifo_count-writer.reserved_data_count!=8)
                        $fatal(1, "PEER_RRESP_RESERVED_WRITE_DID_NOT_DRAIN aw=%0d w=%0d b=%0d desc=%0d",
                               accepted_aw-before_aw, accepted_w-before_w, accepted_b-before_b, writer.descriptor_count);
                    expected_write_beats=16; check_write_counts(expected_write_beats);
                    for (integer k=16;k<24;k=k+1) expected_valid[WRITE_BASE+k]=1'b0;
                    verify_region(WRITE_BASE,16);
                end
                "PEER_RRESP_FINAL_BEAT": begin
                    if (PEER_FAULT_TEST != 1) $fatal(1, "PEER_CASE_REQUIRES_PEER_FAULT_PROFILE");
                    stall_r=1;
                    configure_read(READ_BASE*4,4);
                    before_r=read_beats;
                    issue_load(READ_BASE*4);
                    stall_r=0;
                    timeout_cycles=0;
                    while (read_beats<before_r+3 && timeout_cycles<1000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (timeout_cycles>=1000 || reader.descriptor_count!=1 ||
                        reader.descriptor_remaining[reader.descriptor_rd_ptr]!=1)
                        $fatal(1, "PEER_RRESP_FINAL_BEAT_PRECONDITION_FAILED");
                    inject_rresp_fault=1;
                    timeout_cycles=0;
                    while (!read_fault && timeout_cycles<1000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (timeout_cycles>=1000 || read_done || read_beats!=before_r+4 || reader.descriptor_count!=0)
                        $fatal(1, "PEER_RRESP_FINAL_BEAT_POP_FAULT_EDGE_MISMATCH beats=%0d desc=%0d",
                               read_beats-before_r, reader.descriptor_count);
                    timeout_cycles=0;
                    while (!write_fault && timeout_cycles<1000) begin
                        @(negedge clock); timeout_cycles=timeout_cycles+1;
                    end
                    if (timeout_cycles>=1000)
                        $fatal(1, "PEER_RRESP_FINAL_BEAT_NOT_PROPAGATED_TO_WRITER");
                end
                "PEER_FAULT_BLOCKS_RESERVATION": begin
                    if (PEER_FAULT_TEST != 1) $fatal(1, "PEER_CASE_REQUIRES_PEER_FAULT_PROFILE");
                    configure_read(READ_BASE*4,40);
                    timeout_cycles=0;
                    while (accepted_ar<1 && timeout_cycles<1000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (timeout_cycles>=1000) $fatal(1, "PEER_RESERVATION_FIRST_AR_TIMEOUT");
                    stall_ar=1;
                    before_issued=integer'(reader.issued);
                    before_descriptors=integer'(reader.descriptor_count);
                    if (!reader.region_active || reader.issued>=reader.region_count || reader.m_axi_arvalid_reg ||
                        reader.fault_reg || before_descriptors>=READ_OUTSTANDING)
                        $fatal(1, "PEER_RESERVATION_EDGE_NOT_ELIGIBLE issued=%0d desc=%0d arvalid=%b",
                               reader.issued, reader.descriptor_count, reader.m_axi_arvalid_reg);
                    inject_peer_fault_reader=1;
                    @(posedge clock); #1;
                    if (!read_fault || integer'(reader.issued)!=before_issued ||
                        integer'(reader.descriptor_count)!=before_descriptors ||
                        reader.m_axi_arvalid_reg)
                        $fatal(1, "PEER_FAULT_DID_NOT_BLOCK_SAME_EDGE_RESERVATION");
                end
                "PEER_FAULT_RESET_RESTART": begin
                    if (PEER_FAULT_TEST != 1) $fatal(1, "PEER_CASE_REQUIRES_PEER_FAULT_PROFILE");
                    stall_r=1;
                    configure_read(READ_BASE*4,4);
                    configure_write(WRITE_BASE*4,4);
                    timeout_cycles=0;
                    while (accepted_ar<1 && timeout_cycles<1000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (timeout_cycles>=1000 || reader.descriptor_count!=1)
                        $fatal(1, "PEER_FAULT_RESET_DRAIN_PRECONDITION_FAILED");
                    before_r=read_beats;
                    inject_peer_fault_writer=1;
                    timeout_cycles=0;
                    while ((!read_fault || !write_fault) && timeout_cycles<1000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (timeout_cycles>=1000) $fatal(1, "PEER_FAULT_DID_NOT_REACH_BOTH_CORES");
                    inject_peer_fault_writer=0;
                    stall_r=0;
                    timeout_cycles=0;
                    while ((read_beats<before_r+4 || reader.descriptor_count!=0) && timeout_cycles<2000) begin
                        @(negedge clock); timeout_cycles=timeout_cycles+1;
                    end
                    if (timeout_cycles>=2000 || read_beats!=before_r+4 || reader.descriptor_count!=0)
                        $fatal(1, "PEER_FAULT_RESET_DRAIN_DID_NOT_COMPLETE");
                    reset_cores();
                    stall_r=1;
                    configure_read(READ_BASE*4,4);
                    configure_write(WRITE_BASE*4,4);
                    issue_store_region(WRITE_BASE*4,4,32'h8d000000);
                    wait_write_complete(1,2000);
                    drain_write(1);
                    expected_write_beats=4; check_write_counts(expected_write_beats);
                    before_r=read_beats;
                    stall_r=0;
                    wait_read_beats(before_r+4,1000);
                    for (integer k=0;k<4;k=k+1) issue_load_and_check(READ_BASE*4+k*4);
                    if (read_fault || write_fault || accepted_aw!=1 || accepted_b!=1 || read_beats!=before_r+4)
                        $fatal(1, "PEER_FAULT_RESET_RESTART_FAILED read=%b write=%b r=%0d",
                               read_fault, write_fault, read_beats-before_r);
                    verify_region(WRITE_BASE,4);
                end
                "PROFILE_AND_CONFIG_ISOLATION": begin
                    stall_ar=1;
                    configure_read(READ_BASE*4, 13);
                    timeout_cycles=0;
                    while (!arvalid && timeout_cycles<1000) begin @(negedge clock); timeout_cycles=timeout_cycles+1; end
                    if (!arvalid || araddr !== READ_BASE*4 || arlen !== 8'd12)
                        $fatal(1, "READ_PROFILE_AR_NOT_STALLED_AS_CONFIGURED");
                    saved_araddr=araddr; saved_arlen=arlen;
                    configure_write(WRITE_BASE*4, 5);
                    if (!arvalid || araddr !== saved_araddr || arlen !== saved_arlen)
                        $fatal(1, "WRITER_CONFIG_CHANGED_STALLED_READ_AR");
                    if (READ_B_MAX != 16 || READ_OUTSTANDING != 3 || READ_FIFO_DEPTH != 64 ||
                        WRITE_B_MAX != 8 || WRITE_OUTSTANDING != 2 || WRITE_FIFO_DEPTH != 32)
                        $fatal(1, "PROFILE_OVERRIDE_MISMATCH");
                    issue_store_region(WRITE_BASE*4,5,32'h87000000);
                    wait_write_complete(1,2000); drain_write(1);
                    check_write_counts(5);
                    if (last_aw_addr !== WRITE_BASE*4 || last_aw_len != 4)
                        $fatal(1, "WRITE_PROFILE_AW_MISMATCH addr=%h len=%0d", last_aw_addr, last_aw_len);
                    if (!arvalid || araddr !== saved_araddr || arlen !== saved_arlen)
                        $fatal(1, "STALLED_READ_AR_CHANGED_DURING_WRITES");
                    stall_ar=0;
                    wait_read_beats(13,2000);
                    if (last_ar_len != 12 || last_aw_len != 4)
                        $fatal(1, "UNEQUAL_PROFILE_BURST_LENGTHS arlen=%0d awlen=%0d", last_ar_len, last_aw_len);
                    verify_region(WRITE_BASE,5);
                    for (integer k=0;k<13;k=k+1) issue_load_and_check(READ_BASE*4+k*4);
                    if (last_ar_addr !== READ_BASE*4 || last_ar_len != 12 || read_beats != 13)
                        $fatal(1, "READ_PROFILE_TRAFFIC_MISMATCH addr=%h len=%0d beats=%0d", last_ar_addr, last_ar_len, read_beats);
                    reset_cores();
                    configure_write((WRITE_BASE+64)*4, 3);
                    configure_read((READ_BASE+64)*4, 7);
                    issue_store_region((WRITE_BASE+64)*4,3,32'h88000000);
                    wait_write_complete(1,2000); drain_write(1);
                    expected_write_beats=3; check_write_counts(expected_write_beats);
                    if (last_aw_addr !== (WRITE_BASE+64)*4 || last_aw_len != 2)
                        $fatal(1, "READER_CONFIG_CHANGED_WRITER_PROFILE addr=%h len=%0d", last_aw_addr, last_aw_len);
                    for (integer k=0;k<7;k=k+1) issue_load_and_check((READ_BASE+64)*4+k*4);
                    wait_read_beats(7,2000);
                    if (last_ar_addr !== (READ_BASE+64)*4 || last_ar_len != 6 || read_beats != 7)
                        $fatal(1, "SECOND_READ_PROFILE_TRAFFIC_MISMATCH addr=%h len=%0d beats=%0d", last_ar_addr, last_ar_len, read_beats);
                    verify_region(WRITE_BASE+64,3);
                end
                default: $fatal(1, "UNKNOWN_CASE=%s", name);
            endcase
            if ((name != "RRESP_FAULT_LOCAL" && name != "PEER_BRESP_DRAIN" &&
                 name != "PEER_RRESP_RESERVED_WRITE_DRAIN" && name != "PEER_RRESP_FINAL_BEAT" &&
                 name != "PEER_FAULT_BLOCKS_RESERVATION" && name != "PEER_FAULT_RESET_RESTART" && read_fault) ||
                (name != "BRESP_FAULT_LOCAL" && name != "PEER_BRESP_DRAIN" &&
                 name != "PEER_RRESP_RESERVED_WRITE_DRAIN" && name != "PEER_RRESP_FINAL_BEAT" &&
                 name != "PEER_FAULT_BLOCKS_RESERVATION" && name != "PEER_FAULT_RESET_RESTART" && write_fault))
                $fatal(1, "UNEXPECTED_CORE_FAULT read=%b write=%b", read_fault, write_fault);
            verify_full_memory();
            $display("DUPLEX_CORES PASS CASE=%s AR=%0d R=%0d AW=%0d W=%0d B=%0d", name,
                     accepted_ar, read_beats, accepted_aw, accepted_w, accepted_b);
        end
    endtask

    integer i;
    string selected_case;
    initial begin
        for (i=0;i<MEM_WORDS;i=i+1) begin
            memory[i]=32'hcafe0000+i;
            initial_memory[i]=memory[i];
            read_snapshot[i]=memory[i];
            expected[i]=0;
            expected_valid[i]=0;
        end
        for (i=0;i<128;i=i+1) begin
            memory[READ_BASE+i]=(READ_BASE+i)^32'h5a5a0000;
            initial_memory[READ_BASE+i]=memory[READ_BASE+i];
            read_snapshot[READ_BASE+i]=memory[READ_BASE+i];
        end
        if (!$value$plusargs("CASE=%s", selected_case)) $fatal(1, "CASE_PLUSARG_REQUIRED");
            run_selected(selected_case);
        repeat (5) @(posedge clock);
        $finish;
    end
endmodule
