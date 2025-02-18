#! /usr/bin/bash

# Set CPU DVFS driver to CPUFreq
echo passive > /sys/devices/system/cpu/intel_pstate/status

# Set governor to performance on every cpu
cpupower frequency-set -g performance