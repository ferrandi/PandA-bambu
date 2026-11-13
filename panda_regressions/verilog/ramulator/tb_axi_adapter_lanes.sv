// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: MIT
module tb_axi_adapter_lanes;
parameter TAGGED = 0;
parameter WIDTH = 32;
localparam BYTES = WIDTH / 8;
reg clock = 0;
always #5 clock = !clock;
reg reset = 1;
reg Mout_we_ram = 0, Mout_oe_ram = 0;
reg [31 : 0] Mout_addr_ram = 0;
reg [WIDTH - 1 : 0] Mout_Wdata_ram = 0;
reg [7 : 0] Mout_data_ram_size = 8;
reg [1 : 0] Mout_tag = 0;
wire Mout_back_pressure, M_DataRdy;
wire [WIDTH - 1 : 0] M_Rdata_ram;
wire [1 : 0] M_tag;
reg m_axi_awready = 0, m_axi_wready = 0, m_axi_arready = 0;
reg m_axi_rvalid = 0;
reg [WIDTH - 1 : 0] m_axi_rdata = 0;
reg [1 : 0] m_axi_rid = 0;
wire m_axi_awvalid, m_axi_wvalid, m_axi_arvalid, m_axi_rready;
wire [31 : 0] m_axi_awaddr, m_axi_araddr;
wire [WIDTH - 1 : 0] m_axi_wdata;
wire [BYTES - 1 : 0] m_axi_wstrb;
wire [2 : 0] m_axi_awsize, m_axi_arsize;
reg [WIDTH - 1 : 0] pattern;
integer lane, size;

