#!/bin/bash

set -e
mkdir -p build
cd build
cmake -G "CodeBlocks - Unix Makefiles" ..
make -j$(nproc)
cd ..
size build/03_lr2021_pingpong
