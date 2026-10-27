// Cycle-by-cycle comparison with an explicit pre-refactor memory model.
// Deterministic independent AR/AW/W traffic includes bursts, stalls, aliasing
// and a reset with traffic in flight. Backend call order and contents matter.
module tb_testbench_memaxi_equivalence;
 timeunit 1ns; timeprecision 1ps;
 parameter integer WRITE_DELAY=1;
logic clock = '0;
logic reset = '0;
logic arid = '0;
logic [31:0] araddr = '0;
logic [7:0] arlen = '0;
logic [2:0] arsize = '0;
logic [1:0] arburst = '0;
logic arlock = '0;
logic [3:0] arcache = '0;
logic [2:0] arprot = '0;
logic [3:0] arqos = '0;
logic [3:0] arregion = '0;
logic aruser = '0;
logic arvalid = '0;
wire gold_arready;
wire gate_arready;
wire gold_rid;
wire gate_rid;
wire [31:0] gold_rdata;
wire [31:0] gate_rdata;
wire [1:0] gold_rresp;
wire [1:0] gate_rresp;
wire [1:0] gold_ruser;
wire [1:0] gate_ruser;
wire gold_rlast;
wire gate_rlast;
wire gold_rvalid;
wire gate_rvalid;
logic rready = '0;
logic awid = '0;
logic [31:0] awaddr = '0;
logic [7:0] awlen = '0;
logic [2:0] awsize = '0;
logic [1:0] awburst = '0;
logic awlock = '0;
logic [3:0] awcache = '0;
logic [2:0] awprot = '0;
logic [3:0] awqos = '0;
logic [3:0] awregion = '0;
logic awuser = '0;
logic awvalid = '0;
wire gold_awready;
wire gate_awready;
logic [31:0] wdata = '0;
logic [3:0] wstrb = '0;
logic wlast = '0;
logic wvalid = '0;
wire gold_wready;
wire gate_wready;
logic wuser = '0;
wire gold_bid;
wire gate_bid;
wire [1:0] gold_bresp;
wire [1:0] gate_bresp;
wire [1:0] gold_buser;
wire [1:0] gate_buser;
wire gold_bvalid;
wire gate_bvalid;
logic bready = '0;
always #5 clock=~clock;
BaselineTestbenchMEMAXI #(.QUEUE_SIZE(8),
    .WRITE_DELAY(WRITE_DELAY),
    .READ_DELAY(3)) gold_dut(
    .clock(clock),
    .reset(reset),
    .arid(arid),
    .araddr(araddr),
    .arlen(arlen),
    .arsize(arsize),
    .arburst(arburst),
    .arlock(arlock),
    .arcache(arcache),
    .arprot(arprot),
    .arqos(arqos),
    .arregion(arregion),
    .aruser(aruser),
    .arvalid(arvalid),
    .arready(gold_arready),
    .rid(gold_rid),
    .rdata(gold_rdata),
    .rresp(gold_rresp),
    .ruser(gold_ruser),
    .rlast(gold_rlast),
    .rvalid(gold_rvalid),
    .rready(rready),
    .awid(awid),
    .awaddr(awaddr),
    .awlen(awlen),
    .awsize(awsize),
    .awburst(awburst),
    .awlock(awlock),
    .awcache(awcache),
    .awprot(awprot),
    .awqos(awqos),
    .awregion(awregion),
    .awuser(awuser),
    .awvalid(awvalid),
    .awready(gold_awready),
    .wdata(wdata),
    .wstrb(wstrb),
    .wlast(wlast),
    .wvalid(wvalid),
    .wready(gold_wready),
    .wuser(wuser),
    .bid(gold_bid),
    .bresp(gold_bresp),
    .buser(gold_buser),
    .bvalid(gold_bvalid),
    .bready(bready));
