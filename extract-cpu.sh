#! /usr/bin/bash

if [ -z "$1" ]; then
    echo "Please select a res_dir in results..."
    exit 1
else
    run_dir=$1
fi
curr=`pwd`

cd $run_dir

echo "test,msg_len,workers,pps,duration,run,core,usr,kernel,softirq,idle"

for sarfile in $(ls sar); do
    params=$(echo $sarfile | cut -d. -f1)
    test=$(echo $params | cut -d- -f2)
    msg_len=$(echo $params | cut -d- -f3)
    workers=$(echo $params | cut -d- -f4)
    input_pps=$(echo $params | cut -d- -f5)
    duration=$(echo $params | cut -d- -f6)
    run=$(echo $params | cut -d- -f7)

    sadf -d sar/$sarfile -- -u ALL -P ALL \
        | tail -n +2 \
        | awk -v test=$test -v msg_len=$msg_len -v run=$run -v workers=$workers -v input_pps=$input_pps -v duration=$duration 'BEGIN{FS=";"} {print test,msg_len,workers,input_pps,duration,run,$4,$5,$7,$11,$14}' \
        | tr ',' '.' \
        | tr ' ' ','
done

cd $curr