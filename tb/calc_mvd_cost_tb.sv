`timescale 1ns / 1ps

module calc_mvd_cost_tb;

    // Clock and reset
    logic clk;
    logic rst_n;

    // DUT signals
    logic start;
    logic [31:0] x;
    logic [31:0] y;
    logic [31:0] mv_shift;
    logic [15:0] mv_cand[4];
    logic [63:0] bitcost;
    logic bitcost_vld;
    logic [63:0] lambda_sqrt_integer;
    logic [63:0] lambda_sqrt_decimal;
    logic [63:0] mvd_cost;
    logic mvd_cost_vld;

    // Test data variables
    logic [63:0] expected_bitcost;
    logic [63:0] expected_mvd_cost;

    // File handling
    integer file_handle;
    integer test_count;
    logic test_done;

    // Test control
    logic [2:0] test_state;
    localparam IDLE = 3'b000;
    localparam LOAD_DATA = 3'b001;
    localparam START_TEST = 3'b010;
    localparam WAIT_RESULT = 3'b011;
    localparam CHECK_RESULT = 3'b100;
    localparam NEXT_TEST = 3'b101;
    localparam FINISH = 3'b110;

    // Clock generation
    initial begin
        clk = 0;
        forever #5 clk = ~clk; // 100MHz clock
    end

    // DUT instantiation
    calc_mvd_cost dut (
        .ap_clk                     (clk),
        .ap_rst                     (~rst_n),
        .ap_start                   (start),
        .x                          (x),
        .y                          (y),
        .mv_shift                   (mv_shift),
        .mv_cand_0                  (mv_cand[0]),
        .mv_cand_1                  (mv_cand[1]),
        .mv_cand_2                  (mv_cand[2]),
        .mv_cand_3                  (mv_cand[3]),
        .bitcost                    (bitcost),
        .bitcost_ap_vld             (bitcost_vld),
        .lambda_sqrt_integer_int64  (lambda_sqrt_integer),
        .lambda_sqrt_decimal_int64  (lambda_sqrt_decimal),
        .mvd_cost_int64             (mvd_cost),
        .mvd_cost_int64_ap_vld      (mvd_cost_vld)
    );

    // Test sequence
    initial begin
        // Initialize signals
        rst_n = 0;
        start = 0;
        x = 0;
        y = 0;
        mv_shift = 0;
        mv_cand = '{default: 16'b0};
        lambda_sqrt_integer = 0;
        lambda_sqrt_decimal = 0;
        test_count = 0;
        test_done = 0;
        test_state = IDLE;

        // Reset sequence
        #20;
        rst_n = 1;
        #10;

        $display("Test calc_mvd_cost");

        // Open test data file
        file_handle = $fopen("calc_mvd_cost_test_data.bin", "rb");
        if (file_handle == 0) begin
            $error("Failed to open test data file: calc_mvd_cost_test_data.bin");
            $finish;
        end

        // Main test loop
        while (!test_done) begin
            @(posedge clk);

            case (test_state)
                IDLE: begin
                    test_state <= LOAD_DATA;
                end

                LOAD_DATA: begin
                    if (load_test_data()) begin
                        test_state <= START_TEST;
                        $display("==== test_id: %d ====", test_count);
                        $display("x:%X", x);
                        $display("y:%X", y);
                        $display("mv_shift:%X", mv_shift);
                        $display("mv_cand:%X,%X,%X,%X", mv_cand[0], mv_cand[1], mv_cand[2], mv_cand[3]);
                        $display("bitcost:%X", expected_bitcost);
                        $display("lambda_sqrt_integer:%X", lambda_sqrt_integer);
                        $display("lambda_sqrt_decimal:%X", lambda_sqrt_decimal);
                        $display("mvd_cost:%X", expected_mvd_cost);
                    end else begin
                        test_state <= FINISH;
                    end
                end

                START_TEST: begin
                    start <= 1;
                    test_state <= WAIT_RESULT;
                end

                WAIT_RESULT: begin
                    start <= 0;
                    if (bitcost_vld && mvd_cost_vld) begin
                        test_state <= CHECK_RESULT;
                    end
                end

                CHECK_RESULT: begin
                    $display("bitcost result:%X", bitcost);
                    $display("mvd_cost result:%X", mvd_cost);

                    // Check results
                    if (bitcost !== expected_bitcost) begin
                        $error("Test %d FAILED: bitcost mismatch. Expected: %X, Got: %X",
                               test_count, expected_bitcost, bitcost);
                        $fclose(file_handle);
                        $finish;
                    end

                    if (mvd_cost !== expected_mvd_cost) begin
                        $error("Test %d FAILED: mvd_cost mismatch. Expected: %X, Got: %X",
                               test_count, expected_mvd_cost, mvd_cost);
                        $fclose(file_handle);
                        $finish;
                    end

                    test_count++;
                    test_state <= NEXT_TEST;
                end

                NEXT_TEST: begin
                    test_state <= LOAD_DATA;
                end

                FINISH: begin
                    $display("Passed after %d tests", test_count);
                    $fclose(file_handle);
                    test_done = 1;
                    $finish;
                end

                default: begin
                    test_state <= IDLE;
                end
            endcase
        end
    end

    function logic [15:0] change_byte_endian16(input [15:0] val);
        localparam int TOP_BYTE_IDX = 1;
        change_byte_endian16 = 0;
        for (int b=0; b<=TOP_BYTE_IDX; b++) begin
            change_byte_endian16[(TOP_BYTE_IDX-b)*8 +: 8] = val[b*8 +: 8];
        end
    endfunction

    function logic [31:0] change_byte_endian32(input [31:0] val);
        localparam int TOP_BYTE_IDX = 3;
        change_byte_endian32 = 0;
        for (int b=0; b<=TOP_BYTE_IDX; b++) begin
            change_byte_endian32[(TOP_BYTE_IDX-b)*8 +: 8] = val[b*8 +: 8];
        end
    endfunction

    function logic [63:0] change_byte_endian64(input [63:0] val);
        localparam int TOP_BYTE_IDX = 7;
        change_byte_endian64 = 0;
        for (int b=0; b<=TOP_BYTE_IDX; b++) begin
            change_byte_endian64[(TOP_BYTE_IDX-b)*8 +: 8] = val[b*8 +: 8];
        end
    endfunction

    // Function to load test data from binary file
    function automatic logic load_test_data();
        integer bytes_read;
        logic [31:0] temp_32;
        logic [63:0] temp_64;
        logic [15:0] temp_16;

        // Read x (4 bytes)
        bytes_read = $fread(temp_32, file_handle);
        if (bytes_read != 4) return 0;
        x = change_byte_endian32(temp_32);

        // Read y (4 bytes)
        bytes_read = $fread(temp_32, file_handle);
        if (bytes_read != 4) return 0;
        y = change_byte_endian32(temp_32);

        // Read mv_shift (4 bytes, but only use lower 8 bits)
        bytes_read = $fread(temp_32, file_handle);
        if (bytes_read != 4) return 0;
        mv_shift = change_byte_endian32(temp_32);

        // Read mv_cand array (8 bytes = 4 * 2 bytes)
        for (int i = 0; i < 4; i++) begin
            bytes_read = $fread(temp_16, file_handle);
            if (bytes_read != 2) return 0;
            mv_cand[i] = change_byte_endian16(temp_16);
        end

        // Read lambda_sqrt_integer (8 bytes)
        bytes_read = $fread(temp_64, file_handle);
        if (bytes_read != 8) return 0;
        lambda_sqrt_integer = change_byte_endian64(temp_64);

        // Read lambda_sqrt_decimal (8 bytes)
        bytes_read = $fread(temp_64, file_handle);
        if (bytes_read != 8) return 0;
        lambda_sqrt_decimal = change_byte_endian64(temp_64);

        // Read expected bitcost (8 bytes)
        bytes_read = $fread(temp_64, file_handle);
        if (bytes_read != 8) return 0;
        expected_bitcost = change_byte_endian64(temp_64);

        // Read expected mvd_cost (8 bytes)
        bytes_read = $fread(temp_64, file_handle);
        if (bytes_read != 8) return 0;
        expected_mvd_cost = change_byte_endian64(temp_64);

        return 1; // Successfully loaded data
    endfunction

    // Timeout watchdog
    initial begin
        #1000000; // 1ms timeout
        $error("Test timeout!");
        $finish;
    end

    // Optional: Dump waveforms
    initial begin
        $dumpfile("calc_mvd_cost_tb.vcd");
        $dumpvars(0, calc_mvd_cost_tb);
    end

endmodule