CandidateTestbenchMEMAXI #(.QUEUE_SIZE(8),
    .WRITE_DELAY(WRITE_DELAY),
    .READ_DELAY(3)) gate_dut(
    .clock(clock),
    .reset(reset),
    .arid(arid),
    .araddr(araddr),
    .arlen(arlen),
    .arsize(arsize),
    .arburst(arburst),
    .arlock(arlock),
    .arcache(arcache),
    .arprot(arprot),
    .arqos(arqos),
    .arregion(arregion),
    .aruser(aruser),
    .arvalid(arvalid),
    .arready(gate_arready),
    .rid(gate_rid),
    .rdata(gate_rdata),
    .rresp(gate_rresp),
    .ruser(gate_ruser),
    .rlast(gate_rlast),
    .rvalid(gate_rvalid),
    .rready(rready),
    .awid(awid),
    .awaddr(awaddr),
    .awlen(awlen),
    .awsize(awsize),
    .awburst(awburst),
    .awlock(awlock),
    .awcache(awcache),
    .awprot(awprot),
    .awqos(awqos),
    .awregion(awregion),
    .awuser(awuser),
    .awvalid(awvalid),
    .awready(gate_awready),
    .wdata(wdata),
    .wstrb(wstrb),
    .wlast(wlast),
    .wvalid(wvalid),
    .wready(gate_wready),
    .wuser(wuser),
    .bid(gate_bid),
    .bresp(gate_bresp),
    .buser(gate_buser),
    .bvalid(gate_bvalid),
    .bready(bready));

integer ar_index_reg=0, aw_index_reg=0, w_index_reg=0, w_beat_reg=0;
reg ar_accepted_reg=0, aw_accepted_reg=0, w_accepted_reg=0;
function integer beats(input integer index);
  beats = ((index * 7) % 8) + 1;
endfunction
always @(posedge clock)
begin
  ar_accepted_reg <= reset && arvalid && gold_arready;
  if (!reset) ar_index_reg <= 0;
  else if (arvalid && gold_arready) ar_index_reg <= ar_index_reg+1;
end
always @(posedge clock)
begin
  aw_accepted_reg <= reset && awvalid && gold_awready;
  if (!reset) aw_index_reg <= 0;
  else if (awvalid && gold_awready) aw_index_reg <= aw_index_reg+1;
end
always @(posedge clock)
begin
  w_accepted_reg <= reset && wvalid && gold_wready;
  if (!reset)
  begin
    w_index_reg<=0; w_beat_reg<=0;
  end
  else if (wvalid && gold_wready)
  begin
    if (w_beat_reg == beats(w_index_reg)-1)
    begin
      w_index_reg<=w_index_reg+1; w_beat_reg<=0;
    end
    else w_beat_reg<=w_beat_reg+1;
  end
