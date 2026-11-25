module tb_std_fifo;
    parameter DEPTH=64;
    parameter DATA_BITS=32;
    localparam COUNT_BITS=$clog2(DEPTH+1);
    reg clock=0;
    reg reset=0;
    reg clear=0;
    reg push=0;
    reg pop=0;
    reg [DATA_BITS-1:0] data_in=0;
    wire [DATA_BITS-1:0] data_out;
    wire [COUNT_BITS-1:0] level;
    wire empty,full;
    reg [DATA_BITS-1:0] reference_reg [0:DEPTH-1];
    reg [31:0] random_reg=32'hf7c00abc;
    integer expected_count=0;
    integer cycle,index;
    integer pushes=0,pops=0,full_replace=0,empty_bypass=0,clears=0;
    integer old_count;
    always #5 clock=~clock;
    STD_FIFO #(.n_elements(DEPTH), .BITSIZE_data_in(DATA_BITS),
        .BITSIZE_data_out(DATA_BITS), .BITSIZE_level(COUNT_BITS)) dut(
        .clock(clock),.reset(reset),.clear(clear),.push(push),.pop(pop),
        .data_in(data_in),.data_out(data_out),.level(level),.empty(empty),.full(full));
    initial
    begin
        repeat(3)@(negedge clock);
        reset=1;
        for(cycle=0;cycle<3*DEPTH+2016;cycle=cycle+1)
        begin
            @(negedge clock);
            random_reg={random_reg[30:0],random_reg[31]^random_reg[21]^random_reg[1]^random_reg[0]};
            data_in={random_reg,~random_reg,random_reg ^ 32'hc0ffee};
            clear=0;
            if(cycle<DEPTH)
            begin
                push=1;pop=0;
            end
            else if(cycle<2*DEPTH)
            begin
                push=1;pop=1;
            end
            else if(cycle<3*DEPTH)
            begin
                push=0;pop=1;
            end
            else if(cycle<3*DEPTH+16)
            begin
                push=1;pop=1;
            end
            else
            begin
                pop=random_reg[1] && expected_count>0;
                push=random_reg[2] && (expected_count<DEPTH || pop);
                if(expected_count==0 && push)pop=random_reg[3];
                clear=(cycle%117)==0;
            end
            if(expected_count>0 && data_out !== reference_reg[0])$fatal(1,"HEAD_BEFORE_POP");
            @(posedge clock);
            old_count=expected_count;
            if(clear)
            begin
                expected_count=0;clears=clears+1;
            end
            else
            begin
                if(pop && old_count>0)
                begin
                    for(index=0;index<expected_count-1;index=index+1)reference_reg[index]=reference_reg[index+1];
                    expected_count=expected_count-1;pops=pops+1;
                end
                if(push && !(pop && old_count==0))
                begin
                    reference_reg[expected_count]=data_in;
                    expected_count=expected_count+1;pushes=pushes+1;
                end
                if(push && pop && old_count==0)empty_bypass=empty_bypass+1;
                if(push && pop && old_count==DEPTH)full_replace=full_replace+1;
            end
            #1;
            if(level!==expected_count || empty!==(expected_count==0) || full!==(expected_count==DEPTH))$fatal(1,"LEVEL_FLAGS_MISMATCH");
            if(expected_count>0 && data_out !== reference_reg[0])$fatal(1,"HEAD_AFTER_EDGE");
        end
        if(full_replace<DEPTH || empty_bypass<16 || clears==0)$fatal(1,"MISSING_COVERAGE");
        $display("COMPLETE_FIFO_PASS D=%0d W=%0d push=%0d pop=%0d full_replace=%0d bypass=%0d",DEPTH,DATA_BITS,pushes,pops,full_replace,empty_bypass);
        $finish;
    end
endmodule
