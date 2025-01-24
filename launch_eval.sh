#! /usr/bin/bash


if [ $1 = "continue" ]; then
    cont=1
    if [ -z $2 ]; then
        echo "You want to continue experiment. Give the index to continue"
        exit
    fi
    index=$2
else
    cont=
fi


now="date +%d_%m_%Y__%H_%M_%S"

source parameters.sh

description="------ CHACHA POLY CPU BENCHMARK -------"
echo $description

res_dir=res-`$now`
mkdir -p results/$res_dir/sar
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

if [ -z $cont ]; then
    echo "run;test;msg_len;workers;pps;duration;total_packets;throughput" > $res_csv 
fi

curr=1
for test in $tests; do
for msg_len in $msg_lens; do
for worker in $n_workers; do
for pps in $set_pps; do
for duration in $set_duration; do
for run in `seq 1 $runs`; do
    if ! [ -z $cont ]; then
        if [ $curr -lt $index ]; then
            continue
        fi
    fi
    printf "%3d/%3d\r" $curr $total
    echo -n "$run;" >> $res_csv
    ./chachapoly-bench $test $msg_len $worker $pps $duration >> $res_csv &
    sar -A -o results/$res_dir/sar/sar-$test-$msg_len-$worker-$pps-$duration-$run.data 1 $((duration+2)) > /dev/null
    curr=$(( curr + 1 ))
done
done
done
done
done
done

echo
