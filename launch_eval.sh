#! /usr/bin/bash

now="date +%d_%m_%Y__%H_%M_%S"

source parameters.sh

description="------ CHACHA POLY CPU BENCHMARK -------"
echo $description

res_dir=res-`$now`
mkdir -p results/$res_dir
res_csv=results/$res_dir/results.csv
res_data=results/$res_dir/EXPE_DATA.txt

# Print Experiment Data
echo $description >> $res_data
echo "tests: $tests" >> $res_data
echo "msg_lens: $msg_lens" >> $res_data
echo "n_workers: $n_workers" >> $res_data
echo "set_pps: $set_pps" >> $res_data
echo "set_duration: $set_duration" >> $res_data
echo "runs: $runs" >> $res_data

echo "run;test;msg_len;workers;pps;duration;total_packets;throughput" > $res_csv 
curr=1
for test in $tests; do
for msg_len in $msg_lens; do
for worker in $n_workers; do
for pps in $set_pps; do
for duration in $set_duration; do
for run in `seq 1 $runs`; do
    printf "%3d/%3d\r" $curr $total
    echo -n "$run;" >> $res_csv
    ./chachapoly-bench $test $msg_len $worker $pps $duration >> $res_csv &
    curr=$(( curr + 1 ))
done
done
done
done
done
done

echo
