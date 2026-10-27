typedef int unsigned ptr_t;

module if_utils #(parameter integer ID = 0, BITSIZE_data = 32) ();
    localparam integer WORDS = 4096;
    localparam integer MAX_CALLS = 4096;
    reg [BITSIZE_data-1:0] words [0:WORDS-1];
    reg [31:0] read_addr [0:MAX_CALLS-1];
    reg [31:0] write_addr [0:MAX_CALLS-1];
    time write_time [0:MAX_CALLS-1];
    reg [BITSIZE_data-1:0] write_data [0:MAX_CALLS-1];
    reg [((BITSIZE_data + 7) / 8)-1:0] write_mask [0:MAX_CALLS-1];
    integer read_calls = 0;
    integer write_calls = 0;
    integer i;

    initial
    begin
        for (i = 0; i < WORDS; i = i + 1) words[i] = '0;
    end

    function automatic integer word_index(input ptr_t addr);
        word_index = (addr >> 2) % WORDS;
    endfunction

    function automatic [BITSIZE_data-1:0] read_a(input ptr_t addr);
        integer slot;
        begin
            slot = word_index(addr);
            if (read_calls < MAX_CALLS) read_addr[read_calls] = addr;
            read_calls = read_calls + 1;
            read_a = words[slot];
        end
    endfunction

    function automatic integer write_strobe(
        input logic [BITSIZE_data-1:0] data,
        input logic [((BITSIZE_data + 7) / 8)-1:0] strobe,
        input ptr_t addr
    );
        integer slot;
        integer lane;
        begin
            slot = word_index(addr);
            if (write_calls < MAX_CALLS)
            begin
                write_addr[write_calls] = addr;
                write_time[write_calls] = $time;
                write_data[write_calls] = data;
                write_mask[write_calls] = strobe;
            end
            write_calls = write_calls + 1;
            for (lane = 0; lane < (BITSIZE_data + 7) / 8; lane = lane + 1)
                if (strobe[lane]) words[slot][lane*8 +: 8] = data[lane*8 +: 8];
            write_strobe = 0;
        end
    endfunction

    task automatic set_word(input ptr_t addr, input logic [BITSIZE_data-1:0] data);
        words[word_index(addr)] = data;
    endtask

    task automatic reset_counters;
        read_calls = 0;
        write_calls = 0;
    endtask
endmodule
