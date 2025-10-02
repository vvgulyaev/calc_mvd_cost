#!/bin/bash

g++ calc_mvd_cost_int64.cpp  calc_mvd_cost_int64_tb.cpp  -o  calc_mvd_cost_int64_tb

# Check the exit status
if [ $? -eq 0 ]; then
    echo "Build succeeded."
else
    echo "Build failed."
    exit 1
fi

./calc_mvd_cost_int64_tb