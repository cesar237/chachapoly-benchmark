#! /usr/bin/bash

g++ -DMSG_LEN=$1 bench.cc -std=c++11 -lbenchmark -lpthread -lssl -lcrypto -o bench_cc
export BENCHMARK_OUT_FORMAT=csv

./bench_cc --benchmark_out_format=csv > res.out

tail -n +4 res.out | awk '{print $1,$2,$4}' | tr / " " | tr " " , > out.csv

