  std::cout << " // verilator lint_off LITENDIAN" << std::endl;
  std::cout << "parameter MAX_BUFF_SIZE = 256;" << std::endl;
  std::cout << "reg [0:8*MAX_BUFF_SIZE-1] buffer_name;" << std::endl;
  std::string clog2_function = "\n\
  `ifndef _SIM_HAVE_CLOG2\n\
    function integer log2;\n\
       input integer value;\n\
       integer temp_value;\n\
      begin\n\
        temp_value = value-1;\n\
        for (log2=0; temp_value>0; log2=log2+1)\n\
          temp_value = temp_value>>1;\n\
      end\n\
    endfunction\n\
  `endif";
  std::cout << clog2_function << std::endl;

  std::string nbit_buffer = "\n\
  `ifdef _SIM_HAVE_CLOG2\n\
    parameter nbits_buffer = $clog2(MAX_BUFF_SIZE);\n\
  `else\n\
    parameter nbits_buffer = log2(MAX_BUFF_SIZE);\n\
  `endif";
  std::cout << nbit_buffer << std::endl;

  std::string sensitivity;
  for(int i=0;i<_np;i++)
  {
    sensitivity += " or " + _p[i].name;
  }

  std::string modes = "in2";

  std::string flags_string = "(" + modes + " & "  + STR(O_RDWR) + ") != 0 && (" + modes + " & "  + STR(O_APPEND) + ") ? \"a+b\" : ((" + modes + " & "  + STR(O_RDWR) + ") != 0 ? \"r+b\" : ((" + modes + " & "  + STR(O_WRONLY) + ") != 0 && (" + modes + " & "  + STR(O_APPEND) + ") ? \"ab\" : (" + modes + " & "  + STR(O_WRONLY) + ") != 0 ? \"wb\" : \"rb\"" + "))";
  

  std::string fsm="\
  reg [nbits_buffer-1:0] _present_index 1INIT_ZERO_VALUE;\n\
  reg [nbits_buffer-1:0] _next_index;\n\
  reg [BITSIZE_Mout_addr_ram-1:0] _present_pointer 1INIT_ZERO_VALUE;\n\
  reg [BITSIZE_Mout_addr_ram-1:0] _next_pointer;\n\
  reg done_port;\n\
  wire mem_done_port;\n\
  reg signed [BITSIZE_out1-1:0] temp_out1;\n\
  \n\
  parameter [1:0] S_0 = 2'd0,\n\
                  S_1 = 2'd1,\n\
                  S_2 = 2'd2,\n\
                  S_3 = 2'd3;\n\
  reg [3:0] _present_state 1INIT_ZERO_VALUE;\n\
  reg [3:0] _next_state;\n\
  reg [63:0] data1;\n\
  reg [7:0] data1_size;\n\
  wire [" + data_bus_bitsize + "-1:0] mem_out1;\n\
  reg [" + addr_bus_bitsize + "-1:0] mem_in2;\n\
  reg [" + size_bus_bitsize + "-1:0] mem_in3;\n\
  reg mem_sel_LOAD;\n\
  mem_ctrl_kernel #(.BITSIZE_tag(" + tag_bus_bitsize + "), .TAG_MEM_REQ(" + tag_memory_ctrl + "), .BITSIZE_in1(" + data_bus_bitsize + "), .BITSIZE_in2(" + addr_bus_bitsize + "), .BITSIZE_in3(" + size_bus_bitsize + "), .BITSIZE_out1(" + data_bus_bitsize + "), .BITSIZE_Mout_oe_ram(BITSIZE_Mout_oe_ram), .BITSIZE_Mout_we_ram(BITSIZE_Mout_we_ram), .BITSIZE_Mout_addr_ram(BITSIZE_Mout_addr_ram), .BITSIZE_M_Rdata_ram(BITSIZE_M_Rdata_ram), .BITSIZE_Mout_Wdata_ram(BITSIZE_Mout_Wdata_ram), .BITSIZE_Mout_data_ram_size(BITSIZE_Mout_data_ram_size), .BITSIZE_M_DataRdy(BITSIZE_M_DataRdy)) mem_ctrl_kernel_instance (.in2(mem_in2), .in3(mem_in3), .M_Rdata_ram(M_Rdata_ram), .in1(0), .sel_LOAD(mem_sel_LOAD), .sel_STORE(1'b0), .M_DataRdy(M_DataRdy), .done(mem_done_port), .Mout_addr_ram(Mout_addr_ram), .out1(mem_out1), .Mout_data_ram_size(Mout_data_ram_size), .Mout_Wdata_ram(Mout_Wdata_ram), .Mout_oe_ram(Mout_oe_ram), .Mout_we_ram(Mout_we_ram), .Mout_tag_ram(Mout_tag_ram), .Min_tag(Min_tag), .request_accepted(request_accepted));\n\
  \n\
  \n\
  always @(posedge clock 1RESET_EDGE)\n\
    if (1RESET_VALUE)\n\
      begin\n\
        _present_state <= S_0;\n\
        _present_pointer <= {BITSIZE_Mout_addr_ram{1'b0}};\n\
        _present_index <= {nbits_buffer{1'b0}};\n\
      end\n\
    else\n\
      begin\n\
        _present_state <= _next_state;\n\
        _present_pointer <= _next_pointer;\n\
        _present_index <= _next_index;\n\
      end\n\
  \n\
  assign out1 = {1'b0,temp_out1[30:0]};\
  always @(_present_state or _present_pointer or _present_index or start_port or mem_done_port or " + sensitivity + " or mem_out1 or M_DataRdy)\n\
      begin\n\
        done_port = 1'b0;\n\
        _next_state = _present_state;\n\
        _next_pointer = _present_pointer;\n\
        _next_index = _present_index;\n\
        mem_sel_LOAD = 1'b0;\n\
        mem_in2=" + addr_bus_bitsize + "'d0;\n\
        mem_in3=" + size_bus_bitsize + "'d0;\n\
        case (_present_state)\n\
          S_0:\n\
            if(start_port)\n\
              begin\n\
                _next_pointer=0;\n\
                _next_index={nbits_buffer{1'b0}};\n\
                _next_state=S_1;  \n\
                buffer_name=0;  \n\
              end\n\
            \n\
         S_1:\n\
           begin\n\
             mem_in2 = in1[BITSIZE_Mout_addr_ram-1:0]+_present_pointer;\n\
             mem_in3 = {{BITSIZE_Mout_data_ram_size-4{1'b0}}, 4'd8};\n\
             mem_sel_LOAD=1'b1;\n\
             if(mem_done_port)\n\
             begin\n\
                buffer_name[_present_index*8 +:8] = mem_out1[7:0];\n\
                if(mem_out1[7:0] == 8'd0)\n\
                  _next_state=S_2;\n\
                else\n\
                  _next_state=S_3;\n\
             end\n\
           end\n\
         S_2:\n\
           begin\n\
// synthesis translate_off\n\
             temp_out1 = $fopen(buffer_name, "+ flags_string + ");\n\
// synthesis translate_on\n\
             done_port = 1'b1;\n\
             _next_state=S_0;\n\
           end\n\
         S_3:\n\
           begin\n\
             if(!mem_done_port)\n\
             begin\n\
              _next_pointer=_present_pointer+1'd1;\n\
              _next_index=_present_index+1'd1;\n\
              _next_state=S_1;\n\
             end\n\
           end\n\
      endcase\n \
  end\n";

std::cout << fsm;
std::cout << " // verilator lint_on LITENDIAN" << std::endl;
