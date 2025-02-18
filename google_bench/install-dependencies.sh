#! /usr/bin/bash

# Install dependencies for chacha and google benchmark
sudo apt install -y libssl-dev cmake git

# Install Google benchmark
cd dependencies
git clone --depth=1 https://github.com/google/benchmark.git
cd benchmark
cmake -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on -DCMAKE_BUILD_TYPE=Release