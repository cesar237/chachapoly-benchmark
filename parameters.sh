#! /usr/bin/bash

tests="encryption decryption"
msg_lens="50 100 1500 15000"
n_workers="1 20"
set_pps="100000 500000 1000000"
set_duration="60"
runs="1"

total=$(( 2*4*2*3*1*1 ))