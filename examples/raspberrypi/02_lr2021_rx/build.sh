#!/bin/bash

set -e
mkdir -p build
cd build
cmake -G "CodeBlocks - Unix Makefiles" ..
make
cd ..
size build/02_lr2021_rx
