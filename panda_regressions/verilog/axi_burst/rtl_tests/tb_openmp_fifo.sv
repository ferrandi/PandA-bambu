module tb_openmp_fifo;
parameter CONTEXT_BITS=4;
parameter FILTER=0;
parameter CS=1;
localparam DEPTH=1 << CONTEXT_BITS;
reg clock=0;
always #5 clock=~clock;
reg reset=0;
reg Mout_oe_ram=0;
reg Mout_we_ram=0;
reg [31:0] Mout_addr_ram=0;
reg [31:0] Mout_Wdata_ram=0;
reg [6:0] Mout_data_ram_size=0;
reg [7:0] Mout_tag=0;
wire ref_Mout_back_pressure;
wire new_Mout_back_pressure;
wire ref_Mout_oe_ram_memory;
wire new_Mout_oe_ram_memory;
wire ref_Mout_we_ram_memory;
wire new_Mout_we_ram_memory;
wire [31:0] ref_Mout_addr_ram_memory;
wire [31:0] new_Mout_addr_ram_memory;
wire [31:0] ref_Mout_Wdata_ram_memory;
wire [31:0] new_Mout_Wdata_ram_memory;
wire [6:0] ref_Mout_data_ram_size_memory;
wire [6:0] new_Mout_data_ram_size_memory;
wire [7:0] ref_Mout_tag_memory;
wire [7:0] new_Mout_tag_memory;
reg Mout_back_pressure_memory=0;
wire ref_Mout_oe_ram_local;
wire new_Mout_oe_ram_local;
wire ref_Mout_we_ram_local;
wire new_Mout_we_ram_local;
wire [31:0] ref_Mout_addr_ram_local;
wire [31:0] new_Mout_addr_ram_local;
wire [31:0] ref_Mout_Wdata_ram_local;
wire [31:0] new_Mout_Wdata_ram_local;
wire [6:0] ref_Mout_data_ram_size_local;
wire [6:0] new_Mout_data_ram_size_local;
wire [7:0] ref_Mout_tag_local;
wire [7:0] new_Mout_tag_local;
reg M_DataRdy_memory=0;
reg [31:0] M_Rdata_ram_memory=0;
wire [7:0] ref_M_tag;
wire [7:0] new_M_tag;
reg M_DataRdy_local=0;
reg [31:0] M_Rdata_ram_local=0;
reg [7:0] M_tag_local=0;
wire ref_M_DataRdy;
wire new_M_DataRdy;
wire [31:0] ref_M_Rdata_ram;
wire [31:0] new_M_Rdata_ram;
reg [7:0] M_tag_memory=0;
wire ref_M_back_pressure_local;
wire new_M_back_pressure_local;
GMIReference #(.USE_CS(CS),.CONTEXT_NUMBER_SIZE(CONTEXT_BITS),.USE_FILTER(FILTER),.INNER_TAG(1),.EXTERNAL_BASE_ADDRESS(2147483648),.BITSIZE_Mout_addr_ram(32),.BITSIZE_Mout_Wdata_ram(32),.BITSIZE_Mout_data_ram_size(7),.BITSIZE_Mout_tag(8),.BITSIZE_Mout_addr_ram_memory(32),.BITSIZE_Mout_Wdata_ram_memory(32),.BITSIZE_Mout_data_ram_size_memory(7),.BITSIZE_Mout_tag_memory(8),.BITSIZE_Mout_addr_ram_local(32),.BITSIZE_Mout_Wdata_ram_local(32),.BITSIZE_Mout_data_ram_size_local(7),.BITSIZE_Mout_tag_local(8),.BITSIZE_M_Rdata_ram_memory(32),.BITSIZE_M_tag(8),.BITSIZE_M_Rdata_ram_local(32),.BITSIZE_M_tag_local(8),.BITSIZE_M_Rdata_ram(32),.BITSIZE_M_tag_memory(8)) ref_dut (.clock(clock),.reset(reset),.Mout_oe_ram(Mout_oe_ram),.Mout_we_ram(Mout_we_ram),.Mout_addr_ram(Mout_addr_ram),.Mout_Wdata_ram(Mout_Wdata_ram),.Mout_data_ram_size(Mout_data_ram_size),.Mout_tag(Mout_tag),.Mout_back_pressure(ref_Mout_back_pressure),.Mout_oe_ram_memory(ref_Mout_oe_ram_memory),.Mout_we_ram_memory(ref_Mout_we_ram_memory),.Mout_addr_ram_memory(ref_Mout_addr_ram_memory),.Mout_Wdata_ram_memory(ref_Mout_Wdata_ram_memory),.Mout_data_ram_size_memory(ref_Mout_data_ram_size_memory),.Mout_tag_memory(ref_Mout_tag_memory),.Mout_back_pressure_memory(Mout_back_pressure_memory),.Mout_oe_ram_local(ref_Mout_oe_ram_local),.Mout_we_ram_local(ref_Mout_we_ram_local),.Mout_addr_ram_local(ref_Mout_addr_ram_local),.Mout_Wdata_ram_local(ref_Mout_Wdata_ram_local),.Mout_data_ram_size_local(ref_Mout_data_ram_size_local),.Mout_tag_local(ref_Mout_tag_local),.M_DataRdy_memory(M_DataRdy_memory),.M_Rdata_ram_memory(M_Rdata_ram_memory),.M_tag(ref_M_tag),.M_DataRdy_local(M_DataRdy_local),.M_Rdata_ram_local(M_Rdata_ram_local),.M_tag_local(M_tag_local),.M_DataRdy(ref_M_DataRdy),.M_Rdata_ram(ref_M_Rdata_ram),.M_tag_memory(M_tag_memory),.M_back_pressure_local(ref_M_back_pressure_local));
GMICandidate #(.USE_CS(CS),.CONTEXT_NUMBER_SIZE(CONTEXT_BITS),.USE_FILTER(FILTER),.INNER_TAG(1),.EXTERNAL_BASE_ADDRESS(2147483648),.BITSIZE_Mout_addr_ram(32),.BITSIZE_Mout_Wdata_ram(32),.BITSIZE_Mout_data_ram_size(7),.BITSIZE_Mout_tag(8),.BITSIZE_Mout_addr_ram_memory(32),.BITSIZE_Mout_Wdata_ram_memory(32),.BITSIZE_Mout_data_ram_size_memory(7),.BITSIZE_Mout_tag_memory(8),.BITSIZE_Mout_addr_ram_local(32),.BITSIZE_Mout_Wdata_ram_local(32),.BITSIZE_Mout_data_ram_size_local(7),.BITSIZE_Mout_tag_local(8),.BITSIZE_M_Rdata_ram_memory(32),.BITSIZE_M_tag(8),.BITSIZE_M_Rdata_ram_local(32),.BITSIZE_M_tag_local(8),.BITSIZE_M_Rdata_ram(32),.BITSIZE_M_tag_memory(8)) new_dut (.clock(clock),.reset(reset),.Mout_oe_ram(Mout_oe_ram),.Mout_we_ram(Mout_we_ram),.Mout_addr_ram(Mout_addr_ram),.Mout_Wdata_ram(Mout_Wdata_ram),.Mout_data_ram_size(Mout_data_ram_size),.Mout_tag(Mout_tag),.Mout_back_pressure(new_Mout_back_pressure),.Mout_oe_ram_memory(new_Mout_oe_ram_memory),.Mout_we_ram_memory(new_Mout_we_ram_memory),.Mout_addr_ram_memory(new_Mout_addr_ram_memory),.Mout_Wdata_ram_memory(new_Mout_Wdata_ram_memory),.Mout_data_ram_size_memory(new_Mout_data_ram_size_memory),.Mout_tag_memory(new_Mout_tag_memory),.Mout_back_pressure_memory(Mout_back_pressure_memory),.Mout_oe_ram_local(new_Mout_oe_ram_local),.Mout_we_ram_local(new_Mout_we_ram_local),.Mout_addr_ram_local(new_Mout_addr_ram_local),.Mout_Wdata_ram_local(new_Mout_Wdata_ram_local),.Mout_data_ram_size_local(new_Mout_data_ram_size_local),.Mout_tag_local(new_Mout_tag_local),.M_DataRdy_memory(M_DataRdy_memory),.M_Rdata_ram_memory(M_Rdata_ram_memory),.M_tag(new_M_tag),.M_DataRdy_local(M_DataRdy_local),.M_Rdata_ram_local(M_Rdata_ram_local),.M_tag_local(M_tag_local),.M_DataRdy(new_M_DataRdy),.M_Rdata_ram(new_M_Rdata_ram),.M_tag_memory(M_tag_memory),.M_back_pressure_local(new_M_back_pressure_local));

