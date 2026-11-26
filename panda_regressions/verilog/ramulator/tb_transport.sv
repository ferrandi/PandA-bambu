// SPDX-License-Identifier: MIT
// Clock-edge contract with production models and real MDPI/Ramulator.
module tb_transport #(parameter AXI=0, RF=1, WF=1, RR=1, WR=1, RESET_INFLIGHT=0);
  import "DPI-C" function int unsigned m_next(input int unsigned state);
  reg clock=0;
  always #5 clock=!clock;
  reg reset=1, read_request=0, write_request=0, response_ready=0;
  wire read_ready, write_ready, read_valid, write_valid;
  wire [31:0] read_data;
  wire [63:0] model_cycle;
  wire submitted_write, submitted_read, completed_read;
  generate if(AXI) begin : axi
    wire arready, awready, wready, rvalid, bvalid;
    TestbenchMEMAXIRamulatorIntegrated #(.index(0),.AXI_PORT_ID(0),.QUEUE_SIZE(3),
      .READ_FORWARD(RF),.WRITE_FORWARD(WF),.READ_RETURN(RR),.WRITE_RETURN(WR),
      .BITSIZE_araddr(64),.BITSIZE_awaddr(64),.BITSIZE_arid(8),.BITSIZE_awid(8),
      .BITSIZE_rid(8),.BITSIZE_bid(8),.BITSIZE_arlen(8),.BITSIZE_awlen(8),
      .BITSIZE_arsize(3),.BITSIZE_awsize(3),.BITSIZE_arburst(2),.BITSIZE_awburst(2),
      .BITSIZE_rdata(32),.BITSIZE_wdata(32),.BITSIZE_wstrb(4),.BITSIZE_rresp(2),.BITSIZE_bresp(2)) dut(
      .clock(clock),.reset(reset),.arvalid(read_request),.arready(arready),.araddr(64'h40000000),
      .arid(8'd1),.arlen(8'd0),.arsize(3'd2),.arburst(2'd1),.arlock(1'b0),.arcache(1'b0),
      .arprot(1'b0),.arqos(1'b0),.arregion(1'b0),.aruser(1'b0),
      .rready(response_ready),.rvalid(rvalid),.rdata(read_data),.rid(),.rresp(),.rlast(),.ruser(),
      .awvalid(write_request),.awready(awready),.awaddr(64'h40000000),.awid(8'd2),.awlen(8'd0),
      .awsize(3'd2),.awburst(2'd1),.awlock(1'b0),.awcache(1'b0),.awprot(1'b0),.awqos(1'b0),
      .awregion(1'b0),.awuser(1'b0),.wvalid(write_request),.wready(wready),.wdata(32'h12345678),
      .wstrb(4'hf),.wlast(1'b1),.wuser(1'b0),.bready(response_ready),.bvalid(bvalid),.bid(),.bresp(),.buser());
    assign read_ready=arready;
    assign write_ready=awready && wready;
    assign read_valid=rvalid;
    assign write_valid=bvalid;
    assign model_cycle=dut.memory_cycle_reg;
    assign submitted_write=dut.aw_complete_reg[0];
    assign submitted_read=dut.ar_issued_reg[0]!=0;
    assign completed_read=dut.ar_data_valid_reg[0][0];
  end else begin : minimal
    wire ready,bp;
    TestbenchMEMRamulatorIntegrated #(.index(0),.QUEUE_SIZE(2),.base_addr(64'h40000000),
      .READ_FORWARD(RF),.WRITE_FORWARD(WF),.READ_RETURN(RR),.WRITE_RETURN(WR),
      .BITSIZE_Mout_addr_ram(64),.BITSIZE_Mout_Wdata_ram(32),.BITSIZE_Mout_data_ram_size(6),
      .BITSIZE_M_Rdata_ram(32),.BITSIZE_Mout_tag(8),.BITSIZE_M_tag(8)) dut(
      .clock(clock),.reset(reset),.setup_port(1'b0),.done_port(1'b0),
      .Mout_oe_ram(read_request),.Mout_we_ram(write_request),.Mout_addr_ram(64'h40000000),
      .Mout_Wdata_ram(32'h12345678),.Mout_data_ram_size(6'd32),.Mout_tag(8'd1),
      .M_back_pressure(!response_ready),.Mout_back_pressure(bp),.M_DataRdy(ready),.M_Rdata_ram(read_data),.M_tag());
    assign read_ready=!bp; assign write_ready=!bp;
    assign read_valid=ready; assign write_valid=ready;
    assign model_cycle=dut.memory_cycle_reg;
    assign submitted_write=dut.channel[0].completion_valid_reg;
    assign submitted_read=dut.channel[0].read_valid_reg[0];
    assign completed_read=dut.channel[0].completion_valid_reg;
  end endgenerate

  // Observe real backend state publication, not only its cycle counter. A
  // falling-edge backend would otherwise look like a fractional-cycle delay.
  always @(submitted_read or submitted_write or completed_read)
    if(!reset && (submitted_read || submitted_write || completed_read) && clock !== 1'b1)
      $fatal(1,"Ramulator backend state changed away from a rising edge");

  integer accepted, submitted, completed, observed;
  initial begin
    if((m_next(1) & 255) != 2) $fatal(1,"MDPI setup");
    repeat(3) @(negedge clock);
    reset=0;
    if(RESET_INFLIGHT) begin
      @(negedge clock); read_request=1;
      @(posedge clock);
      @(negedge clock); read_request=0;
      repeat(RF+2) @(negedge clock);
      reset=1;
      repeat(2) @(negedge clock);
      reset=0;
    end
    @(negedge clock); write_request=1;
    @(posedge clock);
    if(!write_ready) $fatal(1,"unexpected initial write backpressure");
    accepted=model_cycle;
    @(negedge clock); write_request=0;
    while(!submitted_write) @(negedge clock);
    submitted=model_cycle-1;
    if(submitted != accepted + (AXI && WF==0 ? 1 : WF))
      $fatal(1,"write ingress: accepted=%0d submitted=%0d WF=%0d",accepted,submitted,WF);
    response_ready=1;
    do begin @(posedge clock); observed=model_cycle; end while(!write_valid);
    if(observed != submitted + (WR>0 ? WR : 1))
      $fatal(1,"write return: submitted=%0d observed=%0d WR=%0d",submitted,observed,WR);
    @(negedge clock); response_ready=0;
    repeat(3) @(negedge clock);
    read_request=1;
    @(posedge clock);
    if(!read_ready) $fatal(1,"unexpected initial read backpressure");
    accepted=model_cycle;
    @(negedge clock); read_request=0;
    while(!submitted_read) @(negedge clock);
    submitted=model_cycle-1;
    if(submitted != accepted + (AXI && RF==0 ? 1 : RF))
      $fatal(1,"read ingress: accepted=%0d submitted=%0d RF=%0d",accepted,submitted,RF);
    while(!completed_read) @(negedge clock);
    completed=model_cycle-1;
    response_ready=1;
    do begin @(posedge clock); observed=model_cycle; end while(!read_valid);
    if(observed != completed + (RR>0 ? RR : 1))
      $fatal(1,"read return: completed=%0d observed=%0d RR=%0d",completed,observed,RR);
    if(read_data !== 32'h12345678) $fatal(1,"functional write/read mismatch");
    @(negedge clock);
    $display("PASS posedge transport AXI=%0d RF=%0d WF=%0d RR=%0d WR=%0d",AXI,RF,WF,RR,WR);
    $finish;
  end
  initial begin #200000; $fatal(1,"transport timeout"); end
endmodule
