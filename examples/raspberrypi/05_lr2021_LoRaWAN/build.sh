#!/bin/bash

set -e
mkdir -p build
cd build
cmake -G "CodeBlocks - Unix Makefiles" ..
make -j$(nproc)
cd ..
size build/05_lr2021_LoRaWAN
