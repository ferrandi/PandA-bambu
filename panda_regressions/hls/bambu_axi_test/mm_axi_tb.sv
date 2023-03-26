`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 03/20/2023 10:40:46 AM
// Design Name: 
// Module Name: mmult_tb
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////
`define NULL 0
`define EOF -1

import axi_vip_pkg::*;
import VIP_axi_vip_0_0_pkg::*;

module mmult_tb(

    );
    logic clk;
    logic rst;
    reg [31:0] a;
    reg [31:0] b;
    reg [31:0] c;
    logic start;
    logic done;
    
    integer file;
    integer _ch_ = 0, _r_;
    logic [31:0] base_addr;
    logic [7:0] databyte;
    logic [31:0] mem_rd_byte;
    logic [31:0] mem_wr_byte;
    logic [3:0] wstrb;
    logic [31:0] byte_cnt = 0;
    logic [8000:0] line; // Comment line read from file
    logic success = 1;

    bit [31:0] mem_fill_payload;
    
    VIP_axi_vip_0_0_slv_mem_t                                  mem_agent;
    
    initial begin 
        clk <= 1'b1;
        rst = 1'b0;
        
        repeat(1) @(posedge clk);
        rst = 1'b1;
        repeat(5) @(posedge clk);
        
        slv_start_stimulus();
        user_gen_arready();
        user_gen_awready();
        user_gen_wready();
        
        // OPEN FILE WITH VALUES FOR SIMULATION
        file = $fopen("/data2/home/barone/panda/PandA-bambu/panda_regressions/hls/bambu_axi_test/values.txt","r");
        // Error in file open
        if (file == `NULL)
        begin
          $display("ERROR - Error opening the file");
          $finish;// Terminate
        end
        _ch_ = $fgetc(file);
        if ($feof(file))
        begin
          $fclose(file);
          $finish;
        end
        while (_ch_ == "/" || _ch_ == "\n" || _ch_ == "b")
        begin
          if (_ch_ == "b")
          begin
            _r_ = $fscanf(file,"%b\n", base_addr); end
          else
          begin
            _r_ = $fgets(line, file);
          end
          _ch_ = $fgetc(file);
        end
        // initializing memory --------------------------------------------------------------
        while (_ch_ == "/" || _ch_ == "\n" || _ch_ == "m")
        begin
          if (_ch_ == "m")
          begin
            _r_ = $fscanf(file,"%b\n", databyte);
            wstrb = 1 << (byte_cnt % 4);
            mem_wr_byte = databyte << (byte_cnt % 4 * 8);
            backdoor_mem_write(base_addr + byte_cnt, mem_wr_byte, wstrb);
            byte_cnt = byte_cnt + 1;
          end
          else
          begin
            _r_ = $fgets(line, file);
          end
          _ch_ = $fgetc(file);
        end
        // Read address values --------------------------------------------------------------
        while (_ch_ == "/" || _ch_ == "\n")
        begin
          _r_ = $fgets(line, file);
          _ch_ = $fgetc(file);
        end
        // If no character found
        if (_ch_ == -1)
        begin
          $fclose(file);
          $finish;
        end
        if (_ch_ == "p")
        begin
          _r_ = $fscanf(file,"%b\n", a); // expected format: bbb...b (example: 00101110)
        end
        if (_r_ != 1) // error
        begin
          _ch_ = $fgetc(file);
          if (_ch_ == `EOF) // end-of-file reached
          begin
            $display("ERROR - End of file reached before getting all the values for the parameters");
            $fclose(file);
            $finish;
          end
          else // generic error
          begin
            $display("ERROR - Unknown error while reading the file. Character found: %c", _ch_[7:0]);
            $fclose(file);
            $finish;
          end
        end
        else
        begin
          $display("Value found for input a: %b", a);
        end
        _ch_ = $fgetc(file);
        while (_ch_ == "/" || _ch_ == "\n")
        begin
          _r_ = $fgets(line, file);
          _ch_ = $fgetc(file);
        end
        // If no character found
        if (_ch_ == -1)
        begin
          $fclose(file);
          $finish;
        end
        if (_ch_ == "p")
        begin
          _r_ = $fscanf(file,"%b\n", b); // expected format: bbb...b (example: 00101110)
        end
        if (_r_ != 1) // error
        begin
          _ch_ = $fgetc(file);
          if (_ch_ == `EOF) // end-of-file reached
          begin
            $display("ERROR - End of file reached before getting all the values for the parameters");
            $fclose(file);
            $finish;
          end
          else // generic error
          begin
            $display("ERROR - Unknown error while reading the file. Character found: %c", _ch_[7:0]);
            $fclose(file);
            $finish;
          end
        end
        else
        begin
          $display("Value found for input b: %b", b);
        end
        _ch_ = $fgetc(file);
        while (_ch_ == "/" || _ch_ == "\n")
        begin
          _r_ = $fgets(line, file);
          _ch_ = $fgetc(file);
        end
        // If no character found
        if (_ch_ == -1)
        begin
          $fclose(file);
          $finish;
        end
        if (_ch_ == "p")
        begin
          _r_ = $fscanf(file,"%b\n", c); // expected format: bbb...b (example: 00101110)
        end
        if (_r_ != 1) // error
        begin
          _ch_ = $fgetc(file);
          if (_ch_ == `EOF) // end-of-file reached
          begin
            $display("ERROR - End of file reached before getting all the values for the parameters");
            $fclose(file);
            $finish;
          end
          else // generic error
          begin
            $display("ERROR - Unknown error while reading the file. Character found: %c", _ch_[7:0]);
            $fclose(file);
            $finish;
          end
        end
        else
        begin
          $display("Value found for input c: %b", c);
        end
                
        start = 1'b1;
        @(posedge clk);
        start = 1'b0;
        
        while(!done) begin
            @(posedge clk);
        end
        
        //Read results  --------------------------------------------------------------
        _ch_ = $fgetc(file);
        byte_cnt = 0;
        while (_ch_ == "/" || _ch_ == "\n" || _ch_ == "o")
        begin
          if (_ch_ == "o")
          begin
            _r_ = $fscanf(file,"%b\n", databyte); // expected format: bbb...b (example: 00101110)
            if (_r_ != 1)
            begin
              // error
              $display("ERROR - Unknown error while reading the file. Character found: %c", _ch_[7:0]);
              $fclose(file);
              $finish;
            end
            if(byte_cnt % 4 == 0) begin
                backdoor_mem_read(a + byte_cnt, mem_rd_byte);
            end else begin
                mem_rd_byte = mem_rd_byte >> 8;
            end
            $display("comparison = %d memory[%d] = %d  expected = %d \n", mem_rd_byte[7:0] == databyte, a + byte_cnt, mem_rd_byte[7:0], databyte);
            if (mem_rd_byte[7:0] !== databyte)
            begin
              success = 0;
            end
            byte_cnt = byte_cnt + 1;
            _ch_ = $fgetc(file);
          end
          else
          begin
            // skip comments and empty lines
            _r_ = $fgets(line, file);
            _ch_ = $fgetc(file);
          end
        end
        if (_ch_ == "e")
        begin
          _r_ = $fgets(line, file);
          _ch_ = $fgetc(file);
        end
        else
        begin
        // error
          $display("ERROR - Unknown error while reading the file. Character found: %c", _ch_[7:0]);
          $fclose(file);
          $finish;
        end
        _ch_ = $fgetc(file);
        byte_cnt = 0;
        while (_ch_ == "/" || _ch_ == "\n" || _ch_ == "o")
        begin
          if (_ch_ == "o")
          begin
            _r_ = $fscanf(file,"%b\n", databyte); // expected format: bbb...b (example: 00101110)
            if (_r_ != 1)
            begin
              // error
              $display("ERROR - Unknown error while reading the file. Character found: %c", _ch_[7:0]);
              $fclose(file);
              $finish;
            end
            if(byte_cnt % 4 == 0) begin
                backdoor_mem_read(b + byte_cnt, mem_rd_byte);
            end else begin
                mem_rd_byte = mem_rd_byte >> 8;
            end

            $display("comparison = %d memory[%d] = %d  expected = %d \n", mem_rd_byte[7:0] == databyte, b + byte_cnt, mem_rd_byte[7:0], databyte);
            if (mem_rd_byte[7:0] !== databyte)            begin
              success = 0;
            end
            byte_cnt = byte_cnt + 1;
            _ch_ = $fgetc(file);
          end
          else
          begin
            // skip comments and empty lines
            _r_ = $fgets(line, file);
            _ch_ = $fgetc(file);
          end
        end
        if (_ch_ == "e")
        begin
          _r_ = $fgets(line, file);
          _ch_ = $fgetc(file);
        end
        else
        begin
        // error
          $display("ERROR - Unknown error while reading the file. Character found: %c", _ch_[7:0]);
          $fclose(file);
          $finish;
        end
        _ch_ = $fgetc(file);
        byte_cnt = 0;
        while (_ch_ == "/" || _ch_ == "\n" || _ch_ == "o")
        begin
          if (_ch_ == "o")
          begin
            _r_ = $fscanf(file,"%b\n", databyte); // expected format: bbb...b (example: 00101110)
            if (_r_ != 1)
            begin
              // error
              $display("ERROR - Unknown error while reading the file. Character found: %c", _ch_[7:0]);
              $fclose(file);
              $finish;
            end
            if(byte_cnt % 4 == 0) begin
                backdoor_mem_read(c + byte_cnt, mem_rd_byte);
            end else begin
                mem_rd_byte = mem_rd_byte >> 8;
            end
            $display("comparison = %d memory[%d] = %d  expected = %d \n", mem_rd_byte[7:0] == databyte, c + byte_cnt, mem_rd_byte[7:0], databyte);
            if (mem_rd_byte[7:0] !== databyte)
            begin
              success = 0;
            end
            byte_cnt = byte_cnt + 1;
            _ch_ = $fgetc(file);
          end
          else
          begin
            // skip comments and empty lines
            _r_ = $fgets(line, file);
            _ch_ = $fgetc(file);
          end
        end
        if (_ch_ == "e")
        begin
          _r_ = $fgets(line, file);
          _ch_ = $fgetc(file);
        end
        else
        begin
        // error
          $display("ERROR - Unknown error while reading the file. Character found: %c", _ch_[7:0]);
          $fclose(file);
          $finish;
        end
        
        
        $fclose(file);
        
        if(success == 0) begin
            $display("Simulation not correct");
            $fatal;
        end
        
        $display("Simulation ended correctly");
        $finish;    
    end
    
    always #5 begin
        clk = ~clk;
    end
    
    VIP mmult(
        .clk(clk),
        .rst(rst),
        .a(a),
        .b(b),
        .c(c),
        .start(start),
        .done(done)
    );
    
  task slv_start_stimulus();
    mem_agent = new("slave vip agent with memory model",mmult.axi_vip_0.inst.IF);
    mem_agent.set_agent_tag("My VIP");
    mem_agent.set_verbosity(0);
    mem_agent.start_slave();                                     //Agent starts to run
    
    mem_fill_payload = 'hFFFFFFFF;
    set_mem_default_value_fixed(mem_fill_payload); // Call task to do fill in memory with default fixed value

  endtask
  
  task user_gen_awready();
    axi_ready_gen                           awready_gen;
    awready_gen = mem_agent.wr_driver.create_ready("awready");
    awready_gen.set_ready_policy(XIL_AXI_READY_GEN_NO_BACKPRESSURE);
    mem_agent.wr_driver.send_awready(awready_gen);
  endtask : user_gen_awready
  
  task user_gen_wready();
    axi_ready_gen                           wready_gen;
    wready_gen = mem_agent.wr_driver.create_ready("wready");
    wready_gen.set_ready_policy(XIL_AXI_READY_GEN_NO_BACKPRESSURE);
    mem_agent.wr_driver.send_wready(wready_gen);
  endtask : user_gen_wready
  
  task user_gen_arready();
    axi_ready_gen                           arready_gen;
    arready_gen = mem_agent.wr_driver.create_ready("arready");
    arready_gen.set_ready_policy(XIL_AXI_READY_GEN_NO_BACKPRESSURE);
    mem_agent.rd_driver.send_arready(arready_gen);
  endtask : user_gen_arready
  
  task backdoor_mem_write(
    input xil_axi_ulong                         addr, 
    input bit [32-1:0]           wr_data,
    input bit [(32/8)-1:0]       wr_strb ={(32/8){1'b1}}
  );
    mem_agent.mem_model.backdoor_memory_write(addr, wr_data, wr_strb);

  endtask

  /*************************************************************************************************
  * Task backdoor_mem_read shows how user can do backdoor read data from memory model
  * it has one input which is backdoor memory read address and one output which is read out data
  * User has to note to declare memory read address and data like below
  * input xil_axi_ulong mem_rd_addr,
  * output bit [DATA_WIDTH-1:0] mem_rd_data 
  *************************************************************************************************/
  task backdoor_mem_read(
    input xil_axi_ulong mem_rd_addr,
    output bit [32-1:0] mem_rd_data
   );
    mem_rd_data= mem_agent.mem_model.backdoor_memory_read(mem_rd_addr);

  endtask
  
  task set_mem_default_value_fixed(input bit [32-1:0] fill_payload);
    mem_agent.mem_model.set_memory_fill_policy(XIL_AXI_MEMORY_FILL_FIXED);
    mem_agent.mem_model.set_default_memory_value(fill_payload);
  endtask

endmodule
