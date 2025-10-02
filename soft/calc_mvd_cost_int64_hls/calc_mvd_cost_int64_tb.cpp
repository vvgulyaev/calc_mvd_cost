#include <stdio.h>
#include "tgload.h"

#include "hls.h"
using namespace std;

bool test_calc_mvd_cost_int64_hls(){
#if 1 //(TBCONFIG_CALC_MVD_COST_INT64_HLS || TBCONFIG_ALL)
    fprintf(stderr, "Test calc_mvd_cost_int64_hls\n");
    // define input variables
    int x;
    int y;
    int mv_shift;
    int64_t lambda_sqrt_integer_int64;
    int64_t lambda_sqrt_decimal_int64;

    // define arrays
    int16_t mv_cand [4];

    // define temp variables
    int64_t bitcost;
    int64_t mvd_cost_int64;

    // Open binary output file for writing test data
    FILE* output_file = fopen("calc_mvd_cost_test_data.bin", "wb");
    if (!output_file) {
        fprintf(stderr, "Error: Could not open output file for writing\n");
        return false;
    }

    HLS_COMMON_INIT_VAR();

    // start loading
    tgLoad("calc_mvd_cost_int64_hls_output.bin")

    unsigned int total_count = 0;
    bool finish = false;
    do{
        tgPop(x,y,mv_shift,mv_cand,bitcost,lambda_sqrt_integer_int64,lambda_sqrt_decimal_int64,mvd_cost_int64);
#if DEBUG
        fprintf(stderr, "==== test_id: %d ====\n", total_count);
#endif
        if (finish) {
            break;
        }

        std::cerr << "x:" << hex << x << '\n';
        std::cerr << "y:" << hex << y << '\n';
        std::cerr << "mv_shift:" << hex << mv_shift << '\n';
        std::cerr << "mv_cand:" << hex << mv_cand[0] << ',' << hex << mv_cand[1] << ',' << hex << mv_cand[2] << ',' << hex << mv_cand[3] << '\n';
        //std::cerr << "bitcost:" << bitcost << '\n';
        std::cerr << "lambda_sqrt_integer_int64:" << hex << lambda_sqrt_integer_int64 << '\n';
        std::cerr << "lambda_sqrt_decimal_int64:" << hex << lambda_sqrt_decimal_int64 << '\n';
        //std::cerr << "mvd_cost_int64:" << mvd_cost_int64 << '\n';
/*
        std::cout << "x:" << hex << x << endl;
        std::cout << "y:" << hex << y << endl;
        std::cout << "mv_shift:" << hex << mv_shift << endl;
        std::cout << "mv_cand:" << hex << mv_cand[0] << ',' << hex << mv_cand[1] << ',' << hex << mv_cand[2] << ',' << hex << mv_cand[3] << endl;
        std::cout << "lambda_sqrt_integer_int64:" << hex << lambda_sqrt_integer_int64 << endl;
        std::cout << "lambda_sqrt_decimal_int64:" << hex << lambda_sqrt_decimal_int64 << endl;
*/
        // Write test data to binary file before calling the function
        fwrite(&x, sizeof(int), 1, output_file);
        fwrite(&y, sizeof(int), 1, output_file);
        fwrite(&mv_shift, sizeof(int), 1, output_file);
        fwrite(mv_cand, sizeof(int16_t), 4, output_file);
        fwrite(&lambda_sqrt_integer_int64, sizeof(int64_t), 1, output_file);
        fwrite(&lambda_sqrt_decimal_int64, sizeof(int64_t), 1, output_file);

        // call the function
        calc_mvd_cost_int64_hls(HLS_COMMON_ARG_CALL x,y,mv_shift,mv_cand,&bitcost,lambda_sqrt_integer_int64,lambda_sqrt_decimal_int64,&mvd_cost_int64);

        fwrite(&bitcost, sizeof(int64_t), 1, output_file);
        fwrite(&mvd_cost_int64, sizeof(int64_t), 1, output_file);

        /*std::cout << "bitcost:" << hex << bitcost << endl;
        std::cout << "mvd_cost_int64:" << hex << mvd_cost_int64 << endl;*/


        std::cerr << "bitcost result:" << hex << bitcost << '\n';
        std::cerr << "mvd_cost_int64 result:" << hex << mvd_cost_int64 << '\n';

        tgCheck(mv_cand,bitcost,mvd_cost_int64);
        ++total_count;
    } while(true);

    // Close the output file
    fclose(output_file);

    fprintf(stderr, "Passed after %d\n", total_count);
    return true;
#else
    fprintf(stderr, "Skip calc_mvd_cost_int64_hls\n");
    return true;
#endif
}

int main() {
    test_calc_mvd_cost_int64_hls();
    return 0;
}
