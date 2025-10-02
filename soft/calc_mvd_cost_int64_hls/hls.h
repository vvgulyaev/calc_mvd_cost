void calc_mvd_cost_int64_hls(
  HLS_COMMON_ARG
  int x,
  int y,
  int mv_shift,
  int16_t mv_cand[4],
  int64_t* bitcost,
  int64_t lambda_sqrt_integer_int64,
  int64_t lambda_sqrt_decimal_int64,
  int64_t *mvd_cost_int64
);