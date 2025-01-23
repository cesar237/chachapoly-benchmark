#! /usr/bin/bash

# Install go
wget https://go.dev/dl/go1.23.1.linux-amd64.tar.gz
rm -rf /usr/local/go && tar -C /usr/local -xzf go1.23.1.linux-amd64.tar.gz
export PATH=$PATH:/usr/local/go/bin
rm -f go1.23.1.linux-amd64.tar.gz

# Install sysstat
sudo apt install -y sysstat