reg [79:0] expected_reg [0:16383];
integer supplied=0;
integer reference_taken=0;
integer candidate_taken=0;
integer max_occupancy=0;
integer read_requests=0;
integer write_requests=0;
integer local_taken=0;
integer cycle=0;
reg [31:0] random_reg=32'hf7a1b032;
wire [79:0] source_word={Mout_oe_ram,Mout_addr_ram,Mout_Wdata_ram,Mout_data_ram_size,Mout_tag};
wire ref_valid=ref_Mout_oe_ram_memory || ref_Mout_we_ram_memory || ref_Mout_oe_ram_local || ref_Mout_we_ram_local;
wire new_valid=new_Mout_oe_ram_memory || new_Mout_we_ram_memory || new_Mout_oe_ram_local || new_Mout_we_ram_local;
wire ref_stalled=Mout_back_pressure_memory && (ref_Mout_oe_ram_memory || ref_Mout_we_ram_memory);
wire new_stalled=Mout_back_pressure_memory && (new_Mout_oe_ram_memory || new_Mout_we_ram_memory);
wire ref_local=ref_Mout_oe_ram_local || ref_Mout_we_ram_local;
wire new_local=new_Mout_oe_ram_local || new_Mout_we_ram_local;
wire [79:0] ref_word=ref_local ? {ref_Mout_oe_ram_local,ref_Mout_addr_ram_local,ref_Mout_Wdata_ram_local,ref_Mout_data_ram_size_local,ref_Mout_tag_local} :
 {ref_Mout_oe_ram_memory,ref_Mout_addr_ram_memory,ref_Mout_Wdata_ram_memory,ref_Mout_data_ram_size_memory,ref_Mout_tag_memory};
