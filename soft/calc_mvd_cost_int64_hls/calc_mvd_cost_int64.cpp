#include <stdint.h>
#include <math.h>
#include "common_with_hls.h"


#define LAMBDA_DECIMAL_SHIFT_BITS           14

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif






unsigned get_ep_ex_golomb_bitcost_inline(unsigned symbol)
{
	unsigned bins = 0;
#pragma HLS INLINE
	if (symbol >= 32768)
	{
		bins = 30;
	}
	else if (symbol >= 16384)
	{
		bins = 28;
	}
	else if (symbol >= 8192)
	{
		bins = 26;
	}
	else if (symbol >= 4096)
	{
		bins = 24;
	}
	else if (symbol >= 2048)
	{
		bins = 22;
	}
	else if (symbol >= 1024)
	{
		bins = 20;
	}
	else if (symbol >= 512)
	{
		bins = 18;
	}
	else if (symbol >= 256)
	{
		bins = 16;
	}
	else if (symbol >= 128)
	{
		bins = 14;
	}
	else if (symbol >= 64)
	{
		bins = 12;
	}
	else if (symbol >= 32)
	{
		bins = 10;
	}
	else if (symbol >= 16)
	{
		bins = 8;
	}
	else if (symbol >= 8)
	{
		bins = 6;
	}
	else if (symbol >= 4)
	{
		bins = 4;
	}
	else if (symbol >= 2)
	{
		bins = 2;
	}
	else if (symbol >= 1)
	{
		bins = 0;
	}

	return bins;
}


int get_ep_ex_golomb_bitcost_inline_withpow(int64_t symbol)
{
#pragma HLS INLINE
	if (symbol >= 32768)
		return 983040;
	else if (symbol >= 16384)
		return 917504;
	else if (symbol >= 8192)
		return 851968;
	else if (symbol >= 4096)
		return 786432;
	else if (symbol >= 2048)
		return 720896;
	else if (symbol >= 1024)
		return 655360;
	else if (symbol >= 512)
		return 589824;
	else if (symbol >= 256)
		return 524288;
	else if (symbol >= 128)
		return 458752;
	else if (symbol >= 64)
		return 393216;
	else if (symbol >= 32)
		return 327680;
	else if (symbol >= 16)
		return 262144;
	else if (symbol >= 8)
		return 196608;
	else if (symbol >= 4)
		return 131072;
	else if (symbol >= 2)
		return 65536;
	else
		return 0; // if (symbol >= 1)
}




/*
 * \@li latency 5, Interval 1, Pipeliined yes, DSP 0, FF 66 LUT 1013, BRAM 0, URAM 0
 * COSIM: II: avg 1, max 2, min 1. Latency: avg 1, max 1, min 1.
 */
int64_t get_mvd_coding_cost_hls_inline(int32_t mvd_hor, int32_t mvd_ver)
{
#pragma HLS INLINE

constexpr int CTX_FRAC_BITS = 15;

#if 0
	int bitcost = 4 << CTX_FRAC_BITS;
	int x = abs(mvd_hor);
	int y = abs(mvd_ver);
	int Q = 1 << CTX_FRAC_BITS;
	bitcost += x == 1 ? Q : (0);
	bitcost += y == 1 ? Q : (0);

	bitcost += get_ep_ex_golomb_bitcost_inline(x) << CTX_FRAC_BITS;
	bitcost += get_ep_ex_golomb_bitcost_inline(y) << CTX_FRAC_BITS;

	return (int64_t)(bitcost >> CTX_FRAC_BITS) * ENTROPY_BITS_WEIGHT;
#else
	int64_t bitcost = 4 << CTX_FRAC_BITS;
	int64_t x = abs(mvd_hor);
	int64_t y = abs(mvd_ver);
	int64_t Q = 1 << CTX_FRAC_BITS;
	bitcost += x == 1 ? Q : (0);
	bitcost += y == 1 ? Q : (0);

	int64_t a = get_ep_ex_golomb_bitcost_inline_withpow(x);
	int64_t b = get_ep_ex_golomb_bitcost_inline_withpow(y);

	int64_t ans = a + b + bitcost;
	return (ans >> CTX_FRAC_BITS) << 34;
#endif
}


int IMPL(select_mv_cand_int64_hls)(HLS_COMMON_ARG int16_t mv_cand[4], int32_t mv_x, int32_t mv_y, bool calc_cost_out, int64_t *cost_out)
{
#pragma HLS ARRAY_PARTITION variable=mv_cand type=complete
#pragma HLS PIPELINE II = 1

  const bool same_cand =
    (mv_cand[0] == mv_cand[2] && mv_cand[1] == mv_cand[3]);

  if (same_cand && !calc_cost_out) {
    // Pick the first one if both candidates are the same.
    return 0;
  }

  int64_t cand1_cost = get_mvd_coding_cost_hls_inline(
    mv_x - mv_cand[0],
    mv_y - mv_cand[1]);

  int64_t cand2_cost;
  if (same_cand) {
    cand2_cost = cand1_cost;
  }
  else {
    cand2_cost = get_mvd_coding_cost_hls_inline(
      mv_x - mv_cand[2],
      mv_y - mv_cand[3]);
  }

  if (calc_cost_out) {
    *cost_out = MIN(cand1_cost, cand2_cost);
  }

  // Pick the second candidate if it has lower cost.
  return cand2_cost < cand1_cost ? 1 : 0;
}


void IMPL(calc_mvd_cost_int64_hls)(
  HLS_COMMON_ARG
  int x,
  int y,
  int mv_shift,
  int16_t mv_cand[4],
  int64_t* bitcost,
  int64_t lambda_sqrt_integer_int64,
  int64_t lambda_sqrt_decimal_int64,
  int64_t *mvd_cost_int64
)
{
#pragma HLS ARRAY_PARTITION variable=mv_cand type=complete
#pragma HLS PIPELINE II = 1
  //int64_t temp_bitcost = 0;
  //uint32_t merge_idx;
  //int8_t merged = 0;

  x *= 1 << mv_shift;
  y *= 1 << mv_shift;

/*
  // Check every candidate to find a match
  // remove merge_cand because it is NULL and num_cand is 0
  for (merge_idx = 0; merge_idx < (uint32_t)num_cand; merge_idx++) {
    if (merge_cand[merge_idx].dir == 3) continue;

    int dir_idx = merge_cand[merge_idx].dir - 1;
    if (merge_cand[merge_idx].mv[dir_idx*2] == x && merge_cand[merge_idx].mv[(dir_idx)*2+1] == y && ref_LX[(dir_idx)*16 +  merge_cand[merge_idx].ref[dir_idx]] == ref_idx) {
      temp_bitcost += (int64_t)merge_idx * ENTROPY_BITS_WEIGHT;
      merged = 1;
      break;
    }
  }
*/

  // Check mvd cost only if mv is not merged
  //if (!merged) {
    //int64_t mvd_cost = 0;
    select_mv_cand_int64_hls(HLS_COMMON_ARG_CALL mv_cand, x, y, true, bitcost);	//replace &mvd_cost to bitcost
    //temp_bitcost += mvd_cost;

  //}
  //*bitcost = temp_bitcost;
  *mvd_cost_int64 = *bitcost * lambda_sqrt_integer_int64 + ((*bitcost * lambda_sqrt_decimal_int64) >> LAMBDA_DECIMAL_SHIFT_BITS);	//replace temp_bitcost to bitcost
}