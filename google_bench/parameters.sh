#! /usr/bin/bash

tests="encryption decryption"
msg_lens="50 120 1500 2000"
n_workers="1 18"
set_pps="500000 1000000 2000000"
set_duration="10"
runs="5"

total=$(( 2*4*2*3*1*5 ))