wire [79:0] new_word=new_local ? {new_Mout_oe_ram_local,new_Mout_addr_ram_local,new_Mout_Wdata_ram_local,new_Mout_data_ram_size_local,new_Mout_tag_local} :
 {new_Mout_oe_ram_memory,new_Mout_addr_ram_memory,new_Mout_Wdata_ram_memory,new_Mout_data_ram_size_memory,new_Mout_tag_memory};
always @(posedge clock)
begin
 if(!reset)
 begin
  supplied=0;reference_taken=0;candidate_taken=0;
 end
 else
 begin
  if(Mout_oe_ram || Mout_we_ram)
  begin
   expected_reg[supplied]=source_word;supplied=supplied+1;
   if(Mout_oe_ram)read_requests=read_requests+1;else write_requests=write_requests+1;
  end
  if(ref_valid && !ref_stalled)
  begin
   if(reference_taken>=supplied || ref_word !== expected_reg[reference_taken])
    $fatal(1,"REFERENCE_BAD_WORD cycle=%0d index=%0d word=%h expected=%h",cycle,reference_taken,ref_word,expected_reg[reference_taken]);
   reference_taken=reference_taken+1;
   if(ref_local)local_taken=local_taken+1;
  end
  if(new_valid && !new_stalled)
  begin
   if(candidate_taken>=supplied || new_word !== expected_reg[candidate_taken])
    $fatal(1,"CANDIDATE_BAD_WORD cycle=%0d index=%0d word=%h expected=%h",cycle,candidate_taken,new_word,expected_reg[candidate_taken]);
   candidate_taken=candidate_taken+1;
  end
  if(reference_taken!=candidate_taken)$fatal(1,"CYCLE_ACCEPTANCE_MISMATCH cycle=%0d",cycle);
  if(supplied-reference_taken>max_occupancy)max_occupancy=supplied-reference_taken;
 end
 #1;

 if(ref_Mout_back_pressure !== new_Mout_back_pressure)$fatal(1,"OUTPUT_MISMATCH Mout_back_pressure cycle=%0d ref=%h new=%h",cycle,ref_Mout_back_pressure,new_Mout_back_pressure);
 if(ref_Mout_oe_ram_memory !== new_Mout_oe_ram_memory)$fatal(1,"OUTPUT_MISMATCH Mout_oe_ram_memory cycle=%0d ref=%h new=%h",cycle,ref_Mout_oe_ram_memory,new_Mout_oe_ram_memory);
 if(ref_Mout_we_ram_memory !== new_Mout_we_ram_memory)$fatal(1,"OUTPUT_MISMATCH Mout_we_ram_memory cycle=%0d ref=%h new=%h",cycle,ref_Mout_we_ram_memory,new_Mout_we_ram_memory);
 if(ref_Mout_addr_ram_memory !== new_Mout_addr_ram_memory)$fatal(1,"OUTPUT_MISMATCH Mout_addr_ram_memory cycle=%0d ref=%h new=%h",cycle,ref_Mout_addr_ram_memory,new_Mout_addr_ram_memory);
 if(ref_Mout_Wdata_ram_memory !== new_Mout_Wdata_ram_memory)$fatal(1,"OUTPUT_MISMATCH Mout_Wdata_ram_memory cycle=%0d ref=%h new=%h",cycle,ref_Mout_Wdata_ram_memory,new_Mout_Wdata_ram_memory);
 if(ref_Mout_data_ram_size_memory !== new_Mout_data_ram_size_memory)$fatal(1,"OUTPUT_MISMATCH Mout_data_ram_size_memory cycle=%0d ref=%h new=%h",cycle,ref_Mout_data_ram_size_memory,new_Mout_data_ram_size_memory);
 if(ref_Mout_tag_memory !== new_Mout_tag_memory)$fatal(1,"OUTPUT_MISMATCH Mout_tag_memory cycle=%0d ref=%h new=%h",cycle,ref_Mout_tag_memory,new_Mout_tag_memory);
 if(ref_Mout_oe_ram_local !== new_Mout_oe_ram_local)$fatal(1,"OUTPUT_MISMATCH Mout_oe_ram_local cycle=%0d ref=%h new=%h",cycle,ref_Mout_oe_ram_local,new_Mout_oe_ram_local);
 if(ref_Mout_we_ram_local !== new_Mout_we_ram_local)$fatal(1,"OUTPUT_MISMATCH Mout_we_ram_local cycle=%0d ref=%h new=%h",cycle,ref_Mout_we_ram_local,new_Mout_we_ram_local);
 if(ref_Mout_addr_ram_local !== new_Mout_addr_ram_local)$fatal(1,"OUTPUT_MISMATCH Mout_addr_ram_local cycle=%0d ref=%h new=%h",cycle,ref_Mout_addr_ram_local,new_Mout_addr_ram_local);
 if(ref_Mout_Wdata_ram_local !== new_Mout_Wdata_ram_local)$fatal(1,"OUTPUT_MISMATCH Mout_Wdata_ram_local cycle=%0d ref=%h new=%h",cycle,ref_Mout_Wdata_ram_local,new_Mout_Wdata_ram_local);
 if(ref_Mout_data_ram_size_local !== new_Mout_data_ram_size_local)$fatal(1,"OUTPUT_MISMATCH Mout_data_ram_size_local cycle=%0d ref=%h new=%h",cycle,ref_Mout_data_ram_size_local,new_Mout_data_ram_size_local);
 if(ref_Mout_tag_local !== new_Mout_tag_local)$fatal(1,"OUTPUT_MISMATCH Mout_tag_local cycle=%0d ref=%h new=%h",cycle,ref_Mout_tag_local,new_Mout_tag_local);
 if(ref_M_tag !== new_M_tag)$fatal(1,"OUTPUT_MISMATCH M_tag cycle=%0d ref=%h new=%h",cycle,ref_M_tag,new_M_tag);
 if(ref_M_DataRdy !== new_M_DataRdy)$fatal(1,"OUTPUT_MISMATCH M_DataRdy cycle=%0d ref=%h new=%h",cycle,ref_M_DataRdy,new_M_DataRdy);
 if(ref_M_Rdata_ram !== new_M_Rdata_ram)$fatal(1,"OUTPUT_MISMATCH M_Rdata_ram cycle=%0d ref=%h new=%h",cycle,ref_M_Rdata_ram,new_M_Rdata_ram);
 if(ref_M_back_pressure_local !== new_M_back_pressure_local)$fatal(1,"OUTPUT_MISMATCH M_back_pressure_local cycle=%0d ref=%h new=%h",cycle,ref_M_back_pressure_local,new_M_back_pressure_local);

