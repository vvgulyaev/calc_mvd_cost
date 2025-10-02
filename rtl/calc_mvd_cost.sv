
module calc_mvd_cost (
    input                       ap_clk,
    input                       ap_rst,
    input                       ap_start,
    output logic                ap_done,
    output logic                ap_ready,
    output logic                ap_idle,
    input        [31:0]         x,
    input        [31:0]         y,
    input        [31:0]         mv_shift,
    input        [15:0]         mv_cand_0,
    input        [15:0]         mv_cand_1,
    input        [15:0]         mv_cand_2,
    input        [15:0]         mv_cand_3,
    output logic [63:0]         bitcost,
    output logic                bitcost_ap_vld,
    input        [63:0]         lambda_sqrt_integer_int64,
    input        [63:0]         lambda_sqrt_decimal_int64,
    output logic [63:0]         mvd_cost_int64,
    output logic                mvd_cost_int64_ap_vld
);

    // Constants
    localparam LAMBDA_DECIMAL_SHIFT_BITS = 14;
    localparam CTX_FRAC_BITS = 15;

    // Pipeline registers
    logic [31:0] x_shifted, y_shifted;
    logic [63:0] lambda_sqrt_integer_reg, lambda_sqrt_decimal_reg;
    logic [15:0] mv_cand_reg[4];
    logic [2:0] pipeline_valid;

    // Internal signals for select_mv_cand logic
    logic same_cand;
    logic [63:0] cand1_cost, cand2_cost;
    logic [63:0] selected_cost;

    always_ff @(posedge ap_clk or posedge ap_rst) begin
        if (ap_rst) begin
            //stage 1
            x_shifted <= 32'b0;
            y_shifted <= 32'b0;
            lambda_sqrt_integer_reg <= 64'b0;
            lambda_sqrt_decimal_reg <= 64'b0;
            mv_cand_reg <= '{default: 16'b0};
            pipeline_valid[0] <= 1'b0;

            //stage 2
            same_cand <= 1'b0;
            cand1_cost <= 64'b0;
            cand2_cost <= 64'b0;
            pipeline_valid[1] <= 1'b0;

            //stage 3
            bitcost <= 64'b0;
            bitcost_ap_vld <= 1'b0;
            mvd_cost_int64 <= 64'b0;
            mvd_cost_int64_ap_vld <= 1'b0;
            pipeline_valid[2] <= 1'b0;
        end
        else begin
            // Pipeline stage 1: Input processing and shifting
            if (ap_start) begin
                x_shifted <= x << mv_shift;
                y_shifted <= y << mv_shift;
                lambda_sqrt_integer_reg <= lambda_sqrt_integer_int64;
                lambda_sqrt_decimal_reg <= lambda_sqrt_decimal_int64;
                mv_cand_reg[0] <= mv_cand_0;
                mv_cand_reg[1] <= mv_cand_1;
                mv_cand_reg[2] <= mv_cand_2;
                mv_cand_reg[3] <= mv_cand_3;
                pipeline_valid[0] <= 1'b1;
            end else begin
                pipeline_valid[0] <= 1'b0;
            end

            // Pipeline stage 2: Calculate MVD costs
            pipeline_valid[1] <= pipeline_valid[0];
            if (pipeline_valid[0]) begin
                // Check if candidates are the same
                same_cand <= (mv_cand_reg[0] == mv_cand_reg[2]) && (mv_cand_reg[1] == mv_cand_reg[3]);

                // Calculate cost for first candidate
                cand1_cost <= get_mvd_coding_cost(
                    $signed(x_shifted) - $signed(mv_cand_reg[0]),
                    $signed(y_shifted) - $signed(mv_cand_reg[1])
                );

                // Calculate cost for second candidate
                cand2_cost <= get_mvd_coding_cost(
                    $signed(x_shifted) - $signed(mv_cand_reg[2]),
                    $signed(y_shifted) - $signed(mv_cand_reg[3])
                );
            end

            // Pipeline stage 3: Select minimum cost and calculate final result
            pipeline_valid[2] <= pipeline_valid[1];
            bitcost_ap_vld <= pipeline_valid[1];
            mvd_cost_int64_ap_vld <= pipeline_valid[1];

            if (pipeline_valid[1]) begin
                // Select minimum cost
                if (same_cand) begin
                    selected_cost = cand1_cost;
                end else begin
                    selected_cost = (cand2_cost < cand1_cost) ? cand2_cost : cand1_cost;
                end

                bitcost <= selected_cost;

                // Calculate final mvd_cost_int64
                mvd_cost_int64 <= selected_cost * lambda_sqrt_integer_reg +
                           ((selected_cost * lambda_sqrt_decimal_reg) >> LAMBDA_DECIMAL_SHIFT_BITS);
            end
        end
    end

    assign ap_ready = 1'b1;
    assign ap_done  = mvd_cost_int64_ap_vld;
    assign ap_idle  = 0;

    // Function to calculate MVD coding cost (equivalent to get_mvd_coding_cost_hls_inline)
    function automatic logic [63:0] get_mvd_coding_cost(input logic signed [31:0] mvd_hor, input logic signed [31:0] mvd_ver);
        logic [63:0] bitcost_temp;
        logic [63:0] x_abs, y_abs;
        logic [63:0] Q;
        logic [63:0] a, b;

        bitcost_temp = 64'd4 << CTX_FRAC_BITS;
        x_abs = (mvd_hor < 0) ? -mvd_hor : mvd_hor;
        y_abs = (mvd_ver < 0) ? -mvd_ver : mvd_ver;
        Q = 64'd1 << CTX_FRAC_BITS;

        bitcost_temp += (x_abs == 1) ? Q : 64'b0;
        bitcost_temp += (y_abs == 1) ? Q : 64'b0;

        a = get_ep_ex_golomb_bitcost_withpow(x_abs);
        b = get_ep_ex_golomb_bitcost_withpow(y_abs);

        return ((a + b + bitcost_temp) >> CTX_FRAC_BITS) << 34;
    endfunction

    // Function equivalent to get_ep_ex_golomb_bitcost_inline_withpow
    function automatic logic [63:0] get_ep_ex_golomb_bitcost_withpow(input logic [63:0] symbol);
        if (symbol >= 32768)
            return 64'd983040;
        else if (symbol >= 16384)
            return 64'd917504;
        else if (symbol >= 8192)
            return 64'd851968;
        else if (symbol >= 4096)
            return 64'd786432;
        else if (symbol >= 2048)
            return 64'd720896;
        else if (symbol >= 1024)
            return 64'd655360;
        else if (symbol >= 512)
            return 64'd589824;
        else if (symbol >= 256)
            return 64'd524288;
        else if (symbol >= 128)
            return 64'd458752;
        else if (symbol >= 64)
            return 64'd393216;
        else if (symbol >= 32)
            return 64'd327680;
        else if (symbol >= 16)
            return 64'd262144;
        else if (symbol >= 8)
            return 64'd196608;
        else if (symbol >= 4)
            return 64'd131072;
        else if (symbol >= 2)
            return 64'd65536;
        else
            return 64'd0;
    endfunction

endmodule