end
integer tick, k, checked_reads=0, checked_writes=0;
reg [31:0] random_reg;
initial
begin
  random_reg=32'h815a639d;
  reset=0;
  arsize=2; awsize=2;
  repeat(3) @(negedge clock);
  reset=1;
  for(tick=0;tick<3000;tick=tick+1)
  begin
    @(negedge clock);
    if (gold_arready !== gate_arready) $fatal(1,"DIFFERENTIAL arready tick=%0d",tick);
    if (gold_rid !== gate_rid) $fatal(1,"DIFFERENTIAL rid tick=%0d",tick);
    if (gold_rdata !== gate_rdata) $fatal(1,"DIFFERENTIAL rdata tick=%0d",tick);
    if (gold_rresp !== gate_rresp) $fatal(1,"DIFFERENTIAL rresp tick=%0d",tick);
    if (gold_ruser !== gate_ruser) $fatal(1,"DIFFERENTIAL ruser tick=%0d",tick);
    if (gold_rlast !== gate_rlast) $fatal(1,"DIFFERENTIAL rlast tick=%0d",tick);
    if (gold_rvalid !== gate_rvalid) $fatal(1,"DIFFERENTIAL rvalid tick=%0d",tick);
    if (gold_awready !== gate_awready) $fatal(1,"DIFFERENTIAL awready tick=%0d",tick);
    if (gold_wready !== gate_wready) $fatal(1,"DIFFERENTIAL wready tick=%0d",tick);
    if (gold_bid !== gate_bid) $fatal(1,"DIFFERENTIAL bid tick=%0d",tick);
    if (gold_bresp !== gate_bresp) $fatal(1,"DIFFERENTIAL bresp tick=%0d",tick);
    if (gold_buser !== gate_buser) $fatal(1,"DIFFERENTIAL buser tick=%0d",tick);
    if (gold_bvalid !== gate_bvalid) $fatal(1,"DIFFERENTIAL bvalid tick=%0d",tick);

    if (gold_dut.m_utils.read_calls != gate_dut.m_utils.read_calls ||
        gold_dut.m_utils.write_calls != gate_dut.m_utils.write_calls)
      $fatal(1,"DIFFERENTIAL backend call counts tick=%0d",tick);
    for(k=checked_reads;k<gold_dut.m_utils.read_calls;k=k+1)
      if(gold_dut.m_utils.read_addr[k] !== gate_dut.m_utils.read_addr[k])
        $fatal(1,"DIFFERENTIAL read address call=%0d",k);
    for(k=checked_writes;k<gold_dut.m_utils.write_calls;k=k+1)
      if({gold_dut.m_utils.write_addr[k],gold_dut.m_utils.write_data[k],gold_dut.m_utils.write_mask[k],gold_dut.m_utils.write_time[k]} !==
         {gate_dut.m_utils.write_addr[k],gate_dut.m_utils.write_data[k],gate_dut.m_utils.write_mask[k],gate_dut.m_utils.write_time[k]})
        $fatal(1,"DIFFERENTIAL write call=%0d",k);
    checked_reads=gold_dut.m_utils.read_calls;
    checked_writes=gold_dut.m_utils.write_calls;
    random_reg={random_reg[30:0],random_reg[31]^random_reg[21]^random_reg[1]^random_reg[0]};
    if(tick==1500) reset=0;
    if(tick==1503) reset=1;
    arvalid = reset && ar_index_reg<80 && ((arvalid && !ar_accepted_reg) || random_reg[0]);
    awvalid = reset && aw_index_reg<80 && ((awvalid && !aw_accepted_reg) || random_reg[7]);
    wvalid = reset && w_index_reg<80 && ((wvalid && !w_accepted_reg) || random_reg[13]);
    rready = random_reg[21] || (tick%32==0);
    bready = random_reg[25] || (tick%32==0);
    arid=ar_index_reg%2; awid=aw_index_reg%2;
    araddr=(ar_index_reg%32)*64; awaddr=(aw_index_reg%32)*64;
    arlen=beats(ar_index_reg)-1; awlen=beats(aw_index_reg)-1;
    arburst=(ar_index_reg%2); awburst=(aw_index_reg%2);
    wdata=32'h12345678 ^ (w_index_reg*31+w_beat_reg);
    wstrb=(w_index_reg+w_beat_reg)%16;
    wlast=(w_beat_reg==beats(w_index_reg)-1);
  end
  if(ar_index_reg!=80 || aw_index_reg!=80 || w_index_reg!=80 || gold_bvalid || gold_rvalid)
    $fatal(1,"DIFFERENTIAL traffic did not drain ar=%0d aw=%0d w=%0d",ar_index_reg,aw_index_reg,w_index_reg);
  for(k=0;k<4096;k=k+1)
    if(gold_dut.m_utils.words[k] !== gate_dut.m_utils.words[k])
      $fatal(1,"DIFFERENTIAL final memory word=%0d",k);
  $display("MEM_DIFFERENTIAL_PASS delay=%0d cycles=3000 reads=%0d writes=%0d",WRITE_DELAY,checked_reads,checked_writes);
  $finish;
end
endmodule