end
initial
begin
 repeat(3)@(negedge clock);
 reset=1;
 for(cycle=0;cycle<5000;cycle=cycle+1)
 begin
  @(negedge clock);
  random_reg={random_reg[30:0],random_reg[31]^random_reg[21]^random_reg[1]^random_reg[0]};
  Mout_back_pressure_memory=CS ? ((cycle%97<30) || random_reg[4]) : 0;
  Mout_addr_ram={random_reg[0],random_reg[30:0]};
  Mout_Wdata_ram=random_reg ^ cycle;
  Mout_data_ram_size=32;
  Mout_tag=cycle;
  Mout_oe_ram=0;Mout_we_ram=0;
  if(supplied-reference_taken<DEPTH && random_reg[1])
  begin
   Mout_oe_ram=random_reg[3];Mout_we_ram=!random_reg[3];
  end
  M_Rdata_ram_memory=random_reg;
  M_Rdata_ram_local=~random_reg;
  M_tag_memory=random_reg;
  M_tag_local=~random_reg;
  M_DataRdy_memory=random_reg[9];
  M_DataRdy_local=random_reg[13];
  if(cycle==2500)
  begin
   reset=0;Mout_oe_ram=0;Mout_we_ram=0;
  end
  else reset=1;
 end
 @(negedge clock);Mout_oe_ram=0;Mout_we_ram=0;Mout_back_pressure_memory=0;
 repeat(DEPTH+10)@(negedge clock);
 if(supplied!=reference_taken || supplied!=candidate_taken)$fatal(1,"UNDELIVERED_WORDS");
 if(read_requests<100 || write_requests<100)$fatal(1,"MISSING_RW_COVERAGE");
 if(FILTER && local_taken<100)$fatal(1,"MISSING_LOCAL_COVERAGE");
 $display("GMI_EQUIVALENCE_PASS C=%0d F=%0d CS=%0d read=%0d write=%0d max_occupancy=%0d",CONTEXT_BITS,FILTER,CS,read_requests,write_requests,max_occupancy);
 $finish;
end
endmodule
