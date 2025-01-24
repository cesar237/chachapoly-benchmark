#! /usr/bin/bash

tests="encryption decryption"
msg_lens="50 100 1500 15000"
n_workers="1 9 18"
set_pps="100000 500000 1000000 2000000"
set_duration="50"
runs="10"

total=$(( 2*4*3*4*1*10 ))