generate
  if(TAGGED)
  begin
    MinimalAXI4Adapter #(
      .BITSIZE_Mout_addr_ram(32), .BITSIZE_Mout_Wdata_ram(WIDTH), .BITSIZE_Mout_data_ram_size(8),
      .BITSIZE_Mout_tag(2), .BITSIZE_M_tag(2), .BITSIZE_M_Rdata_ram(WIDTH),
      .BITSIZE_m_axi_awaddr(32), .BITSIZE_m_axi_araddr(32),
      .BITSIZE_m_axi_wdata(WIDTH), .BITSIZE_m_axi_wstrb(BYTES), .BITSIZE_m_axi_rdata(WIDTH),
      .BITSIZE_m_axi_awid(2), .BITSIZE_m_axi_arid(2), .BITSIZE_m_axi_rid(2), .BITSIZE_m_axi_bid(2)
    ) dut(.clock(clock), .reset(reset), .Mout_we_ram(Mout_we_ram), .Mout_oe_ram(Mout_oe_ram),
      .Mout_addr_ram(Mout_addr_ram), .Mout_Wdata_ram(Mout_Wdata_ram), .Mout_data_ram_size(Mout_data_ram_size),
      .Mout_tag(Mout_tag), .Mout_back_pressure(Mout_back_pressure), .M_back_pressure(1'b0),
      .M_DataRdy(M_DataRdy), .M_Rdata_ram(M_Rdata_ram), .M_tag(M_tag),
      .m_axi_awready(m_axi_awready), .m_axi_wready(m_axi_wready), .m_axi_arready(m_axi_arready),
      .m_axi_awvalid(m_axi_awvalid), .m_axi_wvalid(m_axi_wvalid), .m_axi_arvalid(m_axi_arvalid),
      .m_axi_awaddr(m_axi_awaddr), .m_axi_araddr(m_axi_araddr), .m_axi_wdata(m_axi_wdata), .m_axi_wstrb(m_axi_wstrb),
      .m_axi_awsize(m_axi_awsize), .m_axi_arsize(m_axi_arsize),
      .m_axi_rvalid(m_axi_rvalid), .m_axi_rready(m_axi_rready), .m_axi_rdata(m_axi_rdata),
      .m_axi_rid(m_axi_rid), .m_axi_rresp(2'b0), .m_axi_rlast(1'b1),
      .m_axi_bvalid(1'b0), .m_axi_bid(2'b0), .m_axi_bresp(2'b0));
  end
  else
  begin
    MinimalAXI4AdapterSingleBeat #(
      .BITSIZE_Mout_addr_ram(32), .BITSIZE_Mout_Wdata_ram(WIDTH), .BITSIZE_Mout_data_ram_size(8),
      .BITSIZE_M_Rdata_ram(WIDTH), .BITSIZE_m_axi_awaddr(32), .BITSIZE_m_axi_araddr(32),
      .BITSIZE_m_axi_wdata(WIDTH), .BITSIZE_m_axi_wstrb(BYTES), .BITSIZE_m_axi_rdata(WIDTH)
    ) dut(.clock(clock), .reset(reset), .Mout_we_ram(Mout_we_ram), .Mout_oe_ram(Mout_oe_ram),
      .Mout_addr_ram(Mout_addr_ram), .Mout_Wdata_ram(Mout_Wdata_ram), .Mout_data_ram_size(Mout_data_ram_size),
      .M_DataRdy(M_DataRdy), .M_Rdata_ram(M_Rdata_ram),
      .m_axi_awready(m_axi_awready), .m_axi_wready(m_axi_wready), .m_axi_arready(m_axi_arready),
      .m_axi_awvalid(m_axi_awvalid), .m_axi_wvalid(m_axi_wvalid), .m_axi_arvalid(m_axi_arvalid),
      .m_axi_awaddr(m_axi_awaddr), .m_axi_araddr(m_axi_araddr), .m_axi_wdata(m_axi_wdata), .m_axi_wstrb(m_axi_wstrb),
      .m_axi_awsize(m_axi_awsize), .m_axi_arsize(m_axi_arsize),
      .m_axi_rvalid(m_axi_rvalid), .m_axi_rready(m_axi_rready), .m_axi_rdata(m_axi_rdata),
      .m_axi_rid(1'b0), .m_axi_rresp(2'b0), .m_axi_rlast(1'b1),
      .m_axi_bvalid(1'b0), .m_axi_bid(1'b0), .m_axi_bresp(2'b0));
  end
endgenerate

task issue_read(input integer offset, input integer tag_value);
begin
  @(negedge clock);
  Mout_addr_ram = 32'h1000 + offset;
  Mout_tag = tag_value;
  Mout_oe_ram = 1;
  @(negedge clock);
  Mout_oe_ram = 0;
  Mout_addr_ram = 0;
  repeat(3)
  begin
    if(!m_axi_arvalid || m_axi_araddr !== 32'h1000 + offset)
    begin
      $fatal(1, "Read address changed during stall");
    end
    @(negedge clock);
  end
  m_axi_arready = 1;
  @(negedge clock);
  m_axi_arready = 0;
end
endtask

task respond_read(input integer offset, input integer tag_value);
begin
  m_axi_rid = tag_value;
  m_axi_rdata = pattern;
  m_axi_rvalid = 1;
  #1;
  if(!m_axi_rready || !M_DataRdy || M_Rdata_ram !== (pattern >> (offset * 8)))
  begin
    $fatal(1, "Wrong read lane at offset %0d", offset);
  end
  if(TAGGED && M_tag !== tag_value)
  begin
    $fatal(1, "Wrong response tag");
  end
  @(negedge clock);
  m_axi_rvalid = 0;
end
endtask

initial
begin
  pattern = '0;
  for(lane = 0; lane < BYTES; lane = lane + 1)
  begin
    pattern = pattern | (WIDTH'('h11 + lane) << (lane * 8));
  end
  repeat(3) @(negedge clock);
  reset = 0;
  repeat(2) @(negedge clock);
  for(size = 1; size <= BYTES; size = size * 2)
  begin
    for(lane = 0; lane < BYTES; lane = lane + size)
    begin
      Mout_addr_ram = 32'h1000 + lane;
      Mout_data_ram_size = size * 8;
      Mout_Wdata_ram = pattern;
      Mout_we_ram = 1;
      @(negedge clock);
      Mout_we_ram = 0;
      Mout_addr_ram = 0;
      Mout_Wdata_ram = 0;
      repeat(3)
      begin
        if(!m_axi_awvalid || !m_axi_wvalid || m_axi_awaddr !== 32'h1000 + lane ||
           m_axi_wdata !== (pattern << (lane * 8)) ||
           m_axi_wstrb !== BYTES'(((1 << size) - 1) << lane) || m_axi_awsize !== $clog2(size))
        begin
          $fatal(1, "Wrong write lane or unstable request at offset %0d size %0d", lane, size);
        end
        @(negedge clock);
      end
      // Accept the address before the data to exercise independent channels.
      m_axi_awready = 1;
      @(negedge clock);
      m_axi_awready = 0;
      if(!m_axi_wvalid || m_axi_wdata !== (pattern << (lane * 8)))
      begin
        $fatal(1, "Write data changed after address handshake");
      end
      m_axi_wready = 1;
      @(negedge clock);
      m_axi_wready = 0;
      issue_read(lane, 0);
      respond_read(lane, 0);
    end
  end
  if(TAGGED && BYTES > 1)
  begin
    issue_read(0, 1);
    issue_read(BYTES - 1, 2);
    respond_read(BYTES - 1, 2);
    respond_read(0, 1);
  end
  $display("PASS AXI adapter lanes");
  $finish;
end
initial
begin
  #100000;
  $fatal(1, "Timeout");
end
endmodule
